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
	$(CC) $(CFLAGS) $(IFLAGS) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

release: CFLAGS += -O3 -DNDEBUG
release: re

debug: CFLAGS += -g3 -O0 -DDEBUG -fsanitize=address,undefined
debug: LFLAGS += -fsanitize=address,undefined
debug: re

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re release debug