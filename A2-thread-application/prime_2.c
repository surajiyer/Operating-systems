/* 
 * Operating Systems  (2INC0)   Practical Assignment
 * Threaded Application
 *
 * STUDENT_NAME_1 (S.S.Iyer)
 * STUDENT_NAME_2 (X.Teng)
 *
 * Grading:
 * Students who hand in clean code that fully satisfies the minimum requirements will get an 8. 
 * ”Extra” steps can lead to higher marks because we want students to take the initiative. 
 * Extra steps can be, for example, in the form of measurements added to your code, a formal 
 * analysis of deadlock freeness etc.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>     // for usleep()
#include <time.h>       // for time()
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include "prime.h"

// use to ignore unused warning
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
typedef unsigned long long  MY_TYPE;
#define MY_TYPE_SIZE 64
#define BUFFER_SIZE NELEMS(buffer)

// create a bitmask where bit at position n is set
#define BITMASK(n)          (((MY_TYPE) 1) << (n))

// check if bit n in v is set
#define BIT_IS_SET(v,n)     (((v) & BITMASK(n)) == BITMASK(n))

// set bit n in v
#define BIT_SET(v,n)        ((v) =  (v) |  BITMASK(n))

// clear bit n in v
#define BIT_CLEAR(v,n)      ((v) =  (v) & ~BITMASK(n))

// declare the total number of available threads
static pthread_t* threadID;
static int activeThreadCount;

// declare mutexes and initialize them
static pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t conditionLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;

// declare synchronization condition variable
static pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;

// The number to strike out
static int number;

static void rsleep (int t);

static void init() {
	// Set all bits to 1's
	int i;
	for(i=0; i < NELEMS(buffer); i++) {
		buffer[i] = ~0;
	}
	
	// Set certain bits to 0 (we don't want to include them)
	BIT_CLEAR(buffer[0], 0);
	BIT_CLEAR(buffer[0], 1);
	for(i = (NROF_SIEVE+1)%MY_TYPE_SIZE; i < MY_TYPE_SIZE; i++) {
		BIT_CLEAR(buffer[BUFFER_SIZE-1], i);
	}
}

static void* strike_out(void* argument) {
	int n = (int) argument;
	long long j;
	
	// Lock the buffer and work on it
	pthread_mutex_lock (&bufferLock);
	if(BIT_IS_SET(buffer[n / MY_TYPE_SIZE], n % MY_TYPE_SIZE)) {
		for(j=n*n; j <= NROF_SIEVE; j+=n) {
			BIT_CLEAR(buffer[j / MY_TYPE_SIZE], j % MY_TYPE_SIZE);
		}
	}
	pthread_mutex_unlock (&bufferLock);
	
	pthread_mutex_lock (&exitLock);
	activeThreadCount--;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock (&exitLock);
	
	return NULL;
}

static void print_primes() {
	int n;
	for(n=2; n <= NROF_SIEVE; n++) {
		if(BIT_IS_SET(buffer[n / MY_TYPE_SIZE], n % MY_TYPE_SIZE)) {
			printf("%d\n", n); 
		}
	}
}

int main (void)
{
    // TODO: start threads generate all primes between 2 and NROF_SIEVE and output the results
    // (see thread_malloc_free_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)
	// Initialize certain things
	init();
	
	// Strike out bits
	activeThreadCount = 0;
	number = 2;
	while(number <= sqrt(NROF_SIEVE) && activeThreadCount < NROF_THREADS) {
		fprintf(stderr, "Before malloc thread create\n");
		threadID = (pthread_t*) malloc(sizeof(pthread_t));
		fprintf(stderr, "Before thread create\n");
		if(pthread_create(&threadID[0], NULL, strike_out, &number)) {
			fprintf(stderr, "Error creating thread for n = %d\n", number);
			return 1;
		}
		fprintf(stderr, "Thread created\n");
		// detach the thread so we don't need to manually join it
		pthread_detach(threadID[0]);
		fprintf(stderr, "Thread detached\n");
		rsleep(100);
		number++;
		activeThreadCount++;
	}
	
	while(number <= sqrt(NROF_SIEVE)) {
		// wait for a thread to finish with condition variable
		pthread_mutex_lock (&conditionLock);
		pthread_cond_wait(&cond, &conditionLock);
		
		// check which thread finished and restart with new task
		while(number <= sqrt(NROF_SIEVE) && activeThreadCount < NROF_THREADS) {
			// Restart the thread with a new number
			threadID = malloc(sizeof(pthread_t));
			if(pthread_create(&threadID[0], NULL, strike_out, &number)) {
				fprintf(stderr, "Error creating thread for n = %d\n", number);
				return 1;
			}
			// detach the thread so we don't need to manually join it
			pthread_detach(threadID[0]);
			
			// increment the number
			number++;
			
			// Set its exit flag back to false
			activeThreadCount++;
		}
		
		// unlock condition variable mutex bufferLock
		pthread_mutex_unlock (&conditionLock);
	}
	
	// Destroy all mutexes and the condition variable
	pthread_mutex_destroy(&bufferLock);
	pthread_mutex_destroy(&conditionLock);
	pthread_mutex_destroy(&exitLock);
	pthread_cond_destroy(&cond);
	
	// Print all the prime numbers
	print_primes();

    return (0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid());
        first_call = false;
    }
    usleep (random () % t);
}
