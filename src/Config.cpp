
#include "../inc/Config.hpp"
#include "../inc/utils.hpp"

Config::Config(const std::string &file_path) : _servers()
{
    parse(file_path);
}

Config::~Config() {}

const std::vector<ServerConfig> &Config::getServers() const
{
    return _servers;
}

static std::string removeComment(const std::string &line)
{
    size_t pos = line.find('#');
    if (pos != std::string::npos)
    {
        return line.substr(0, pos);
    }
    return line;
}

namespace
{
    std::vector<std::string> parseMethods(const std::string &methods_str)
    {
        std::vector<std::string> methods;
        std::istringstream iss(methods_str);
        std::string method;

        while (iss >> method)
        {
            methods.push_back(method);
        }

        if (methods.empty())
        {
            methods.push_back("GET");
        }

        return methods;
    }
}

void Config::parse(const std::string &config_file)
{
    _valid = true;
    std::ifstream file(config_file.c_str());
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open config file: " + config_file);
    }

    std::string line;
    while (std::getline(file, line))
    {
        line = removeComment(line);
        line = trim(line);

        if (line.empty())
            continue;

        if (line == "server {" || line == "server{")
        {
            bool closed = false;
            parseServerBlock(file, &closed);
            if (!closed)
            {
                throw std::runtime_error("Unclosed server block");
            }
        }
        else if (line.find("server") == 0)
        {
            std::string next_line;
            if (std::getline(file, next_line))
            {
                next_line = trim(removeComment(next_line));
                if (next_line == "{")
                {
                    bool closed = false;
                    parseServerBlock(file, &closed);
                    if (!closed)
                    {
                        throw std::runtime_error("Unclosed server block");
                    }
                }
            }
        }
    }

    if (_servers.empty())
    {
        throw std::runtime_error("No server blocks found in configuration");
    }
}

void Config::parseServerBlock(std::istream &file, bool *closed)
{
    _servers.push_back(ServerConfig());
    ServerConfig &server = _servers.back();

    server._port = 80;
    server._host = "0.0.0.0";
    server._client_max_body_size = 1048576;

    std::string line;
    LocationConfig current_location;
    bool in_location = false;
    bool location_open = false;

    while (std::getline(file, line))
    {

        line = removeComment(line);
        line = trim(line);
        if (line.empty())
            continue;

        if (line == "}")
        {
            if (in_location)
            {
                location_open = false;
                if (current_location._root.empty() && !server._locations.empty())
                {
                    for (size_t i = 0; i < server._locations.size(); i++)
                    {
                        if (server._locations[i]._path == "/")
                        {
                            current_location._root = server._locations[i]._root;
                            break;
                        }
                    }
                }
                server._locations.push_back(current_location);
                current_location = LocationConfig();
                in_location = false;
            }
            else
            {
                *closed = true;
                break;
            }
            continue;
        }
        std::istringstream iss(line);
        std::string directive;
        iss >> directive;

        if (directive == "server")
        {
            _valid = false;
            std::cerr << "Server closing tag missing" << std::endl;
            break;
        }
        if (directive == "location")
        {
            if (in_location)
            {
                _valid = false;
                std::cerr << "Location tag not clossed" << std::endl;
                server._locations.push_back(current_location);
            }

            current_location = LocationConfig();
            std::string path;
            iss >> path;

            if (!path.empty() && path[path.length() - 1] == '{')
            {
                path = path.substr(0, path.length() - 1);
                path = trim(path);
            }

            current_location._path = path;
            in_location = true;

            std::string rest;
            std::getline(iss, rest);
            rest = trim(rest);

            if (rest != "{" && rest != "")
            {
                std::getline(file, line);
                line = removeComment(line);
                line = trim(line);
                if (line != "{")
                {
                    throw std::runtime_error("Expected '{' after location directive");
                }
            }
            location_open = true;
        }
        else
        {
            std::string value;
            std::getline(iss, value);
            value = trim(value);

            if (in_location)
            {
                parseLocationDirective(current_location, directive, value);
            }
            else
            {
                parseServerDirective(server, directive, value);
            }
        }
    }

    if (in_location)
    {
        server._locations.push_back(current_location);
    }

    bool has_default = false;
    for (size_t i = 0; i < server._locations.size(); i++)
    {
        if (server._locations[i]._path == "/")
        {
            has_default = true;
            break;
        }
    }

    if (!has_default && !server._locations.empty())
    {
        LocationConfig default_loc;
        default_loc._path = "/";
        default_loc._root = "www";
        default_loc._index_file = "index.html";
        default_loc._directory_listing = false;
        default_loc._allowed_methods.push_back("GET");
        server._locations.insert(server._locations.begin(), default_loc);
    }
}

void Config::parseServerDirective(ServerConfig &server, const std::string &directive,
                                  const std::string &value)
{
    if (directive == "listen")
    {
        server._port = atoi(value.c_str());
    }
    else if (directive == "host")
    {
        server._host = value;
    }
    else if (directive == "server_name")
    {
        std::istringstream iss(value);
        std::string name;
        while (iss >> name)
        {
            server._server_names.push_back(name);
        }
    }
    else if (directive == "client_max_body_size")
    {
        std::istringstream iss(value);
        size_t size;
        iss >> size;
        server._client_max_body_size = size;
    }
    else if (directive == "error_page")
    {
        std::istringstream iss(value);
        int code;
        std::string path;
        iss >> code >> path;
        server._error_pages[code] = path;
    }
    else if (directive == "root")
    {
    }
    else if (directive == "index")
    {
    }
}

void Config::parseLocationDirective(LocationConfig &location, const std::string &directive,
                                    const std::string &value)
{
    if (directive == "root")
    {
        location._root = value;
    }
    else if (directive == "index")
    {
        location._index_file = value;
    }
    else if (directive == "autoindex")
    {
        location._directory_listing = (value == "on");
    }
    else if (directive == "allow")
    {
        location._allowed_methods = parseMethods(value);
    }
    else if (directive == "allow_methods")
    {
        location._allowed_methods = parseMethods(value);
    }
    else if (directive == "cgi_path")
    {
        location._cgi_path = value;

        if (!fileExists(value))
        {
            std::cerr << "Warning: CGI interpreter not found: " << value << std::endl;
            std::cerr << "Please verify the path or install the interpreter." << std::endl;
        }
    }
    else if (directive == "cgi_extension" || directive == "cgi_ext")
    {
        location._cgi_extension = value;
    }
    else if (directive == "upload_path")
    {
        location._upload_path = value;
    }
    else if (directive == "return")
    {
        location._redirect = value;
    }
    else if (directive == "client_max_body_size")
    {
    }
    else if (directive == "alias")
    {

        location._root = value;
    }
}
