CC=gcc
CFLAGS=-g -Wall -fsanitize=address -std=c99

mysh: mysh.c
	$(CC) $(CFLAGS) $^ -o $@