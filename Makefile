##############################################################################
#  mandatory program                                                         #
##############################################################################
NAME       := webserv

##############################################################################
#  optional / bonus program                                                  #
##############################################################################
CLIENT     := client

##############################################################################
#  directories and sources                                                   #
##############################################################################
OBJDIR     := objs
SERVER_SRC := Main.cpp Consts.cpp WebServer.cpp ServerKey.cpp Server.cpp \
              LocationTrie.cpp Location.cpp StringUtils.cpp FileUtils.cpp \
              ProcUtils.cpp Connection.cpp HttpRequest.cpp HttpResponse.cpp \
              CGI.cpp
CLIENT_SRC := client.cpp

SERVER_OBJ := $(addprefix $(OBJDIR)/,$(SERVER_SRC:.cpp=.o))
CLIENT_OBJ := $(addprefix $(OBJDIR)/,$(CLIENT_SRC:.cpp=.o))

##############################################################################
#  compiler flags                                                            #
##############################################################################
CXX        := c++
CXXFLAGS   := -Wall -Wextra -Werror -std=c++98
DEBUG      ?= 0
SANITIZE   ?= 0

ifeq ($(DEBUG),1)
  CXXFLAGS += -DDEBUG=1 -g
endif
ifeq ($(SANITIZE),1)
  CXXFLAGS += -fsanitize=address
endif

##############################################################################
#  default target                                                            #
##############################################################################
.PHONY: all
all: $(NAME)

##############################################################################
#  build rules                                                               #
##############################################################################
$(NAME): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CLIENT): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# pattern rule for every object file
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

##############################################################################
#  housekeeping                                                              #
##############################################################################
.PHONY: clean fclean re
clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME) $(CLIENT)

re: fclean all

##############################################################################
#  valgrind / leaks helper (optional)                                        #
##############################################################################
UNAME      := $(shell uname)
LEAK_TOOL  := $(if $(filter $(UNAME),Darwin),leaks -atExit --,\
                valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes)

.PHONY: leak
leak: $(NAME)
	$(LEAK_TOOL) ./$(NAME)
