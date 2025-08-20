#pragma once

#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

class HttpRequest
{
  public:
	HttpRequest();
	bool parse(const std::string &data);
	const std::string &getMethod() const;
	const std::string &getUri() const;
	const std::string &getHttpVersion() const;
	const std::string &getBody() const;
	std::string getHeader(const std::string &name) const;
	const std::map<std::string, std::string> &getHeaders() const;

  private:
	std::string method_;
	std::string uri_;
	std::string http_version_;
	std::map<std::string, std::string> headers_;
	std::string body_;
	bool is_valid_;
	size_t body_length_;
	void clear();
	bool parseRequestLine(const std::string &line);
	bool parseHeaders(const std::string &headers);
	void parseBody(const std::string &body);
	static std::string trim(const std::string &str);
	static std::string toLowerCase(const std::string &str);
};
