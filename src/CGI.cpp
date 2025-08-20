
#include "../inc/CGI.hpp"
#include "../inc/HttpRequest.hpp"
#include "../inc/LocationConfig.hpp"

CGI::CGI(const HttpRequest &request,
         const LocationConfig &location) : request_(request), location_(location),
                                           env_vars_(), timeout_seconds_(30)
{
    setupEnvironment();
}

CGI::~CGI()
{
    for (size_t i = 0; i < env_vars_.size(); ++i)
    {
        delete[] env_vars_[i];
    }
}

std::string CGI::execute(const std::string &script_path)
{
    int pipe_in[2];
    int pipe_out[2];
    pid_t pid;

    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
    {
        return (generateErrorResponse(500, "Failed to create pipes"));
    }
    pid = fork();
    if (pid == -1)
    {
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        return (generateErrorResponse(500, "Failed to fork process"));
    }
    if (pid == 0)
    {
        executeCGIChild(script_path, pipe_in, pipe_out);
        exit(1);
    }
    else
    {
        return (handleCGIParent(pid, pipe_in, pipe_out));
    }
}

void CGI::setupEnvironment()
{
    size_t query_pos;

    env_map_.clear();
    env_map_["REQUEST_METHOD"] = request_.getMethod();
    env_map_["SERVER_PROTOCOL"] = request_.getHttpVersion();
    env_map_["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_map_["SERVER_SOFTWARE"] = "webserv/1.0";
    env_map_["SERVER_NAME"] = "localhost";
    env_map_["SERVER_PORT"] = "8080";

    std::string uri = request_.getUri();
    query_pos = uri.find('?');
    if (query_pos != std::string::npos)
    {
        env_map_["PATH_INFO"] = uri.substr(0, query_pos);
        env_map_["QUERY_STRING"] = uri.substr(query_pos + 1);
    }
    else
    {
        env_map_["PATH_INFO"] = uri;
        env_map_["QUERY_STRING"] = "";
    }

    env_map_["SCRIPT_NAME"] = env_map_["PATH_INFO"];

    env_map_["REQUEST_URI"] = uri;

    const std::map<std::string, std::string> &headers = request_.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        std::string env_name = "HTTP_" + toUpperSnakeCase(it->first);
        env_map_[env_name] = it->second;
    }

    if (request_.getMethod() == "POST")
    {
        std::string content_type = request_.getHeader("content-type");
        std::string content_length = request_.getHeader("content-length");
        if (!content_type.empty())
        {
            env_map_["CONTENT_TYPE"] = content_type;
        }
        if (!content_length.empty())
        {
            env_map_["CONTENT_LENGTH"] = content_length;
        }
    }

    env_map_["REMOTE_ADDR"] = "127.0.0.1";
    env_map_["REMOTE_HOST"] = "localhost";

    char *path_env = getenv("PATH");
    if (path_env)
    {
        env_map_["PATH"] = path_env;
    }
    else
    {

        env_map_["PATH"] = "/usr/local/bin:/usr/bin:/bin";
    }

    buildEnvArray();
}

void CGI::buildEnvArray()
{
    char *env_var;

    for (size_t i = 0; i < env_vars_.size(); ++i)
    {
        delete[] env_vars_[i];
    }
    env_vars_.clear();
    for (std::map<std::string,
                  std::string>::const_iterator it = env_map_.begin();
         it != env_map_.end(); ++it)
    {
        std::string env_str = it->first + "=" + it->second;
        env_var = new char[env_str.length() + 1];
        std::strcpy(env_var, env_str.c_str());
        env_vars_.push_back(env_var);
    }
    env_vars_.push_back(NULL);
}

