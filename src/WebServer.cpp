
#include "../inc/CGI.hpp"
#include "../inc/HttpRequest.hpp"
#include "../inc/HttpResponse.hpp"
#include "../inc/WebServer.hpp"
#include "../inc/utils.hpp"

static std::map<int, ClientConnection> g_clients;
static const int BUFFER_SIZE = 8192;
static const int TIMEOUT_SECONDS = 30;

WebServer::WebServer(const std::vector<ServerConfig> &servers) : _servers(servers)
{
}

WebServer::~WebServer()
{
	for (size_t i = 0; i < _poll_fds.size(); i++)
	{
		close(_poll_fds[i].fd);
	}
	g_clients.clear();
}

void WebServer::setupSockets()
{
	int server_fd;
	int opt;
	sockaddr_in addr;
	struct pollfd pfd;

	std::map<std::string, int> used_addresses;
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::ostringstream addr_key;
		addr_key << _servers[i]._host << ":" << _servers[i]._port;
		if (used_addresses.find(addr_key.str()) != used_addresses.end())
		{
			std::cout << "Already listening on " << addr_key.str() << std::endl;
			continue;
		}
		server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd < 0)
		{
			perror("socket");
			continue;
		}
		opt = 1;
		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
					   sizeof(opt)) < 0)
		{
			perror("setsockopt");
			close(server_fd);
			continue;
		}
		fcntl(server_fd, F_SETFL, O_NONBLOCK);
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		if (_servers[i]._host == "0.0.0.0" || _servers[i]._host.empty())
		{
			addr.sin_addr.s_addr = INADDR_ANY;
		}
		else if (_servers[i]._host == "localhost" || _servers[i]._host == "127.0.0.1")
		{
			addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		}
		else
		{
			if (inet_pton(AF_INET, _servers[i]._host.c_str(),
						  &addr.sin_addr) <= 0)
			{
				std::cerr << "Invalid address: " << _servers[i]._host << std::endl;
				close(server_fd);
				continue;
			}
		}
		addr.sin_port = htons(_servers[i]._port);
		if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			perror("bind");
			close(server_fd);
			continue;
		}
		if (listen(server_fd, 128) < 0)
		{
			perror("listen");
			close(server_fd);
			continue;
		}
		pfd.fd = server_fd;
		pfd.events = POLLIN;
		_poll_fds.push_back(pfd);
		_server_fds.push_back(server_fd);
		used_addresses[addr_key.str()] = server_fd;
		std::cout << "✓ Listening on " << _servers[i]._host << ":" << _servers[i]._port;
		if (!_servers[i]._server_names.empty())
		{
			std::cout << " (";
			for (size_t j = 0; j < _servers[i]._server_names.size(); j++)
			{
				if (j > 0)
					std::cout << ", ";
				std::cout << _servers[i]._server_names[j];
			}
			std::cout << ")";
		}
		std::cout << std::endl;
	}
	if (_server_fds.empty())
	{
		throw std::runtime_error("Failed to bind any server socket");
	}
}

void WebServer::run()
{
	setupSockets();
	std::cout << "\n🚀 Webserv started successfully!\n"
			  << std::endl;
	mainLoop();
}

void WebServer::mainLoop()
{
	int activity;

	while (true)
	{
		checkTimeouts();
		activity = poll(_poll_fds.data(), _poll_fds.size(), 1000);
		if (activity < 0)
		{
			if (errno != EINTR)
			{
				perror("poll");
				break;
			}
			continue;
		}
		for (size_t i = 0; i < _poll_fds.size(); i++)
		{
			if (_poll_fds[i].revents & POLLIN)
			{
				if (std::find(_server_fds.begin(), _server_fds.end(),
							  _poll_fds[i].fd) != _server_fds.end())
				{
					acceptNewConnection(_poll_fds[i].fd);
				}
				else
				{
					handleClientData(_poll_fds[i].fd);
				}
			}
			else if (_poll_fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				if (std::find(_server_fds.begin(), _server_fds.end(),
							  _poll_fds[i].fd) == _server_fds.end())
				{
					removeClient(_poll_fds[i].fd);
				}
			}
		}
	}
}

