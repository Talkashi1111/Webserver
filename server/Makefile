NAME :=  server client
INCLUDE_DIR :=
SERVER_FILES := server.cpp
CLIENT_FILES := client.cpp
CFLAGS := -Wall -Wextra -Werror -std=c++98 -g
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
	LEAK_TOOL := leaks -atExit --
	HOSTNAME := $(shell whoami)
else
	LEAK_TOOL := valgrind --leak-check=full
endif
IFLAGS :=
SERVER_OBJ := $(SERVER_FILES:.cpp=.o)
CLIENT_OBJ := $(CLIENT_FILES:.cpp=.o)
CC := c++


.PHONY: all clean fclean re server client leak

all: $(NAME)


server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(SERVER_OBJ) -o server

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
	$(LEAK_TOOL) ./server ./client

