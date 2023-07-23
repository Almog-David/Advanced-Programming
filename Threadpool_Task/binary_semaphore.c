#include "binary_semaphore.h"

void bsem_init(Pbsem bsem_p, int value)
{
	if (value < 0 || value > 1)
	{
		perror("bsem_init(): Binary semaphore can take only values 1 or 0");
		exit(1);
	}
	pthread_mutex_init(&(bsem_p->mutex), NULL);
	pthread_cond_init(&(bsem_p->cond), NULL);
	bsem_p->v = value;
}

void bsem_reset(Pbsem bsem_p)
{
	bsem_init(bsem_p, 0);
}

void bsem_post(Pbsem bsem_p)
{
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v = 1;
	pthread_cond_signal(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}

void bsem_post_all(Pbsem bsem_p)
{
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v = 1;
	pthread_cond_broadcast(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}

void bsem_wait(Pbsem bsem_p)
{
	pthread_mutex_lock(&bsem_p->mutex);
	while (bsem_p->v != 1)
	{
		pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex);
	}
	bsem_p->v = 0;
	pthread_mutex_unlock(&bsem_p->mutex);
}