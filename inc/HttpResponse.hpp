#pragma once

#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

class HttpResponse
{
  public:
	HttpResponse();
	std::string serialize() const;
	void setError(int code, const std::string &message);
	void addHeader(const std::string &key, const std::string &value);
	int getStatusCode() const;
	const std::string &getBody() const;
	const std::string &getConnectionType() const;
	const std::map<std::string, std::string> &getHeaders() const;
	void setStatusCode(int code);
	void setBody(const std::string &content);
	void setConnectionType(const std::string &type);

  private:
	int status_code_;
	std::string connection_type_;
	std::map<std::string, std::string> headers_;
	std::string body_;
	static std::map<int, std::string> initStatusCodes();
	static const std::map<int, std::string> status_messages_;
	static std::string toLowerCase(const std::string &str);
};