void CGI::executeCGIChild(const std::string &script_path, int pipe_in[2],
                          int pipe_out[2])
{
    const char *argv[3];
    size_t last_slash;

    close(pipe_in[1]);
    dup2(pipe_in[0], STDIN_FILENO);
    close(pipe_in[0]);
    close(pipe_out[0]);
    dup2(pipe_out[1], STDOUT_FILENO);
    close(pipe_out[1]);

    std::string directory = getDirectoryPath(script_path);
    if (!directory.empty())
    {
        if (chdir(directory.c_str()) != 0)
        {
            std::cerr << "CGI: Failed to change directory to: " << directory << std::endl;
            exit(1);
        }
    }

    std::string script_name = script_path;
    last_slash = script_path.find_last_of('/');
    if (last_slash != std::string::npos)
    {
        script_name = script_path.substr(last_slash + 1);
    }
    argv[0] = location_._cgi_path.c_str();
    argv[1] = script_name.c_str();
    argv[2] = NULL;

    std::cerr << "CGI: Executing: " << argv[0] << " " << argv[1] << std::endl;
    std::cerr << "CGI: Working directory: " << directory << std::endl;
    execve(argv[0], const_cast<char **>(argv), &env_vars_[0]);

    std::cerr << "CGI execution failed: " << strerror(errno) << std::endl;
    std::cerr << "CGI path: " << argv[0] << std::endl;
    std::cerr << "Script: " << argv[1] << std::endl;
    exit(1);
}

std::string CGI::handleCGIParent(pid_t pid, int pipe_in[2], int pipe_out[2])
{
    int flags;
    char buffer[4096];
    time_t start_time;
    int status;
    pid_t result;
    ssize_t bytes;

    close(pipe_in[0]);
    close(pipe_out[1]);
    if (request_.getMethod() == "POST" && !request_.getBody().empty())
    {
        write(pipe_in[1], request_.getBody().c_str(),
              request_.getBody().length());
    }
    close(pipe_in[1]);
    flags = fcntl(pipe_out[0], F_GETFL, 0);
    fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);
    std::string output;
    start_time = time(NULL);
    while (true)
    {
        if (time(NULL) - start_time > timeout_seconds_)
        {
            kill(pid, SIGTERM);
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                int exit_code = WEXITSTATUS(status);
                std::cerr << "CGI exited with code: " << exit_code << std::endl;
                if (exit_code != 0)
                {
                    std::cerr << "CGI output: " << output << std::endl;
                    return generateErrorResponse(500, "CGI script error (exit code: " + toString(exit_code) + ")");
                }
            }
            close(pipe_out[0]);
            return (generateErrorResponse(504, "CGI timeout"));
        }
        result = waitpid(pid, &status, WNOHANG);
        if (result == pid)
        {
            while ((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
            {
                output.append(buffer, bytes);
            }
            break;
        }
        bytes = read(pipe_out[0], buffer, sizeof(buffer));
        if (bytes > 0)
        {
            output.append(buffer, bytes);
        }
        else if (bytes == 0)
        {
            break;
        }
        usleep(10000);
    }
    close(pipe_out[0]);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
        return (generateErrorResponse(500, "CGI script error"));
    }
    return (parseCGIOutput(output));
}

