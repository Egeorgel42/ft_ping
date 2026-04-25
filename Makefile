CC=gcc
CFLAGS=-Wall -Wextra -Werror

SRCS=
ping.c \
terminal_output.c

OBJS=$(SRCS:.c=.o)

NAME=ft_ping

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^