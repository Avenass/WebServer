
#include "../inc/ServerConfig.hpp"

ServerConfig::ServerConfig() : _port(80),
                               _host("0.0.0.0"),
                               _server_names(),
                               _error_pages(),
                               _client_max_body_size(0),
                               _locations() {}

ServerConfig::~ServerConfig() {}

ServerConfig::ServerConfig(const ServerConfig &other) : _port(other._port),
                                                        _host(other._host),
                                                        _server_names(other._server_names),
                                                        _error_pages(other._error_pages),
                                                        _client_max_body_size(other._client_max_body_size),
                                                        _locations(other._locations) {}

ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    if (this != &other)
    {
        _port = other._port;
        _host = other._host;
        _server_names = other._server_names;
        _error_pages = other._error_pages;
        _client_max_body_size = other._client_max_body_size;
        _locations = other._locations;
    }
    return *this;
}

int ServerConfig::getPort() const
{
    return _port;
}

void ServerConfig::setPort(int port)
{
    _port = port;
}

const LocationConfig &ServerConfig::findLocationForRequest(const std::string &uri_path) const
{

    if (_locations.empty())
    {

        static LocationConfig default_location;
        static bool initialized = false;

        if (!initialized)
        {
            default_location._path = "/";
            default_location._root = "www";
            default_location._index_file = "index.html";
            default_location._directory_listing = false;
            default_location._allowed_methods.push_back("GET");
            initialized = true;
        }

        return default_location;
    }

    const LocationConfig *best_match = &_locations[0];
    size_t best_length = 0;

    for (std::vector<LocationConfig>::const_iterator it = _locations.begin();
         it != _locations.end(); ++it)
    {
        const std::string &loc_path = it->_path;
        const size_t loc_len = loc_path.length();

        if ((uri_path == loc_path) ||
            (uri_path.compare(0, loc_len, loc_path) == 0 &&
             (loc_path[loc_path.size() - 1] == '/' ||
              (uri_path.length() > loc_len && uri_path[loc_len] == '/'))))
        {
            if (loc_len > best_length)
            {
                best_length = loc_len;
                best_match = &(*it);
            }
        }
    }

    return *best_match;
}
