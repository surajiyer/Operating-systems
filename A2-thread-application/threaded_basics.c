/* 
 * Operating Systems (2INC0) Practical 
 * 2009/2015 (c) Joris Geurts
 *
 * This program contains some C constructs which might be useful for
 * the Threaded Application assignment of 2INC0
 *
 *
 *      I M P O R T A N T    M E S S A G E :
 *      ====================================
 *
 * For readability reasons, this program does not check the return value of 
 * the POSIX calls.
 * This is not a good habit.
 * Always check the return value of a system call (you never know if the disk is
 * is full, or if we run out of other system resources)!
 * Possible construction:
 *
 *      rtnval = <posix-call>();
 *      if (rtnval == <error-value-according-documentation>)
 *      {
 *          perror ("<your-message>");
 *          exit (1);
 *      }
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

// use to ignore unused functions warning
#pragma GCC diagnostic ignored "-Wunused-function"

typedef unsigned long long  MY_TYPE;

// create a bitmask where bit at position n is set
#define BITMASK(n)          (((MY_TYPE) 1) << (n))

// check if bit n in v is set
#define BIT_IS_SET(v,n)     (((v) & BITMASK(n)) == BITMASK(n))

// set bit n in v
#define BIT_SET(v,n)        ((v) =  (v) |  BITMASK(n))

// clear bit n in v
#define BIT_CLEAR(v,n)      ((v) =  (v) & ~BITMASK(n))

// declare a mutex, and it is initialized as well
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*-------------------------------------------------------------------------*/


static void
bit_test (void)
{
    MY_TYPE     v;
    
    // set all bits to 1
    v = ~0;
    
    printf ("v (all 1's): %llx\n", v);  // note the '%llx' for printing long-long-integers
    printf("hi: %lld\n", ~BITMASK(4));
	BIT_CLEAR (v, 1);
    printf ("v (one 0)  : %llx\n", v);
    if (BIT_IS_SET (v, 4))
    {
        printf ("bit %d is set\n", 4);
    }
    else
    {
        printf ("bit %d is NOT set\n", 4);
    }
    if (BIT_IS_SET (v, 63))
    {
        printf ("bit %d is set\n", 63);
    }
    else
    {
        printf ("bit %d is NOT set\n", 63);
    }
    BIT_SET (v, 4);
    printf ("v (all 1's): %llx\n", v);
    printf ("\n");
}    
    
    
static void *
my_thread (void * arg)
{
    int *   argi; 
    int     i;      
    int *   rtnval;
    
    argi = (int *) arg;     // proper casting before dereferencing (could also be done in one statement)
    i = *argi;              // get the integer value of the pointer
    free (arg);             // we retrieved the integer value, so now the pointer can be deleted
    
    printf ("        %lx: thread started; parameter=%d\n", pthread_self(), i);
    
    sleep (1);
    
    // a return value to be given back to the calling main-thread
    rtnval = malloc (sizeof (int)); // will be freed by the parent-thread
    *rtnval = 42;           // assign an arbitrary value...
    return (rtnval);        // you can also use pthread_exit(rtnval);
}


static void
thread_test (void)
{
    int *       parameter;
    int *       rtnval;
    pthread_t   thread_id;
    
    // parameter to be handed over to the thread
    parameter = malloc (sizeof (int));  // memory will be freed by the child-thread
    *parameter = 73;        // assign an arbitrary value...
    
    printf ("%lx: starting thread ...\n", pthread_self());
    pthread_create (&thread_id, NULL, my_thread, parameter);
    
    sleep (3);
    
    // wait for the thread, and we are interested in the return value
    pthread_join (thread_id, (void **) &rtnval);
    
    printf ("%lx: thread ready; return value=%d\n", pthread_self(), *rtnval);
    free (rtnval);          // free the memory thas has been allocated by the thread
    printf ("\n");
}


static void *
my_mutex_thread (void * arg)
{
    printf ("        %lx: thread start; wanting to enter CS...\n", pthread_self());
    pthread_mutex_lock (&mutex);
    
    // Here we are in the critical section (protected by a mutex)
    printf ("        %lx: thread entered CS\n", pthread_self());
    sleep (10);
    printf ("        %lx: thread leaves CS\n", pthread_self());
    
    pthread_mutex_unlock (&mutex);
    
    return (NULL);
}


static void
thread_mutex_test (void)
{
    pthread_t   my_threads[2];

    printf ("%lx: starting thread-3 ...\n", pthread_self());
    pthread_create (&my_threads[0], NULL, my_mutex_thread, NULL);
    sleep (3);
    printf ("%lx: starting thread-5 ...\n", pthread_self());
    pthread_create (&my_threads[1], NULL, my_mutex_thread, NULL);
    
    // wait for threads, but we are not interested in the return value 
    pthread_join (my_threads[0], NULL);
    pthread_join (my_threads[1], NULL); 
    
    printf ("%lx: threads ready\n", pthread_self());
    printf ("\n");
}


/*-------------------------------------------------------------------------*/

int main (void)
{
    bit_test();
    //thread_test();
    //thread_mutex_test();
    
    return (0);
}

