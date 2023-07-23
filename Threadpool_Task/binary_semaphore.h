
#ifndef SEMAPHORE
#define SEMAPHORE

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/prctl.h>

typedef struct bsem
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int v;
} bsem, *Pbsem;

/* Init semaphore to 1 or 0 */
void bsem_init(bsem *bsem_p, int value);

/* Reset semaphore to 0 */
void bsem_reset(Pbsem bsem_p);

/* Post to at least one thread */
void bsem_post(Pbsem bsem_p);

/* Post to all threads */
void bsem_post_all(Pbsem bsem_p);

/* Wait on semaphore until semaphore has value 0 */
void bsem_wait(Pbsem bsem_p);

#endif