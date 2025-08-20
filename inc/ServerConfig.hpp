#pragma once

#include "LocationConfig.hpp"
#include <map>
#include <string>
#include <vector>

class ServerConfig
{
  public:
	ServerConfig();
	~ServerConfig();
	ServerConfig(const ServerConfig &other);
	ServerConfig &operator=(const ServerConfig &other);
	int getPort() const;
	void setPort(int port);
	int _port;
	std::string _host;
	std::vector<std::string> _server_names;
	std::map<int, std::string> _error_pages;
	size_t _client_max_body_size;
	std::vector<LocationConfig> _locations;
	const LocationConfig &findLocationForRequest(const std::string &uri_path) const;
};
