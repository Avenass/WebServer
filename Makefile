NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRCDIR = src
INCDIR = inc
OBJDIR = obj

SOURCES = CGI.cpp \
          Config.cpp \
          HttpRequest.cpp \
          HttpResponse.cpp \
          LocationConfig.cpp \
          main.cpp \
          ServerConfig.cpp \
          utils.cpp \
          WebServer.cpp

# cambie aca para que los objetos se formen en otra carpeta.
OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

# es mi mismo makefile de siempre al final.
GREEN = \033[0;32m
YELLOW = \033[0;33m
RED = \033[0;31m
NC = \033[0m

all: $(NAME)

$(NAME): $(OBJECTS)
	@echo "$(YELLOW)Linking $(NAME)...$(NC)"
	@$(CXX) $(OBJECTS) -o $(NAME)
	@echo "$(GREEN)✓ $(NAME) compiled successfully!$(NC)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	@echo "$(YELLOW)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files...$(NC)"
	@rm -rf $(OBJDIR)
	@echo "$(GREEN)✓ Object files removed$(NC)"

fclean: clean
	@echo "$(RED)Removing $(NAME)...$(NC)"
	@rm -f $(NAME)
	@echo "$(GREEN)✓ $(NAME) removed$(NC)"

re: fclean all

# A partir de aca, lo pimpeo la AI.
dirs:
	@echo "$(YELLOW)Creating directory structure...$(NC)"
	@mkdir -p www/error www/uploads www/cgi-bin www/public www/api configs
	@echo "$(GREEN)✓ Directories created$(NC)"

# Crear archivos de ejemplo
examples: dirs
	@echo "$(YELLOW)Creating example files...$(NC)"
	@echo '<!DOCTYPE html><html><head><title>Welcome</title></head><body><h1>Welcome to Webserv!</h1></body></html>' > www/index.html
	@echo '<!DOCTYPE html><html><head><title>404</title></head><body><h1>404 - Not Found</h1></body></html>' > www/error/404.html
	@echo '<!DOCTYPE html><html><head><title>500</title></head><body><h1>500 - Internal Server Error</h1></body></html>' > www/error/500.html
	@echo '#!/usr/bin/env python3\nprint("Content-Type: text/html\\n")\nprint("<h1>Hello from Python CGI!</h1>")' > www/cgi-bin/test.py
	@chmod +x www/cgi-bin/test.py
	@echo "$(GREEN)✓ Example files created$(NC)"

# la ayuda, boh.
# help:
# 	@echo "$(GREEN)Available targets:$(NC)"
# 	@echo "  all      - Build the webserver"
# 	@echo "  clean    - Remove object files"
# 	@echo "  fclean   - Remove object files and executable"
# 	@echo "  re       - Rebuild everything"
# 	@echo "  dirs     - Create directory structure"
# 	@echo "  examples - Create example files"
# 	@echo "  help     - Show this help message"

.PHONY: all clean fclean re dirs examples help
