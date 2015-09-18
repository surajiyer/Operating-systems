/* 
 * Operating Systems  (2INC0)  Practical Assignment
 * Interprocess Communication
 *
 * S.S. Iyer (866094)
 * Xiang Teng (851499)
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
#include <string.h>
#include <errno.h>          // for perror()
#include <unistd.h>         // for getpid()
#include <mqueue.h>         // for mq-stuff
#include <time.h>           // for time()
#include <complex.h>

#include "settings.h"
#include "common.h"

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time(NULL) % getpid());
        first_call = false;
    }
    usleep (random () % t);
}

static double 
complex_dist (complex a)
{
    // distance of vector 'a'
    // (in fact the square of the distance is computed...)
    double re, im;
    
    re = __real__ a;
    im = __imag__ a;
    return ((re * re) + (im * im));
}

static int 
mandelbrot_point (double x, double y)
{
    int     k;
    complex z;
	complex c;
    
	z = x + y * I;     // create a complex number 'z' from 'x' and 'y'
	c = z;

	for (k = 0; (k < MAX_ITER) && (complex_dist (z) < INFINITY); k++)
	{
	    z = z * z + c;
    }
    
    //                                    2
    // k >= MAX_ITER or | z | >= INFINITY
    
    return (k);
}


int main (int argc, char * argv[])
{
    // TODO:
    //  * open the two message queues (whose names are provided in the arguments)
    //  * repeatingly:
    //      - read from a message queue the new job to do
    //      - wait a random amount of time (e.g. rsleep(10000);)
    //      - do that job (use mandelbrot_point() if you like)
    //      - write the results to a message queue
    //    until there are no more jobs to do
    //  * close the message queues

	// check if the user has started this program with valid arguments
	if (argc != 3)
	{
		fprintf(stderr, "%s: %d arguments:\n", argv[0], argc);
		int i;
		for (i = 1; i < argc; i++)
		{
			fprintf(stderr, "     '%s'\n", argv[i]);
		}
		exit(1);
	} // else parse the arguments
	
	char *mq_req_name, *mq_res_name;
	MQ_REQUEST_MESSAGE	req;
	MQ_RESPONSE_MESSAGE	res;
	mqd_t mq_fd_request;
	mqd_t mq_fd_response;

	// parse the arguments for message queue names
	mq_req_name = (char*) malloc(strlen(argv[1])*sizeof(char));
	mq_res_name = (char*) malloc(strlen(argv[2])*sizeof(char));
	strcpy(mq_req_name, argv[1]);
	strcpy(mq_res_name, argv[2]);
	
	// open the two message queues
	mq_fd_request = mq_open(mq_req_name, O_RDONLY);
	mq_fd_response = mq_open(mq_res_name, O_WRONLY);

	// Continuosly recieve from the farmer queue
	do {
		// read the message queue and store it in the request message
		mq_receive(mq_fd_request, (char*) &req, sizeof(MQ_REQUEST_MESSAGE), NULL);
		res.y = req.y;
		
		// if y > -1, compute the pixel color, compute the pixel color
		if (res.y > -1) {
			int x;
			for (x = 0; x < X_PIXEL; x++) {
				res.pix_color[x] = mandelbrot_point(X_LOWERLEFT+(x*STEP), Y_LOWERLEFT+(res.y*STEP));
			}
		}
		
		// send the response
		if(res.y > -1 || RUN_PARALLEL) 
			mq_send(mq_fd_response, (char*) &res, sizeof(MQ_RESPONSE_MESSAGE), 0);
		
		// Sleep the thread to create variation in the process interleaving
		rsleep(10000);
	}while(res.y > -1);

	mq_close(mq_fd_response);
	mq_close(mq_fd_request);

    return (0);
}
