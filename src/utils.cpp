
#include "../inc/utils.hpp"

bool isDirectory(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return S_ISDIR(info.st_mode);
}

bool isFile(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return S_ISREG(info.st_mode);
}

bool fileExists(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

bool isReadable(const std::string &path)
{
    return access(path.c_str(), R_OK) == 0;
}

bool isWritable(const std::string &path)
{
    return access(path.c_str(), W_OK) == 0;
}

bool isExecutable(const std::string &path)
{
    return access(path.c_str(), X_OK) == 0;
}

size_t getFileSize(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return 0;
    }
    return info.st_size;
}

std::string readFile(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return "";
    }

    std::ostringstream content;
    content << file.rdbuf();
    file.close();

    return content.str();
}

bool writeFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    file << content;
    file.close();

    return true;
}

std::string generateDirectoryListing(const std::string &path, const std::string &uri)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
    {
        return "";
    }

    std::ostringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "  <title>Index of " << uri << "</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: monospace; margin: 20px; }\n";
    html << "    h1 { font-size: 24px; }\n";
    html << "    table { border-collapse: collapse; width: 100%; }\n";
    html << "    th, td { padding: 8px 15px; text-align: left; }\n";
    html << "    th { background-color: #f0f0f0; border-bottom: 2px solid #ddd; }\n";
    html << "    tr:hover { background-color: #f5f5f5; }\n";
    html << "    a { text-decoration: none; color: #0066cc; }\n";
    html << "    a:hover { text-decoration: underline; }\n";
    html << "    .dir { font-weight: bold; }\n";
    html << "    .size { text-align: right; font-family: monospace; }\n";
    html << "    .date { font-family: monospace; }\n";
    html << "  </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <h1>Index of " << uri << "</h1>\n";
    html << "  <hr>\n";
    html << "  <table>\n";
    html << "    <thead>\n";
    html << "      <tr>\n";
    html << "        <th>Name</th>\n";
    html << "        <th>Size</th>\n";
    html << "        <th>Last Modified</th>\n";
    html << "      </tr>\n";
    html << "    </thead>\n";
    html << "    <tbody>\n";

    if (uri != "/")
    {
        html << "      <tr>\n";
        html << "        <td><a href=\"../\" class=\"dir\">../</a></td>\n";
        html << "        <td class=\"size\">-</td>\n";
        html << "        <td class=\"date\">-</td>\n";
        html << "      </tr>\n";
    }

    struct dirent *entry;
    std::vector<std::string> entries;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name[0] == '.')
        {
            continue;
        }

        entries.push_back(name);
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());

    for (std::vector<std::string>::const_iterator it = entries.begin();
         it != entries.end(); ++it)
    {

        std::string entry_path = path + "/" + *it;
        struct stat info;

        if (stat(entry_path.c_str(), &info) == 0)
        {
            html << "      <tr>\n";

            html << "        <td>";
            if (S_ISDIR(info.st_mode))
            {
                html << "<a href=\"" << *it << "/\" class=\"dir\">" << *it << "/</a>";
            }
            else
            {
                html << "<a href=\"" << *it << "\">" << *it << "</a>";
            }
            html << "</td>\n";

            html << "        <td class=\"size\">";
            if (S_ISDIR(info.st_mode))
            {
                html << "-";
            }
            else
            {
                html << formatFileSize(info.st_size);
            }
            html << "</td>\n";

            html << "        <td class=\"date\">" << formatTime(info.st_mtime) << "</td>\n";
            html << "      </tr>\n";
        }
    }

    html << "    </tbody>\n";
    html << "  </table>\n";
    html << "  <hr>\n";
    html << "  <address>webserv/1.0 Server</address>\n";
    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

std::string formatFileSize(size_t size)
{
    std::ostringstream oss;

    if (size < 1024)
    {
        oss << size << " B";
    }
    else if (size < 1024 * 1024)
    {
        oss << std::fixed << std::setprecision(1) << (size / 1024.0) << " KB";
    }
    else if (size < 1024 * 1024 * 1024)
    {
        oss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << " MB";
    }
    else
    {
        oss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0 * 1024.0)) << " GB";
    }

    return oss.str();
}

std::string formatTime(time_t timestamp)
{
    char buffer[80];
    struct tm *timeinfo = localtime(&timestamp);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

std::string getMimeType(const std::string &path)
{
    size_t dot_pos = path.find_last_of('.');

    if (dot_pos == std::string::npos)
    {
        return "application/octet-stream";
    }

    std::string ext = path.substr(dot_pos + 1);

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "html" || ext == "htm")
        return "text/html";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "json")
        return "application/json";
    if (ext == "xml")
        return "application/xml";
    if (ext == "txt")
        return "text/plain";
    if (ext == "pdf")
        return "application/pdf";

    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "png")
        return "image/png";
    if (ext == "gif")
        return "image/gif";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "ico")
        return "image/x-icon";
    if (ext == "webp")
        return "image/webp";

    if (ext == "mp3")
        return "audio/mpeg";
    if (ext == "wav")
        return "audio/wav";
    if (ext == "mp4")
        return "video/mp4";
    if (ext == "webm")
        return "video/webm";
    if (ext == "ogg")
        return "audio/ogg";
    if (ext == "avi")
        return "video/x-msvideo";

    if (ext == "zip")
        return "application/zip";
    if (ext == "tar")
        return "application/x-tar";
    if (ext == "gz")
        return "application/gzip";
    if (ext == "rar")
        return "application/x-rar-compressed";

    if (ext == "doc")
        return "application/msword";
    if (ext == "docx")
        return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (ext == "xls")
        return "application/vnd.ms-excel";
    if (ext == "xlsx")
        return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (ext == "ppt")
        return "application/vnd.ms-powerpoint";
    if (ext == "pptx")
        return "application/vnd.openxmlformats-officedocument.presentationml.presentation";

    return "application/octet-stream";
}

std::string urlDecode(const std::string &str)
{
    std::string result;

    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.length())
        {
            int hex;
            std::istringstream hex_stream(str.substr(i + 1, 2));
            if (hex_stream >> std::hex >> hex)
            {
                result += static_cast<char>(hex);
                i += 2;
            }
            else
            {
                result += str[i];
            }
        }
        else if (str[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += str[i];
        }
    }

    return result;
}

std::string urlEncode(const std::string &str)
{
    std::ostringstream result;
    result.fill('0');
    result << std::hex;

    for (size_t i = 0; i < str.length(); ++i)
    {
        unsigned char c = str[i];

        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            result << c;
        }
        else if (c == ' ')
        {
            result << '+';
        }
        else
        {
            result << '%' << std::setw(2) << static_cast<int>(c);
        }
    }

    return result.str();
}

std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return "";
    }

    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string toLowerCase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string toUpperCase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(str);

    while (std::getline(stream, token, delimiter))
    {
        if (!token.empty())
        {
            tokens.push_back(token);
        }
    }

    return tokens;
}

std::string join(const std::vector<std::string> &strings, const std::string &delimiter)
{
    std::string result;

    for (size_t i = 0; i < strings.size(); ++i)
    {
        if (i > 0)
        {
            result += delimiter;
        }
        result += strings[i];
    }

    return result;
}