void WebServer::acceptNewConnection(int server_fd)
{
	sockaddr_in client_addr;
	socklen_t client_len;
	int client_fd;
	struct pollfd client_pfd;
	ClientConnection conn;
	char client_ip[INET_ADDRSTRLEN];
	sockaddr_in local_addr;
	socklen_t local_len;
	int local_port;

	client_len = sizeof(client_addr);
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("accept");
		}
		return;
	}
	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	client_pfd.fd = client_fd;
	client_pfd.events = POLLIN;
	_poll_fds.push_back(client_pfd);
	conn.fd = client_fd;
	conn.buffer = "";
	conn.last_activity = time(NULL);
	conn.keep_alive = false;
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
	conn.client_ip = client_ip;
	conn.server = &_servers[0];
	local_len = sizeof(local_addr);
	if (getsockname(server_fd, (struct sockaddr *)&local_addr, &local_len) == 0)
	{
		local_port = ntohs(local_addr.sin_port);
		for (size_t i = 0; i < _servers.size(); i++)
		{
			if (_servers[i]._port == local_port)
			{
				conn.server = &_servers[i];
				break;
			}
		}
	}
	g_clients[client_fd] = conn;
	std::cout << "✓ New client connected: " << client_ip << " (fd: " << client_fd << ")" << std::endl;
}

void WebServer::handleClientData(int client_fd)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytes;

	std::map<int, ClientConnection>::iterator it = g_clients.find(client_fd);
	if (it == g_clients.end())
	{
		return;
	}
	ClientConnection &conn = it->second;
	conn.last_activity = time(NULL);
	bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0)
	{
		if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
		{
			std::cout << "Client disconnected: " << conn.client_ip << " (fd: " << client_fd << ")" << std::endl;
			removeClient(client_fd);
		}
		return;
	}
	buffer[bytes] = '\0';
	conn.buffer.append(buffer, bytes);
	if (isCompleteRequest(conn.buffer))
	{
		processRequest(conn);
		conn.buffer.clear();
		if (!conn.keep_alive)
		{
			removeClient(client_fd);
		}
	}
	else if (conn.buffer.size() > 1024 * 1024)
	{
		sendErrorResponse(client_fd, 413, "Payload Too Large", conn.server);
		removeClient(client_fd);
	}
}

bool WebServer::isCompleteRequest(const std::string &buffer)
{
	size_t content_length;
	size_t pos;
	size_t end;
	size_t body_start;
	size_t body_length;

	if (buffer.find("\r\n\r\n") == std::string::npos && buffer.find("\n\n") == std::string::npos)
	{
		return (false);
	}
	content_length = 0;
	pos = buffer.find("Content-Length:");
	if (pos == std::string::npos)
	{
		pos = buffer.find("content-length:");
	}
	if (pos != std::string::npos)
	{
		end = buffer.find("\r\n", pos);
		if (end == std::string::npos)
		{
			end = buffer.find("\n", pos);
		}
		if (end != std::string::npos)
		{
			std::string length_str = buffer.substr(pos + 15, end - pos - 15);
			std::istringstream iss(length_str);
			iss >> content_length;
			body_start = buffer.find("\r\n\r\n");
			if (body_start == std::string::npos)
			{
				body_start = buffer.find("\n\n");
				if (body_start != std::string::npos)
				{
					body_start += 2;
				}
			}
			else
			{
				body_start += 4;
			}
			if (body_start != std::string::npos)
			{
				body_length = buffer.length() - body_start;
				return (body_length >= content_length);
			}
		}
	}
	return (true);
}

std::string generateSimpleSessionId()
{
	srand(time(NULL));
	std::ostringstream oss;
	for (int i = 0; i < 16; i++)
	{
		oss << std::hex << (rand() % 16);
	}
	return (oss.str());
}

