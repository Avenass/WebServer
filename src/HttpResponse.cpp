
#include "../inc/HttpResponse.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

const std::map<int, std::string> HttpResponse::status_messages_ = HttpResponse::initStatusCodes();

HttpResponse::HttpResponse() : status_code_(200),
                               connection_type_("close"),
                               headers_(),
                               body_("")
{

    headers_["server"] = "webserv/1.0";
    headers_["content-type"] = "text/html";
}

std::map<int, std::string> HttpResponse::initStatusCodes()
{
    std::map<int, std::string> codes;

    codes[100] = "Continue";
    codes[101] = "Switching Protocols";

    codes[200] = "OK";
    codes[201] = "Created";
    codes[202] = "Accepted";
    codes[204] = "No Content";

    codes[301] = "Moved Permanently";
    codes[302] = "Found";
    codes[303] = "See Other";
    codes[304] = "Not Modified";
    codes[307] = "Temporary Redirect";
    codes[308] = "Permanent Redirect";

    codes[400] = "Bad Request";
    codes[401] = "Unauthorized";
    codes[403] = "Forbidden";
    codes[404] = "Not Found";
    codes[405] = "Method Not Allowed";
    codes[408] = "Request Timeout";
    codes[409] = "Conflict";
    codes[411] = "Length Required";
    codes[413] = "Payload Too Large";
    codes[414] = "URI Too Long";
    codes[415] = "Unsupported Media Type";

    codes[500] = "Internal Server Error";
    codes[501] = "Not Implemented";
    codes[502] = "Bad Gateway";
    codes[503] = "Service Unavailable";
    codes[504] = "Gateway Timeout";
    codes[505] = "HTTP Version Not Supported";

    return codes;
}

std::string HttpResponse::serialize() const
{
    std::ostringstream response;

    response << "HTTP/1.1 " << status_code_ << " ";

    std::map<int, std::string>::const_iterator it = status_messages_.find(status_code_);
    if (it != status_messages_.end())
    {
        response << it->second;
    }
    else
    {
        response << "Unknown";
    }
    response << "\r\n";

    for (std::map<std::string, std::string>::const_iterator header_it = headers_.begin();
         header_it != headers_.end(); ++header_it)
    {
        response << header_it->first << ": " << header_it->second << "\r\n";
    }

    if (!body_.empty())
    {
        response << "content-length: " << body_.length() << "\r\n";
    }

    response << "connection: " << connection_type_ << "\r\n";

    response << "\r\n";

    if (!body_.empty())
    {
        response << body_;
    }

    return response.str();
}

void HttpResponse::setError(int code, const std::string &message)
{
    status_code_ = code;
    headers_["content-type"] = "text/html";

    std::ostringstream error_body;
    error_body << "<!DOCTYPE html>\n";
    error_body << "<html>\n";
    error_body << "<head><title>Error " << code << "</title></head>\n";
    error_body << "<body>\n";
    error_body << "<h1>Error " << code << "</h1>\n";
    error_body << "<p>" << message << "</p>\n";
    error_body << "<hr>\n";
    error_body << "<p><i>webserv/1.0</i></p>\n";
    error_body << "</body>\n";
    error_body << "</html>\n";

    body_ = error_body.str();
}

void HttpResponse::addHeader(const std::string &key, const std::string &value)
{
    std::string lower_key = toLowerCase(key);
    headers_[lower_key] = value;
}

int HttpResponse::getStatusCode() const
{
    return status_code_;
}

const std::string &HttpResponse::getBody() const
{
    return body_;
}

const std::string &HttpResponse::getConnectionType() const
{
    return connection_type_;
}

const std::map<std::string, std::string> &HttpResponse::getHeaders() const
{
    return headers_;
}

void HttpResponse::setStatusCode(int code)
{
    status_code_ = code;
}

void HttpResponse::setBody(const std::string &content)
{
    body_ = content;
}

void HttpResponse::setConnectionType(const std::string &type)
{
    connection_type_ = type;
}

std::string HttpResponse::toLowerCase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
