CC=gcc
CFLAGS=-Wall -Wextra -Werror
IFLAGS=-I.
LFLAGS=

SRCS=				\
main.c				\
ping.c				\
terminal_output.c	\
parsing.c

OBJS=$(SRCS:.c=.o)

NAME=ft_ping

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all