
#include "../inc/HttpRequest.hpp"

HttpRequest::HttpRequest() : method_(""),
                             uri_(""),
                             http_version_(""),
                             headers_(),
                             body_(""),
                             is_valid_(false),
                             body_length_(0) {}

bool HttpRequest::parse(const std::string &data)
{
    clear();

    if (data.empty())
    {
        return false;
    }

    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string::npos)
    {

        header_end = data.find("\n\n");
        if (header_end == std::string::npos)
        {
            return false;
        }
    }

    std::string header_section = data.substr(0, header_end);
    std::string body_section = "";

    if (header_end + 4 < data.length())
    {
        body_section = data.substr(header_end + 4);
    }
    else if (header_end + 2 < data.length())
    {
        body_section = data.substr(header_end + 2);
    }

    size_t first_line_end = header_section.find("\r\n");
    if (first_line_end == std::string::npos)
    {
        first_line_end = header_section.find("\n");
    }

    if (first_line_end == std::string::npos)
    {
        return false;
    }

    std::string request_line = header_section.substr(0, first_line_end);
    if (!parseRequestLine(request_line))
    {
        return false;
    }

    std::string headers_only = header_section.substr(first_line_end);
    if (!headers_only.empty())
    {
        if (headers_only[0] == '\r')
            headers_only = headers_only.substr(2);
        else if (headers_only[0] == '\n')
            headers_only = headers_only.substr(1);

        if (!parseHeaders(headers_only))
        {
            return false;
        }
    }

    if (!body_section.empty())
    {
        parseBody(body_section);
    }

    is_valid_ = true;
    return true;
}

bool HttpRequest::parseRequestLine(const std::string &line)
{
    std::istringstream iss(line);

    if (!(iss >> method_ >> uri_ >> http_version_))
    {
        return false;
    }

    std::transform(method_.begin(), method_.end(), method_.begin(), ::toupper);

    if (method_ != "GET" && method_ != "POST" && method_ != "DELETE" &&
        method_ != "PUT" && method_ != "HEAD" && method_ != "OPTIONS")
    {
        return false;
    }

    if (http_version_ != "HTTP/1.0" && http_version_ != "HTTP/1.1")
    {
        return false;
    }

    return true;
}

bool HttpRequest::parseHeaders(const std::string &headers)
{
    std::istringstream stream(headers);
    std::string line;

    while (std::getline(stream, line))
    {

        if (!line.empty() && line[line.length() - 1] == '\r')
        {
            line = line.substr(0, line.length() - 1);
        }

        if (line.empty())
        {
            break;
        }

        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
        {
            continue;
        }

        std::string name = toLowerCase(trim(line.substr(0, colon_pos)));
        std::string value = trim(line.substr(colon_pos + 1));

        if (!name.empty() && !value.empty())
        {
            headers_[name] = value;
        }
    }

    std::map<std::string, std::string>::const_iterator it = headers_.find("content-length");
    if (it != headers_.end())
    {
        std::istringstream length_stream(it->second);
        length_stream >> body_length_;
    }

    return true;
}

void HttpRequest::parseBody(const std::string &body)
{
    body_ = body;

    if (body_length_ > 0 && body_.length() > body_length_)
    {
        body_ = body_.substr(0, body_length_);
    }
}

void HttpRequest::clear()
{
    method_ = "";
    uri_ = "";
    http_version_ = "";
    headers_.clear();
    body_ = "";
    is_valid_ = false;
    body_length_ = 0;
}

const std::string &HttpRequest::getMethod() const
{
    return method_;
}

const std::string &HttpRequest::getUri() const
{
    return uri_;
}

const std::string &HttpRequest::getHttpVersion() const
{
    return http_version_;
}

const std::string &HttpRequest::getBody() const
{
    return body_;
}

std::string HttpRequest::getHeader(const std::string &name) const
{
    std::string lower_name = toLowerCase(name);
    std::map<std::string, std::string>::const_iterator it = headers_.find(lower_name);

    if (it != headers_.end())
    {
        return it->second;
    }
    return "";
}

const std::map<std::string, std::string> &HttpRequest::getHeaders() const
{
    return headers_;
}

std::string HttpRequest::trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return "";
    }

    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string HttpRequest::toLowerCase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