void WebServer::processRequest(ClientConnection &conn)
{
	HttpRequest request;

	if (!request.parse(conn.buffer))
	{
		sendErrorResponse(conn.fd, 400, "Bad Request", conn.server);
		return;
	}

	std::string cookie_header = request.getHeader("cookie");
	bool has_session_cookie = false;
	std::string session_id;

	if (!cookie_header.empty())
	{

		size_t session_pos = cookie_header.find("WEBSERV_SESSION=");
		if (session_pos != std::string::npos)
		{
			has_session_cookie = true;
			size_t start = session_pos + 16;
			size_t end = cookie_header.find(';', start);
			if (end == std::string::npos)
				end = cookie_header.length();

			session_id = cookie_header.substr(start, end - start);

			size_t first = session_id.find_first_not_of(" \t");
			size_t last = session_id.find_last_not_of(" \t");
			if (first != std::string::npos && last != std::string::npos)
				session_id = session_id.substr(first, last - first + 1);

			std::cout << "🍪 Client " << conn.client_ip << " has existing session: "
					  << session_id << std::endl;
		}
	}

	conn.needs_cookie = !has_session_cookie;

	if (conn.needs_cookie)
	{
		std::cout << "🆕 Client " << conn.client_ip << " needs new session cookie" << std::endl;
	}

	std::string host = request.getHeader("host");
	if (!host.empty())
	{
		size_t colon = host.find(':');
		if (colon != std::string::npos)
		{
			host = host.substr(0, colon);
		}

		for (size_t i = 0; i < _servers.size(); i++)
		{
			for (size_t j = 0; j < _servers[i]._server_names.size(); j++)
			{
				if (_servers[i]._server_names[j] == host &&
					_servers[i]._port == conn.server->_port)
				{
					conn.server = &_servers[i];
					break;
				}
			}
		}
	}

	std::cout << "📥 " << request.getMethod() << " " << request.getUri()
			  << " from " << conn.client_ip << " (fd:" << conn.fd << ")"
			  << " [Server: " << (conn.server->_server_names.empty() ? "default" : conn.server->_server_names[0]) << "]";

	if (has_session_cookie)
		std::cout << " [Session: " << session_id.substr(0, 8) << "...]";

	std::cout << std::endl;

	std::string connection = request.getHeader("connection");
	conn.keep_alive = (request.getHttpVersion() == "HTTP/1.1" &&
					   toLowerCase(connection) != "close") ||
					  (toLowerCase(connection) == "keep-alive");

	const LocationConfig &location = conn.server->findLocationForRequest(request.getUri());

	size_t max_body_size = conn.server->_client_max_body_size;
	if (request.getBody().size() > max_body_size)
	{
		sendErrorResponse(conn.fd, 413, "Payload Too Large", conn.server);
		return;
	}

	if (!location._allowed_methods.empty())
	{
		bool method_allowed = false;
		for (size_t i = 0; i < location._allowed_methods.size(); i++)
		{
			if (location._allowed_methods[i] == request.getMethod())
			{
				method_allowed = true;
				break;
			}
		}

		if (!method_allowed)
		{
			sendErrorResponse(conn.fd, 405, "Method Not Allowed", conn.server);
			return;
		}
	}

	if (request.getMethod() == "GET" || request.getMethod() == "HEAD")
	{
		handleGetRequest(conn, request, location);
	}
	else if (request.getMethod() == "POST")
	{
		handlePostRequest(conn, request, location);
	}
	else if (request.getMethod() == "PUT")
	{
		handlePutRequest(conn, request, location);
	}
	else if (request.getMethod() == "DELETE")
	{
		handleDeleteRequest(conn, request, location);
	}
	else
	{
		sendErrorResponse(conn.fd, 501, "Not Implemented", conn.server);
	}
}

std::string WebServer::generateSessionId()
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	std::string session_id;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec * 1000000 + tv.tv_usec + getpid());

	for (int i = 0; i < 32; ++i)
	{
		session_id += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return session_id;
}

