CC=gcc
Wall=-Wall -pthread  -lpthread
OBJ=threadpool.o job.o binary_semaphore.o
all: coder

coder: codec.h main.c $(OBJ)
	$(CC) $(Wall) main.c ./libCodec.so -o coder $(OBJ)

binary_semaphore.o: binary_semaphore.c binary_semaphore.h
	$(CC) -c binary_semaphore.c

job.o: job.c job.h
	$(CC) -c job.c

threadpool.o: threadpool.c threadpool.h
	$(CC) -c threadpool.c

.PHONY: clean all
clean:
	rm -f coder *.o
