NAME 	=	webserv
DIR_SRC	=	src
DIR_OBJ	=	obj
DIR_INC	=	inc
SRC		=	HttpServer.cpp HttpResponse.cpp HttpRequest.cpp Logger.cpp
OBJ		=	$(SRC:.cpp=.o)
INC		=	-I $(DIR_INC)
LOG		=	server.log
CC		=	c++
CFLAGS	=	-Wall -Wextra -Werror -std=c++98
RM		=	rm -f

OBJ		:=	$(addprefix $(DIR_OBJ)/, $(OBJ))
SRC		:=	$(addprefix $(DIR_SRC)/, $(SRC))

all: 			$(NAME)

$(NAME):		$(OBJ)
			$(CC) $(CFLAGS) $(INC) -o $(NAME) $(OBJ)

$(DIR_OBJ)/%.o:	$(DIR_SRC)/%.cpp
			@mkdir -p $(DIR_OBJ)
			$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
			$(RM) $(OBJ)
			$(RM) -r $(DIR_OBJ)

fclean:			clean
			$(RM) $(NAME) $(LOG)

re: 			fclean all

.PHONY: all clean fclean re