void WebServer::sendResponse(int client_fd, const HttpResponse &response)
{
	std::string data = response.serialize();
	std::map<int, ClientConnection>::iterator it = g_clients.find(client_fd);

	if (it != g_clients.end() && it->second.needs_cookie)
	{

		size_t header_end = data.find("\r\n\r\n");
		if (header_end != std::string::npos)
		{

			std::ostringstream session_id;
			srand(time(NULL) + client_fd + rand());

			for (int i = 0; i < 32; i++)
			{
				session_id << std::hex << (rand() % 16);
			}

			std::string cookie = "Set-Cookie: WEBSERV_SESSION=" + session_id.str() +
								 "; Path=/; Max-Age=3600; HttpOnly\r\n";

			data.insert(header_end, cookie);

			std::cout << "🍪 Set new session cookie for client " << it->second.client_ip
					  << " (fd:" << client_fd << "): " << session_id.str() << std::endl;

			it->second.needs_cookie = false;
		}
	}

	ssize_t sent = send(client_fd, data.c_str(), data.length(), 0);
	if (sent < 0)
	{
		std::cerr << "Error sending response to client " << client_fd << ": "
				  << strerror(errno) << std::endl;
	}
}

void WebServer::handleGetRequest(ClientConnection &conn,
								 const HttpRequest &request, const LocationConfig &location)
{
	size_t query_pos;
	HttpResponse response;

	if (!location._redirect.empty())
	{
		sendRedirectResponse(conn.fd, 301, location._redirect);
		return;
	}
	std::string file_path = location._root;
	if (file_path.empty())
	{
		file_path = "./www";
	}
	std::string uri = request.getUri();
	query_pos = uri.find('?');
	if (query_pos != std::string::npos)
	{
		uri = uri.substr(0, query_pos);
	}
	uri = urlDecode(uri);
	if (uri.find("../") != std::string::npos)
	{
		sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
		return;
	}
	std::string original_uri = uri;
	if (location._path != "/" && uri.find(location._path) == 0)
	{
		if (uri == location._path)
		{
			uri = "/";
		}
		else if (uri.length() > location._path.length() && uri[location._path.length()] == '/')
		{
			uri = uri.substr(location._path.length());
		}
	}
	if (location._path == "/uploads")
	{
		file_path = "www/uploads";
		if (uri != "/")
		{
			file_path += uri;
		}
	}
	else if (location._path == "/cgi-bin")
	{
		file_path = "www/cgi-bin";
		if (uri != "/")
		{
			file_path += uri;
		}
	}
	else
	{
		file_path += uri;
	}
	if (!location._cgi_extension.empty() && file_path.find(location._cgi_extension) != std::string::npos)
	{
		handleCGIRequest(conn, request, location, file_path);
		return;
	}
	if (!fileExists(file_path))
	{
		sendErrorResponse(conn.fd, 404, "Not Found", conn.server);
		return;
	}
	if (isDirectory(file_path))
	{
		if (file_path[file_path.length() - 1] != '/')
		{
			sendRedirectResponse(conn.fd, 301, original_uri + "/");
			return;
		}
		std::string index_path = file_path + location._index_file;
		if (!location._index_file.empty() && fileExists(index_path))
		{
			file_path = index_path;
		}
		else if (location._directory_listing)
		{
			std::string listing_uri = original_uri;
			if (listing_uri[listing_uri.length() - 1] != '/')
			{
				listing_uri += "/";
			}
			std::string listing = generateDirectoryListing(file_path,
														   listing_uri);
			if (listing.empty())
			{
				sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
				return;
			}
			response.setStatusCode(200);
			response.setBody(listing);
			response.addHeader("content-type", "text/html");
			sendResponse(conn.fd, response);
			return;
		}
		else
		{
			sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
			return;
		}
	}
	if (!isReadable(file_path))
	{
		sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
		return;
	}
	serveStaticFile(conn.fd, file_path, request.getMethod() == "HEAD");
}

void WebServer::handlePostRequest(ClientConnection &conn,
								  const HttpRequest &request, const LocationConfig &location)
{
	size_t query_pos;

	std::string uri = request.getUri();
	query_pos = uri.find('?');
	if (query_pos != std::string::npos)
	{
		uri = uri.substr(0, query_pos);
	}
	std::string file_path = location._root;
	if (file_path.empty())
	{
		file_path = "./www";
	}
	if (location._path != "/" && uri.find(location._path) == 0)
	{
		if (uri == location._path)
		{
			uri = "/";
		}
		else if (uri.length() > location._path.length() && uri[location._path.length()] == '/')
		{
			uri = uri.substr(location._path.length());
		}
	}
	if (location._path == "/uploads")
	{
		file_path = "www/uploads";
		if (uri != "/")
		{
			file_path += uri;
		}
	}
	else if (location._path == "/cgi-bin")
	{
		file_path = "www/cgi-bin";
		if (uri != "/")
		{
			file_path += uri;
		}
	}
	else
	{
		file_path += uri;
	}
	if (!location._cgi_extension.empty() && file_path.find(location._cgi_extension) != std::string::npos)
	{
		handleCGIRequest(conn, request, location, file_path);
		return;
	}
	if (!location._upload_path.empty())
	{
		handleFileUpload(conn, request, location);
		return;
	}
	sendErrorResponse(conn.fd, 405, "Method Not Allowed", conn.server);
}

