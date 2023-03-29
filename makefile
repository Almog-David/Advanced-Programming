CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic

myshell: myshell.o
	$(CC) $(CFLAGS) -o myshell myshell.o

myshell.o: myshell.c
	$(CC) $(CFLAGS) -c myshell.c

clean:
	rm -f *.o *.a *.so myshell
