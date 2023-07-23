
#include <time.h>
#include "codec.h"
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include "threadpool.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

int key = 1;
char flag = 0;
int val_index;
int check_index;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

typedef struct data
{
	int ind;
	char val[1025];
} data, *Pdata;

void task(void *arg)
{
	if (flag == 'e')
	{
		encrypt(((Pdata)(arg))->val, key);
	}
	if (flag == 'd')
	{
		decrypt(((Pdata)(arg))->val, key);
	}
	while (true)
	{
		if (((Pdata)(arg))->ind == check_index)
		{
			printf("%s\n", ((Pdata)(arg))->val);
			check_index++;
			free((char *)(arg));
			return;
		}
	}
}

int main(int argc, char *argv[])
{
	check_index = 0;
	val_index = 0;

	int numCPU = sysconf(_SC_NPROCESSORS_ONLN);

	key = atoi(argv[1]);
	printf("The key is: %i \n", key);
	flag = argv[2][1];

	threadpool thpool = thpool_init(numCPU);

	while (!feof(stdin))
	{
		Pdata dat = (Pdata)calloc(1, sizeof(data));
		if (!dat)
		{
			perror("file calloc");
			exit(1);
		}
		dat->ind = val_index;
		fgets(dat->val, 1024, stdin);
		thpool_add_work(thpool, task, (void *)(dat));
		val_index++;
	}

	thpool_wait(thpool);
	thpool_destroy(thpool);

	return 0;
}
