
#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "ServerConfig.hpp"
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <vector>
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>

struct ClientConnection
{
	int fd;
	std::string buffer;
	time_t last_activity;
	bool keep_alive;
	const ServerConfig *server;
	std::string client_ip;
	bool needs_cookie;
};

class HttpRequest;
class HttpResponse;
class LocationConfig;
struct ClientConnection;

class WebServer
{
public:
	WebServer(const std::vector<ServerConfig> &servers);
	~WebServer();
	void run();

private:
	void setupSockets();
	void mainLoop();
	void acceptNewConnection(int server_fd);
	void handleClientData(int client_fd);
	void removeClient(int client_fd);
	void checkTimeouts();
	bool isCompleteRequest(const std::string &buffer);
	void processRequest(ClientConnection &conn);
	void handleGetRequest(ClientConnection &conn, const HttpRequest &request,
						  const LocationConfig &location);
	void handlePostRequest(ClientConnection &conn, const HttpRequest &request,
						   const LocationConfig &location);
	void handlePutRequest(ClientConnection &conn, const HttpRequest &request,
						  const LocationConfig &location);
	void handleDeleteRequest(ClientConnection &conn, const HttpRequest &request,
							 const LocationConfig &location);
	void handleCGIRequest(ClientConnection &conn, const HttpRequest &request,
						  const LocationConfig &location, const std::string &script_path);
	void handleFileUpload(ClientConnection &conn, const HttpRequest &request,
						  const LocationConfig &location);
	void serveStaticFile(int client_fd, const std::string &file_path,
						 bool head_only = false);
	void sendResponse(int client_fd, const HttpResponse &response);
	void sendErrorResponse(int client_fd, int code, const std::string &message,
						   const ServerConfig *server = NULL);
	void sendRedirectResponse(int client_fd, int code,
							  const std::string &location);
	static std::string toString(int num);
	std::vector<ServerConfig> _servers;
	std::vector<struct pollfd> _poll_fds;
	std::vector<int> _server_fds;
	static std::string generateSessionId();
};

#endif
