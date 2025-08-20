
#include "../inc/Config.hpp"
#include "../inc/WebServer.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>

void signalHandler(int signum)
{
	if (signum == SIGPIPE)
	{

		return;
	}
	if (signum == SIGINT || signum == SIGTERM)
	{
		std::cout << "\n\n🛑 Shutting down webserv..." << std::endl;
		exit(0);
	}
}

void printUsage(const char *program)
{
	std::cout << "Usage: " << program << " [config_file]" << std::endl;
	std::cout << "  config_file: Path to configuration file (default: configs/default.conf)" << std::endl;
	std::cout << "\nExample:" << std::endl;
	std::cout << "  " << program << " configs/default.conf" << std::endl;
}

void printServerInfo(const std::vector<ServerConfig> &servers)
{
	std::cout << "\n"
			  << std::string(60, '=') << std::endl;
	std::cout << "            WEBSERV CONFIGURATION LOADED" << std::endl;
	std::cout << std::string(60, '=') << std::endl;
	for (size_t i = 0; i < servers.size(); ++i)
	{
		const ServerConfig &s = servers[i];
		std::cout << "\n📌 Server #" << (i + 1) << std::endl;
		std::cout << std::string(40, '-') << std::endl;
		std::cout << "   Host: " << s._host << std::endl;
		std::cout << "   Port: " << s._port << std::endl;
		if (!s._server_names.empty())
		{
			std::cout << "   Server Names: ";
			for (size_t j = 0; j < s._server_names.size(); ++j)
			{
				if (j > 0)
					std::cout << ", ";
				std::cout << s._server_names[j];
			}
			std::cout << std::endl;
		}
		std::cout << "   Max Body Size: " << s._client_max_body_size << " bytes" << std::endl;
		if (!s._error_pages.empty())
		{
			std::cout << "   Error Pages: ";
			for (std::map<int,
						  std::string>::const_iterator it = s._error_pages.begin();
				 it != s._error_pages.end(); ++it)
			{
				if (it != s._error_pages.begin())
					std::cout << ", ";
				std::cout << it->first;
			}
			std::cout << std::endl;
		}
		std::cout << "\n   📁 Locations:" << std::endl;
		for (size_t j = 0; j < s._locations.size(); ++j)
		{
			const LocationConfig &loc = s._locations[j];
			std::cout << "      [" << loc._path << "]" << std::endl;
			std::cout << "        Root: " << loc._root << std::endl;
			if (!loc._allowed_methods.empty())
			{
				std::cout << "        Methods: ";
				for (size_t k = 0; k < loc._allowed_methods.size(); ++k)
				{
					if (k > 0)
						std::cout << " ";
					std::cout << loc._allowed_methods[k];
				}
				std::cout << std::endl;
			}
			if (!loc._index_file.empty())
			{
				std::cout << "        Index: " << loc._index_file << std::endl;
			}
			if (loc._directory_listing)
			{
				std::cout << "        Autoindex: on" << std::endl;
			}
			if (!loc._cgi_path.empty())
			{
				std::cout << "        CGI: " << loc._cgi_path << " (" << loc._cgi_extension << ")" << std::endl;
			}
			if (!loc._upload_path.empty())
			{
				std::cout << "        Upload: " << loc._upload_path << std::endl;
			}
			if (!loc._redirect.empty())
			{
				std::cout << "        Redirect: " << loc._redirect << std::endl;
			}
		}
	}
	std::cout << "\n"
			  << std::string(60, '=') << std::endl;
}

int main(int argc, char *argv[])
{

	signal(SIGPIPE, signalHandler);
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	if (argc < 2) 
	{
		std::cerr << "Error: No configuration file" << std::endl;
		printUsage(argv[0]);
		return (EXIT_FAILURE);
	}
	if (argc > 2)
	{
		std::cerr << "Error: Too many arguments" << std::endl;
		printUsage(argv[0]);
		return (EXIT_FAILURE);
	}

	std::string config_file = "configs/default.conf";
	if (argc == 2)
	{
		if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
		{
			printUsage(argv[0]);
			return (EXIT_SUCCESS);
		}
		config_file = argv[1];
	}
	try
	{
		config_file = argv[1];
		std::cout << "\n🔧 Loading configuration from: " << config_file << std::endl;

		Config config(config_file);
		if (!config._valid)
		{
			std::cerr << "Invalid config file " << std::endl;
			return (EXIT_FAILURE);
		}
		const std::vector<ServerConfig> &servers = config.getServers();
		if (servers.empty())
		{
			throw std::runtime_error("No servers configured");
		}

		printServerInfo(servers);

		WebServer server(servers);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "\n❌ Error: " << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