void WebServer::handlePutRequest(ClientConnection &conn,
								 const HttpRequest &request, const LocationConfig &location)
{
	bool file_existed;
	HttpResponse response;

	std::string file_path = location._root;
	if (file_path.empty())
	{
		file_path = "./www";
	}
	std::string uri = urlDecode(request.getUri());
	if (uri.find("../") != std::string::npos)
	{
		sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
		return;
	}
	if (location._path != "/" && uri.find(location._path) == 0)
	{
		uri = uri.substr(location._path.length());
		if (uri.empty() || uri[0] != '/')
		{
			uri = "/" + uri;
		}
	}
	file_path += uri;
	std::string dir_path = file_path.substr(0, file_path.find_last_of('/'));
	if (!fileExists(dir_path))
	{
		sendErrorResponse(conn.fd, 404, "Not Found", conn.server);
		return;
	}
	file_existed = fileExists(file_path);
	if (writeFile(file_path, request.getBody()))
	{
		response.setStatusCode(file_existed ? 204 : 201);
		if (!file_existed)
		{
			response.addHeader("location", request.getUri());
		}
		sendResponse(conn.fd, response);
		std::cout << "📝 PUT file: " << file_path << " (" << (file_existed ? "updated" : "created") << ")" << std::endl;
	}
	else
	{
		sendErrorResponse(conn.fd, 500, "Internal Server Error", conn.server);
	}
}

void WebServer::handleDeleteRequest(ClientConnection &conn,
									const HttpRequest &request, const LocationConfig &location)
{
	HttpResponse response;

	std::string file_path = location._root;
	if (file_path.empty())
	{
		file_path = "./www";
	}
	std::string uri = urlDecode(request.getUri());
	if (uri.find("../") != std::string::npos)
	{
		sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
		return;
	}
	if (location._path != "/" && uri.find(location._path) == 0)
	{
		if (uri == location._path)
		{
			uri = "/";
		}
		else if (uri.length() > location._path.length() && uri[location._path.length()] == '/')
		{
			uri = uri.substr(location._path.length());
		}
	}
	if (location._path == "/uploads")
	{
		file_path = "www/uploads";
		if (uri != "/")
		{
			file_path += uri;
		}
	}
	else if (location._path == "/cgi-bin")
	{
		file_path = "www/cgi-bin";
		if (uri != "/")
		{
			file_path += uri;
		}
	}
	else
	{
		file_path += uri;
	}
	if (!fileExists(file_path))
	{
		sendErrorResponse(conn.fd, 404, "Not Found", conn.server);
		return;
	}
	if (isDirectory(file_path))
	{
		sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
		return;
	}
	if (!isWritable(file_path))
	{
		sendErrorResponse(conn.fd, 403, "Forbidden", conn.server);
		return;
	}
	if (unlink(file_path.c_str()) == 0)
	{
		response.setStatusCode(204);
		sendResponse(conn.fd, response);
		std::cout << "🗑️  Deleted: " << file_path << std::endl;
	}
	else
	{
		sendErrorResponse(conn.fd, 500, "Internal Server Error", conn.server);
	}
}

