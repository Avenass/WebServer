
#pragma once

#include "ServerConfig.hpp"
#include <string>
#include <vector>

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cctype>

class Config
{
  public:
	Config(const std::string &file_path);
	~Config();
	const std::vector<ServerConfig> &getServers() const;
	const ServerConfig &findServerConfigForRequest(const std::string &host,
		int port) const;
	void parse(const std::string &file_path);
	static bool checkLastChar(char c, std::string line);
	bool _valid;

  private:
	std::vector<ServerConfig> _servers;
	Config();
	Config(const Config &other);
	Config &operator=(const Config &other);
	void parseServerBlock(std::istream &file, bool *closed);
	void parseLocationBlock(std::istream &file, ServerConfig &server,
		bool *closed);
	void parseDirective(ServerConfig &server, LocationConfig &location,
		const std::string &directive, const std::string &value);
	void parseServerDirective(ServerConfig &server,
		const std::string &directive, const std::string &value);
	void parseLocationDirective(LocationConfig &location,
		const std::string &directive, const std::string &value);
};
