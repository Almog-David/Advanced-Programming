#define _POSIX_C_SOURCE 200809L
#include "threadpool.h"
static volatile int threads_keepalive;
static volatile int threads_on_hold;


/* ========================== THREADPOOL ============================ */


struct thpool_* thpool_init(int num_threads){

	threads_on_hold = 0;
	threads_keepalive = 1;

	if (num_threads < 0){
		perror("thpool_init(): num_threads < 0\n");
	    exit(1);
	}

	/* Make new thread pool */
	Pthpool_ thpool_p;
	thpool_p = (Pthpool_)calloc(1,sizeof(thpool_));
	if (thpool_p == NULL){
			perror("file calloc");
			exit(1);
	}

	thpool_p->num_threads_alive = 0;
	thpool_p->num_threads_working = 0;

	/* Initialise the job queue */
	if (jobqueue_init(&thpool_p->jobqueue) == -1){
			perror("file calloc");
			exit(1);
		return NULL;
	}

	/* Make threads in pool */
	thpool_p->threads = (Pthread*)calloc(num_threads , sizeof(Pthread ));
	if (thpool_p->threads == NULL){
		perror("file calloc");
		jobqueue_destroy(&thpool_p->jobqueue);
		free(thpool_p);
		return NULL;
	}

	pthread_mutex_init(&(thpool_p->thcount_lock), NULL);
	pthread_cond_init(&thpool_p->threads_all_idle, NULL);

	/* Thread init */
	int n;
	for (n=0; n<num_threads; n++){
		thread_init(thpool_p, &thpool_p->threads[n], n);
	}

	/* Wait for threads to initialize */
	while (thpool_p->num_threads_alive != num_threads) {}

	return thpool_p;
}


int thpool_add_work(thpool_* thpool_p, void (*function_p)(void*), void* arg_p){
	job* newjob;

	newjob=(struct job*)malloc(sizeof(struct job));
	if (newjob==NULL){
		perror("file calloc");
		exit(1);
	}

	/* add function and argument */
	newjob->function=function_p;
	newjob->arg=arg_p;

	/* add job to queue */
	jobqueue_push(&thpool_p->jobqueue, newjob);

	return 0;
}



void thpool_wait(Pthpool_ thpool_p){
	pthread_mutex_lock(&thpool_p->thcount_lock);
	while (thpool_p->jobqueue.len || thpool_p->num_threads_working) {
		pthread_cond_wait(&thpool_p->threads_all_idle, &thpool_p->thcount_lock);
	}
	pthread_mutex_unlock(&thpool_p->thcount_lock);
}



void thpool_destroy(Pthpool_ thpool_p){
	/* No need to destory if it's NULL */
	if (thpool_p == NULL) return ;

	volatile int threads_total = thpool_p->num_threads_alive;

	/* End each thread 's infinite loop */
	threads_keepalive = 0;

	/* Give one second to kill idle threads */
	double TIMEOUT = 1.0;
	time_t start, end;
	double tpassed = 0.0;
	time (&start);
	while (tpassed < TIMEOUT && thpool_p->num_threads_alive){
		bsem_post_all(thpool_p->jobqueue.has_jobs);
		time (&end);
		tpassed = difftime(end,start);
	}

	/* Poll remaining threads */
	while (thpool_p->num_threads_alive){
		bsem_post_all(thpool_p->jobqueue.has_jobs);
		sleep(1);
	}

	/* Job queue cleanup */
	jobqueue_destroy(&thpool_p->jobqueue);
	/* Deallocs */
	int n;
	for (n=0; n < threads_total; n++){
		thread_destroy(thpool_p->threads[n]);
	}
	free(thpool_p->threads);
	free(thpool_p);
}



void thpool_pause(Pthpool_ thpool_p) {
	int n;
	for (n=0; n < thpool_p->num_threads_alive; n++){
		pthread_kill(thpool_p->threads[n]->pthread, SIGUSR1);
	}
}


void thpool_resume(Pthpool_ thpool_p) {
    (void)thpool_p;

	threads_on_hold = 0;
}


int thpool_num_threads_working(thpool_* thpool_p)
{
	return thpool_p->num_threads_working;
}





/* ============================ THREAD ============================== */


int thread_init (Pthpool_ thpool_p, Pthread* thread_p, int id)
{

	*thread_p = (Pthread)calloc(1,sizeof(struct thread));
	if (!*thread_p)
	{
		perror("file calloc");
		exit(1);
	}

	(*thread_p)->thpool_p = thpool_p;
	(*thread_p)->id = id;

	pthread_create(&(*thread_p)->pthread, NULL, (void * (*)(void *)) thread_do, (*thread_p));
	pthread_detach((*thread_p)->pthread);
	return 0;
}


void thread_hold(int sig_id)
 {
    (void)sig_id;
	threads_on_hold = 1;
	while (threads_on_hold){
		sleep(1);
	}
}


void* thread_do(Pthread thread_p)
{

	/* Set thread name for profiling and debuging */
	char thread_name[32] = {0};
	snprintf(thread_name, 32, "%d", thread_p->id);

	prctl(PR_SET_NAME, thread_name);


	/* Assure all threads have been created before starting serving */
	thpool_* thpool_p = thread_p->thpool_p;

	/* Register signal handler */
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = thread_hold;
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		perror("thread_do(): cannot handle SIGUSR1");
	    exit(1);
	}

	/* Mark thread as alive (initialized) */
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_alive += 1;
	pthread_mutex_unlock(&thpool_p->thcount_lock);

	while(threads_keepalive){

		bsem_wait(thpool_p->jobqueue.has_jobs);

		if (threads_keepalive){

			pthread_mutex_lock(&thpool_p->thcount_lock);
			thpool_p->num_threads_working++;
			pthread_mutex_unlock(&thpool_p->thcount_lock);

			/* Read job from queue and execute it */
			void (*func_buff)(void*);
			void*  arg_buff;
			job* job_p = jobqueue_pull(&thpool_p->jobqueue);
			if (job_p) {
				func_buff = job_p->function;
				arg_buff  = job_p->arg;
				func_buff(arg_buff);
				free(job_p);
			}

			pthread_mutex_lock(&thpool_p->thcount_lock);
			thpool_p->num_threads_working--;
			if (!thpool_p->num_threads_working) {
				pthread_cond_signal(&thpool_p->threads_all_idle);
			}
			pthread_mutex_unlock(&thpool_p->thcount_lock);

		}
	}
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_alive --;
	pthread_mutex_unlock(&thpool_p->thcount_lock);

	return NULL;
}


void thread_destroy (Pthread thread_p)
{
	free(thread_p);
}









