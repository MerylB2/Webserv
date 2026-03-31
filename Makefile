# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: cmetee-b <cmetee-b@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/08/05 14:08:36 by cmetee-b          #+#    #+#              #
#    Updated: 2026/03/31 15:23:13 by cmetee-b         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Colors
RED		= \033[0;31m
GREEN	= \033[1;32m
YELLOW	= \033[0;33m
BLUE	= \033[1;34m
PURPLE	= \033[0;35m
CYAN	= \033[1;36m
WHITE	= \033[0;37m
RESET	= \033[0m

# Program name
NAME = webserv

# Detect OS
UNAME_S := $(shell uname -s)

# Compiler and flags (C++98)
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 
## -MMD -MP = pour obliger les .o (dependances) de compiler avec tous les fihiers modifs meme les hpp
INCLUDES = -I./includes -I./includes/core -I./includes/http -I./includes/config -I./includes/cgi -I./includes/utils

# OS-specific settings
ifeq ($(UNAME_S), Linux)
    OS_FLAG = -D__LINUX__
    # Linux uses epoll
    POLL_METHOD = poll
endif
ifeq ($(UNAME_S), Darwin)
    OS_FLAG = -D__MACOS__
    # macOS uses kqueue
    POLL_METHOD = kqueue
endif

CXXFLAGS += $(OS_FLAG)

# Directories
SRCDIR = srcs
OBJDIR = obj

# Source files (only existing files for now)
# TODO: Add other files as they are implemented
SRCFILES = main.cpp \
		   core/Server.cpp \
		   core/Client.cpp \
		   config/ConfigParser.cpp \
		   http/Request.cpp \
		   http/Response.cpp \
		   http/Router.cpp \
		   http/Autoindex.cpp \
		   http/SessionManager.cpp \
		   http/SessionHandler.cpp \
		   cgi/CGIHandler.cpp \
		   cgi/CGIAsync.cpp

# Object files
SOURCES = $(addprefix $(SRCDIR)/, $(SRCFILES))
OBJECTS = $(addprefix $(OBJDIR)/, $(SRCFILES:.cpp=.o))

# Rules
all: $(NAME)

$(NAME): $(OBJECTS)
	@echo "$(CYAN)Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(NAME)
	@echo ""
	@echo "$(BLUE)  ██╗    ██╗███████╗██████╗ ███████╗███████╗██████╗ ██╗   ██╗$(RESET)"
	@echo "$(BLUE)  ██║    ██║██╔════╝██╔══██╗██╔════╝██╔════╝██╔══██╗██║   ██║$(RESET)"
	@echo "$(CYAN)  ██║ █╗ ██║█████╗  ██████╔╝███████╗█████╗  ██████╔╝██║   ██║$(RESET)"
	@echo "$(CYAN)  ██║███╗██║██╔══╝  ██╔══██╗╚════██║██╔══╝  ██╔══██╗╚██╗ ██╔╝$(RESET)"
	@echo "$(GREEN)  ╚███╔███╔╝███████╗██████╔╝███████║███████╗██║  ██║ ╚████╔╝ $(RESET)"
	@echo "$(GREEN)   ╚══╝╚══╝ ╚══════╝╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝  ╚═══╝  $(RESET)"
	@echo ""
	@echo "$(GREEN)        ✓ Webserv compiled successfully!$(RESET)"
	@echo "$(PURPLE)        OS: $(UNAME_S) | Poll method: $(POLL_METHOD)$(RESET)"
	@echo ""

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	@rm -rf $(OBJDIR)
	@echo "$(GREEN)✓ Object files cleaned!$(RESET)"

fclean: clean
	@echo "$(RED)Cleaning executable...$(RESET)"
	@rm -f $(NAME)
	@rm -f test_http
	@echo "$(GREEN)✓ Everything cleaned!$(RESET)"

re: fclean all

# Test rule - compile and run HTTP tests
test: test_http
	@echo "$(CYAN)Running HTTP tests...$(RESET)"
	@./test_http

test_http: tests/test_http.cpp srcs/http/Request.cpp srcs/http/Response.cpp srcs/http/Router.cpp
	@echo "$(YELLOW)Compiling tests...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o test_http
	@echo "$(GREEN)✓ Tests compiled!$(RESET)"

# Debug rule
debug: CXXFLAGS += -g3 -fsanitize=address
debug: re

# Valgrind rule
valgrind: $(NAME)
	@echo "$(CYAN)Running $(NAME) with valgrind...$(RESET)"
	@valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes ./$(NAME) config/default.conf

# Help
help:
	@echo "$(CYAN)Available targets:$(RESET)"
	@echo "  $(YELLOW)all$(RESET)         - Build the program"
	@echo "  $(YELLOW)clean$(RESET)       - Remove object files"
	@echo "  $(YELLOW)fclean$(RESET)      - Remove object files and executable"
	@echo "  $(YELLOW)re$(RESET)          - Rebuild everything"
	@echo "  $(YELLOW)test$(RESET)        - Compile and run HTTP tests"
	@echo "  $(YELLOW)debug$(RESET)       - Build with debug flags and sanitizer"
	@echo "  $(YELLOW)valgrind$(RESET)    - Run with valgrind"
	@echo "  $(YELLOW)help$(RESET)        - Show this help"

# Phony targets
.PHONY: all clean fclean re test debug valgrind help

# Silent mode
.SILENT:
