NAME :=  webserv client
INCLUDE_DIR :=
SERVER_FILES := Main.cpp \
				Consts.cpp \
				WebServer.cpp \
				ServerKey.cpp \
				Server.cpp \
				LocationTrie.cpp \
				Location.cpp \
				StringUtils.cpp \
				FileUtils.cpp \
				Connection.cpp \
				HttpRequest.cpp \
				HttpResponse.cpp \
				CGI.cpp
CLIENT_FILES := client.cpp

CFLAGS := -Wall -Wextra -Werror -std=c++98
ifdef DEBUG
    CFLAGS += -DDEBUG=1 -g
endif

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
	LEAK_TOOL := leaks -atExit --
else
	LEAK_TOOL := valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes
endif
IFLAGS :=
SERVER_OBJ := $(SERVER_FILES:.cpp=.o)
CLIENT_OBJ := $(CLIENT_FILES:.cpp=.o)
CC := c++


.PHONY: all clean fclean re webserv client leak

all: $(NAME)


webserv: $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(SERVER_OBJ) -o webserv

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $(CLIENT_OBJ) -o client

%.o : %.cpp
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	rm -rf $(SERVER_OBJ) $(CLIENT_OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

leak: $(NAME)
	$(LEAK_TOOL) ./webserv