void WebServer::handleCGIRequest(ClientConnection &conn,
								 const HttpRequest &request, const LocationConfig &location,
								 const std::string &script_path)
{
	std::cout << "🔧 Executing CGI: " << script_path << std::endl;
	if (!fileExists(script_path))
	{
		std::cerr << "CGI Error: Script not found: " << script_path << std::endl;
		sendErrorResponse(conn.fd, 404, "CGI Script Not Found", conn.server);
		return;
	}
	if (!fileExists(location._cgi_path))
	{
		std::cerr << "CGI Error: Interpreter not found: " << location._cgi_path << std::endl;
		sendErrorResponse(conn.fd, 500, "CGI Interpreter Not Found",
						  conn.server);
		return;
	}
	if (!isReadable(script_path))
	{
		std::cerr << "CGI Error: Script not readable: " << script_path << std::endl;
		sendErrorResponse(conn.fd, 403, "CGI Script Not Readable", conn.server);
		return;
	}
	CGI cgi(request, location);
	std::string response = cgi.execute(script_path);
	if (response.empty())
	{
		sendErrorResponse(conn.fd, 500, "CGI Execution Failed", conn.server);
		return;
	}
	send(conn.fd, response.c_str(), response.length(), 0);
}

void WebServer::handleFileUpload(ClientConnection &conn,
								 const HttpRequest &request, const LocationConfig &location)
{
	HttpResponse response;
	size_t boundary_pos;
	size_t filename_pos;
	size_t filename_end;
	size_t content_start;
	size_t content_end;

	std::string content_type = request.getHeader("content-type");
	std::string upload_path = location._upload_path;
	if (upload_path.empty())
	{
		upload_path = "www/uploads";
	}
	if (!fileExists(upload_path))
	{
		mkdir(upload_path.c_str(), 0755);
	}
	if (content_type.find("multipart/form-data") != std::string::npos)
	{
		boundary_pos = content_type.find("boundary=");
		if (boundary_pos == std::string::npos)
		{
			sendErrorResponse(conn.fd, 400, "Bad Request - No boundary",
							  conn.server);
			return;
		}
		std::string boundary = "--" + content_type.substr(boundary_pos + 9);
		std::string body = request.getBody();
		filename_pos = body.find("filename=\"");
		if (filename_pos == std::string::npos)
		{
			sendErrorResponse(conn.fd, 400, "Bad Request - No filename",
							  conn.server);
			return;
		}
		filename_pos += 10;
		filename_end = body.find("\"", filename_pos);
		std::string filename = body.substr(filename_pos, filename_end - filename_pos);
		content_start = body.find("\r\n\r\n", filename_end);
		if (content_start == std::string::npos)
		{
			content_start = body.find("\n\n", filename_end);
			if (content_start == std::string::npos)
			{
				sendErrorResponse(conn.fd, 400, "Bad Request - Invalid format",
								  conn.server);
				return;
			}
			content_start += 2;
		}
		else
		{
			content_start += 4;
		}
		content_end = body.find(boundary, content_start);
		if (content_end == std::string::npos)
		{
			sendErrorResponse(conn.fd, 400, "Bad Request - No end boundary",
							  conn.server);
			return;
		}
		if (content_end >= 2 && body[content_end - 2] == '\r' && body[content_end - 1] == '\n')
		{
			content_end -= 2;
		}
		else if (content_end >= 1 && body[content_end - 1] == '\n')
		{
			content_end -= 1;
		}
		std::string file_content = body.substr(content_start, content_end - content_start);
		std::string full_path = upload_path + "/" + filename;
		if (writeFile(full_path, file_content))
		{
			response.setStatusCode(201);
			response.setBody("File uploaded successfully: " + filename);
			response.addHeader("content-type", "text/plain");
			response.addHeader("location", "/" + upload_path + "/" + filename);
			sendResponse(conn.fd, response);
			std::cout << "📤 File uploaded: " << full_path << " (" << file_content.length() << " bytes)" << std::endl;
		}
		else
		{
			sendErrorResponse(conn.fd, 500, "Failed to save file", conn.server);
		}
	}
	else
	{
		std::string filename = "upload_" + toString(time(NULL)) + ".txt";
		std::string full_path = upload_path + "/" + filename;
		if (writeFile(full_path, request.getBody()))
		{
			response.setStatusCode(201);
			response.setBody("Data uploaded successfully: " + filename);
			response.addHeader("content-type", "text/plain");
			response.addHeader("location", "/" + upload_path + "/" + filename);
			sendResponse(conn.fd, response);
			std::cout << "📤 Data uploaded: " << full_path << std::endl;
		}
		else
		{
			sendErrorResponse(conn.fd, 500, "Failed to save data", conn.server);
		}
	}
}

