
#pragma once

#include <string>
#include <vector>

class LocationConfig
{
  public:
	LocationConfig();
	~LocationConfig();
	LocationConfig(const LocationConfig &other);
	LocationConfig &operator=(const LocationConfig &other);
	
	std::string _path;
	std::string _root;
	std::vector<std::string> _allowed_methods;
	std::string _index_file;
	bool _directory_listing;
	std::string _cgi_path;
	std::string _cgi_extension;
	std::string _upload_path;
	std::string _redirect;
	size_t _client_max_body_size;
};
