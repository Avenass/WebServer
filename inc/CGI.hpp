#pragma once

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <cctype>
#include <sstream>
#include <vector>
#include <algorithm>

#include "HttpRequest.hpp"
#include "LocationConfig.hpp"
#include <ctime>
#include <map>
#include <string>
#include <vector>

class CGI
{
public:
	CGI(const HttpRequest &request, const LocationConfig &location);
	~CGI();
	std::string execute(const std::string &script_path);
	void setTimeout(time_t seconds)
	{
		timeout_seconds_ = seconds;
	}

private:
	const HttpRequest &request_;
	const LocationConfig &location_;
	std::map<std::string, std::string> env_map_;
	std::vector<char *> env_vars_;
	time_t timeout_seconds_;
	void setupEnvironment();
	void buildEnvArray();
	void executeCGIChild(const std::string &script_path, int pipe_in[2],
						 int pipe_out[2]);
	std::string handleCGIParent(pid_t pid, int pipe_in[2], int pipe_out[2]);
	std::string parseCGIOutput(const std::string &raw_output);
	std::string generateErrorResponse(int code, const std::string &message);
	std::string getDirectoryPath(const std::string &file_path);
	std::string toUpperSnakeCase(const std::string &str);
	std::string toString(int num);
	std::string getStatusMessage(int code);
	CGI(const CGI &);
	CGI &operator=(const CGI &);
};