void WebServer::serveStaticFile(int client_fd, const std::string &file_path,
								bool head_only)
{
	HttpResponse response;

	std::string content = readFile(file_path);
	if (content.empty() && getFileSize(file_path) > 0)
	{
		sendErrorResponse(client_fd, 500, "Failed to read file");
		return;
	}
	response.setStatusCode(200);
	if (!head_only)
	{
		response.setBody(content);
	}
	response.addHeader("content-type", getMimeType(file_path));
	response.addHeader("content-length", toString(content.length()));
	sendResponse(client_fd, response);
}

/* void WebServer::sendResponse(int client_fd, const HttpResponse &response)
{
	size_t header_end;
	ssize_t sent;

	std::string data = response.serialize();
	std::map<int, ClientConnection>::iterator it = g_clients.find(client_fd);

	if (it != g_clients.end())
	{
		bool has_session_cookie = data.find("Set-Cookie: WEBSERV_SESSION=") != std::string::npos;

		if (it->second.needs_cookie || !has_session_cookie)
		{
			header_end = data.find("\r\n\r\n");
			if (header_end != std::string::npos)
			{
				std::ostringstream session_id;
				srand(time(NULL) + client_fd);

				for (int i = 0; i < 16; i++)
				{
					session_id << std::hex << (rand() % 16);
				}

				std::string cookie = "Set-Cookie: WEBSERV_SESSION=" + session_id.str() +
									 "; Path=/; Max-Age=3600; HttpOnly\r\n";

				data.insert(header_end, cookie);

				std::cout << "🍪 Set session cookie for client " << client_fd
						  << ": " << session_id.str() << std::endl;
			}
		}
	}

	sent = send(client_fd, data.c_str(), data.length(), 0);
	if (sent < 0)
	{
		std::cerr << "Error sending response" << std::endl;
	}
} */

void WebServer::sendErrorResponse(int client_fd, int code,
								  const std::string &message, const ServerConfig *server)
{
	HttpResponse response;

	if (server)
	{
		std::map<int,
				 std::string>::const_iterator it = server->_error_pages.find(code);
		if (it != server->_error_pages.end())
		{
			std::string error_page = readFile(it->second);
			if (!error_page.empty())
			{
				response.setStatusCode(code);
				response.setBody(error_page);
				response.addHeader("content-type", "text/html");
				sendResponse(client_fd, response);
				return;
			}
		}
	}
	response.setError(code, message);
	sendResponse(client_fd, response);
}

void WebServer::sendRedirectResponse(int client_fd, int code,
									 const std::string &location)
{
	HttpResponse response;

	response.setStatusCode(code);
	response.addHeader("location", location);
	std::string body = "<!DOCTYPE html><html><head><title>Redirect</title></head>"
					   "<body><h1>Redirecting...</h1><p>Redirecting to "
					   "<a href=\"" +
					   location + "\">" + location + "</a></p></body></html>";
	response.setBody(body);
	response.addHeader("content-type", "text/html");
	sendResponse(client_fd, response);
}

void WebServer::removeClient(int client_fd)
{
	g_clients.erase(client_fd);
	close(client_fd);
	for (std::vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it)
	{
		if (it->fd == client_fd)
		{
			_poll_fds.erase(it);
			break;
		}
	}
}

void WebServer::checkTimeouts()
{
	time_t now;

	now = time(NULL);
	std::vector<int> to_remove;
	for (std::map<int,
				  ClientConnection>::iterator it = g_clients.begin();
		 it != g_clients.end(); ++it)
	{
		if (now - it->second.last_activity > TIMEOUT_SECONDS)
		{
			to_remove.push_back(it->first);
		}
	}
	for (size_t i = 0; i < to_remove.size(); ++i)
	{
		std::cout << "⏱️  Timeout: closing connection " << to_remove[i] << std::endl;
		removeClient(to_remove[i]);
	}
}

std::string WebServer::toString(int num)
{
	std::ostringstream oss;
	oss << num;
	return (oss.str());
}