std::string CGI::parseCGIOutput(const std::string &raw_output)
{
    if (raw_output.empty())
    {
        return generateErrorResponse(500, "Empty CGI response");
    }

    size_t separator = raw_output.find("\r\n\r\n");
    bool use_crlf = true;
    if (separator == std::string::npos)
    {
        separator = raw_output.find("\n\n");
        use_crlf = false;
        if (separator == std::string::npos)
        {

            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " +
                   toString(raw_output.length()) + "\r\n"
                                                   "Connection: close\r\n"
                                                   "\r\n" +
                   raw_output;
        }
    }

    std::string headers = raw_output.substr(0, separator);
    std::string body = raw_output.substr(separator + (use_crlf ? 4 : 2));

    std::ostringstream response;

    std::string status_code = "200";
    std::string status_text = "OK";
    size_t status_pos = headers.find("Status:");
    if (status_pos != std::string::npos)
    {
        size_t status_end = headers.find('\n', status_pos);
        std::string status_line = headers.substr(status_pos + 7, status_end - status_pos - 7);

        if (!status_line.empty() && status_line[status_line.length() - 1] == '\r')
        {
            status_line = status_line.substr(0, status_line.length() - 1);
        }

        size_t first = status_line.find_first_not_of(" \t");
        size_t last = status_line.find_last_not_of(" \t");
        if (first != std::string::npos)
        {
            status_line = status_line.substr(first, last - first + 1);
        }

        size_t space_pos = status_line.find(' ');
        if (space_pos != std::string::npos)
        {
            status_code = status_line.substr(0, space_pos);
            status_text = status_line.substr(space_pos + 1);
        }
        else
        {
            status_code = status_line;
            status_text = getStatusMessage(atoi(status_code.c_str()));
        }
    }

    response << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";

    std::istringstream header_stream(headers);
    std::string header_line;
    std::vector<std::string> set_cookie_headers;
    bool has_content_type = false;
    bool has_content_length = false;
    std::string content_type_value = "text/html";

    while (std::getline(header_stream, header_line))
    {

        if (!header_line.empty() && header_line[header_line.length() - 1] == '\r')
        {
            header_line = header_line.substr(0, header_line.length() - 1);
        }

        if (header_line.empty())
        {
            continue;
        }

        if (header_line.find("Status:") == 0)
        {
            continue;
        }

        size_t colon_pos = header_line.find(':');
        if (colon_pos == std::string::npos)
        {
            continue;
        }

        std::string header_name = header_line.substr(0, colon_pos);
        std::string header_value = header_line.substr(colon_pos + 1);

        size_t value_start = header_value.find_first_not_of(" \t");
        if (value_start != std::string::npos)
        {
            header_value = header_value.substr(value_start);
        }

        std::string lower_header_name = header_name;
        std::transform(lower_header_name.begin(), lower_header_name.end(),
                       lower_header_name.begin(),
                       static_cast<int (*)(int)>(::tolower));

        if (lower_header_name == "set-cookie")
        {

            set_cookie_headers.push_back("Set-Cookie: " + header_value);
        }
        else if (lower_header_name == "content-type")
        {
            has_content_type = true;
            content_type_value = header_value;
            response << "Content-Type: " << header_value << "\r\n";
        }
        else if (lower_header_name == "content-length")
        {
            has_content_length = true;
            response << "Content-Length: " << header_value << "\r\n";
        }
        else if (lower_header_name == "location")
        {

            response << "Location: " << header_value << "\r\n";
        }
        else if (lower_header_name == "cache-control" ||
                 lower_header_name == "expires" ||
                 lower_header_name == "pragma")
        {

            response << header_name << ": " << header_value << "\r\n";
        }
        else
        {

            if (lower_header_name != "connection" &&
                lower_header_name != "transfer-encoding" &&
                lower_header_name != "server")
            {
                response << header_name << ": " << header_value << "\r\n";
            }
        }
    }

    for (std::vector<std::string>::const_iterator it = set_cookie_headers.begin();
         it != set_cookie_headers.end(); ++it)
    {
        response << *it << "\r\n";
    }

    if (!has_content_type)
    {
        response << "Content-Type: " << content_type_value << "\r\n";
    }

    if (!has_content_length)
    {
        response << "Content-Length: " << body.length() << "\r\n";
    }

    response << "Server: webserv/1.0\r\n";

    response << "Connection: close\r\n";

    response << "\r\n";

    response << body;

    return response.str();
}

std::string CGI::generateErrorResponse(int code, const std::string &message)
{
    std::ostringstream response;
    std::string body = "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head><title>CGI Error</title></head>\n"
                       "<body>\n"
                       "<h1>Error " +
                       toString(code) +
                       "</h1>\n"
                       "<p>" +
                       message +
                       "</p>\n"
                       "</body>\n"
                       "</html>\n";
    response << "HTTP/1.1 " << code << " " << getStatusMessage(code) << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    return (response.str());
}

std::string CGI::getDirectoryPath(const std::string &file_path)
{
    size_t last_slash;

    last_slash = file_path.find_last_of('/');
    if (last_slash != std::string::npos)
    {
        return (file_path.substr(0, last_slash));
    }
    return ("");
}

std::string CGI::toUpperSnakeCase(const std::string &str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '-')
        {
            result += '_';
        }
        else
        {
            result += std::toupper(str[i]);
        }
    }
    return (result);
}

std::string CGI::toString(int num)
{
    std::ostringstream oss;
    oss << num;
    return (oss.str());
}

std::string CGI::getStatusMessage(int code)
{
    switch (code)
    {
    case 200:
        return ("OK");
    case 400:
        return ("Bad Request");
    case 404:
        return ("Not Found");
    case 500:
        return ("Internal Server Error");
    case 504:
        return ("Gateway Timeout");
    default:
        return ("Error");
    }
}
