CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -g

stree: stree.o
	$(CC) $(CFLAGS) -o stree stree.o

stree.o: stree.c
	$(CC) $(CFLAGS) -c stree.c

clean:
	rm -f *.o *.a *.so stree
