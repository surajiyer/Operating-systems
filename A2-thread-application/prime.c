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

// declare mutexes and initialize them
static pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t numberLock = PTHREAD_MUTEX_INITIALIZER;

// declare the total number of available threads
static pthread_t threads[NROF_THREADS];

// the number to compute
static int number = 2;

static void rsleep (int t);

static void init() {
	// Set all bits to 1's
	int i;
	for(i=0; i < NELEMS(buffer); i++) {
		buffer[i] = ~0;
	}
	
	// Set certain bits to 0 (we don't want to include them)
	// These incude bits in the last buffer corresponding to
	// numbers > NROF_SIEVE
	BIT_CLEAR(buffer[0], 0);
	BIT_CLEAR(buffer[0], 1);
	for(i = (NROF_SIEVE+1)%MY_TYPE_SIZE; i < MY_TYPE_SIZE; i++) {
		BIT_CLEAR(buffer[BUFFER_SIZE-1], i);
	}
}

static void* strike_out(void* arguments) {
	int n;
	MY_TYPE j;
	while(1) {
		// Get the number for which we need to strike out the multiples
		pthread_mutex_lock (&numberLock);
		if(number >= NROF_SIEVE) {
			pthread_mutex_unlock (&numberLock);
			break;
		}
		n = number;
		number++;
		pthread_mutex_unlock (&numberLock);
		
		// Lock the buffer and work on it
		pthread_mutex_lock (&bufferLock);
		if(BIT_IS_SET(buffer[n/MY_TYPE_SIZE], n%MY_TYPE_SIZE)) {
			for(j=(MY_TYPE) n*n; j <= NROF_SIEVE; j+=n) {
				BIT_CLEAR(buffer[j/MY_TYPE_SIZE], j%MY_TYPE_SIZE);
			}
		}
		pthread_mutex_unlock (&bufferLock);
	}
	
	return NULL;
}

static void print_primes() {
	int n;
	for(n=2; n <= NROF_SIEVE; n++) {
		if(BIT_IS_SET(buffer[n/MY_TYPE_SIZE], n%MY_TYPE_SIZE)) {
			printf("%d\n", n);
		}
	}
}

int main (void)
{
    // TODO: start threads generate all primes between 2 and NROF_SIEVE and output the results
    // (see thread_malloc_free_test() and thread_mutex_test() how to use threads and mutexes,
    //  see bit_test() how to manipulate bits in a large integer)
	int threadNr;
	
	// Initialize certain things
	init();
	
	// Create all threads and let them do their work
	threadNr = 0;
	while(threadNr < NROF_THREADS) {
		if(pthread_create(&threads[threadNr], NULL, strike_out, NULL)) {
			fprintf(stderr, "Error creating thread %d\n", threadNr);
			perror("Error creating thread");
			return 1;
		}
		threadNr++;
		rsleep(100);
	}
	
	// Join all the threads back when they finish execution
	threadNr = 0;
	while(threadNr < NROF_THREADS) {
		if(pthread_join(threads[threadNr], NULL)) {
			fprintf(stderr, "Error joining thread\n");
			return 2;
		}
		threadNr++;
	}
	
	// Destroy all mutexes and the condition variable
	pthread_mutex_destroy(&bufferLock);
	pthread_mutex_destroy(&numberLock);
	
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
