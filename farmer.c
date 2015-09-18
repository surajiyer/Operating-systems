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
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>    
#include <unistd.h>         // for execlp
#include <mqueue.h>         // for mq

#include "settings.h"
#include "output.h"
#include "common.h"

// use to ignore unused functions warning
#pragma GCC diagnostic ignored "-Wunused-function"

/* get teh attributes of the given queue */
static void 
getattr (mqd_t mq_fd)
{
    struct mq_attr      attr;
    int                 rtnval;
    
    rtnval = mq_getattr (mq_fd, &attr);
    if (rtnval == -1)
    {
        perror ("mq_getattr() failed");
        exit (1);
    }
    printf("%d: mqdes=%d max=%ld size=%ld nrof=%ld\n", getpid(), 
		mq_fd, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
}

/* Output a line in the image */
static void
output(MQ_RESPONSE_MESSAGE *res) {
	int x;
	for (x = 0; x < X_PIXEL; x++) {
		output_draw_pixel(x, res->y, res->pix_color[x]);
	}
}

/* Start the worker children */
static void
start_workers(char *n1, char *n2) {
	pid_t	processID;
	int 	i;
	for (i = 0; i < NROF_WORKERS; ++i) {
		if ((processID = fork()) < 0) {
		perror("fork() failed");
			exit(1);
		} else if (processID == 0) {
			execlp ("./worker", "worker", n1, n2, NULL);
			exit(0);	// this should never be reached
		}
	}
}

/* kill the worker children and close and unlink the message queues */
static void
shut_down(char *n1, char *n2, mqd_t fa, mqd_t wo) {
	// send kill message to shut down all worker processes
	int i;
	
	// send kill messages if parallel processing. Normal processing
	// already sends kill messages before to workers before calling
	// shut_down()
	if(RUN_PARALLEL) {
		MQ_REQUEST_MESSAGE req;
		req.y = -1;
		for(i = 0; i < NROF_WORKERS; ++i) {
			mq_send(fa, (char*) &req, sizeof(MQ_REQUEST_MESSAGE), 0);
		}
	}
	
	// wait for all the children to close
	for(i = 0; i < NROF_WORKERS+(RUN_PARALLEL?1:0); ++i) {
		wait(NULL);
	}
	
	// close the queues and deallocate its memory
	mq_close(wo);
	mq_close(fa);
	mq_unlink(n1);
	mq_unlink(n2);
}

int main (int argc, char * argv[])
{
    if (argc != 1) {
        fprintf(stderr, "%s: invalid arguments\n", argv[0]);
    }
    
    // start the output
	output_init();
    
    // TODO:
    //  * create the message queues & the children
    //  * do the farming (use output_draw_pixel() for the coloring
    //  * wait until the children have been stopped
    //  * clean up the message queues

    // Important notice: make sure that your message queues contain your
    // student name and the process id (to ensure uniqueness during testing)
    
	mqd_t			mq_fd_request;			/* Farmer request queue */
	mqd_t			mq_fd_response;			/* Worker response queue */
	struct mq_attr	attr;					/* Attributes of the message queues */
	int 			n = NROF_WORKERS;
	
	/* create names for the message queues */
	char *mq_name1, *mq_name2;
	mq_name1 = (char*) malloc(30*sizeof(char));
	mq_name2 = (char*) malloc(30*sizeof(char));
	sprintf(mq_name1, "/mq_request_%s_%d", STUDENT_NAME_1, getpid());
	sprintf(mq_name2, "/mq_response_%s_%d", STUDENT_NAME_2, getpid());

	// open the worker (request) and farmer (response) queues
	attr.mq_maxmsg = MQ_MAX_MESSAGES;
	attr.mq_msgsize = sizeof(MQ_REQUEST_MESSAGE);
	mq_fd_request = mq_open(mq_name1, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
	attr.mq_msgsize = sizeof(MQ_RESPONSE_MESSAGE);
	mq_fd_response = mq_open(mq_name2, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);
	
	// read the response simultaeneously on a another new child process
	if(RUN_PARALLEL) {
		pid_t	processID;
		if ((processID = fork()) < 0) {
			perror("fork() failed");
			exit(1);
		} else if (processID == 0) {
			MQ_RESPONSE_MESSAGE	res;	/* response message */
			while (n > 0) {
				// read the result and store it in the response message
				mq_receive(mq_fd_response, (char*) &res, sizeof(MQ_RESPONSE_MESSAGE), NULL);
				
				// Continue receiving messages from the response queue 
				// until every worker is inactive.
				if (res.y < 0) --n;
				else { // else if it is a normal response message
					// draw the pixel in the output
					output(&res);
				}
			}
		} else {
			// request message
			MQ_REQUEST_MESSAGE	req;
			
			// start the worker children
			start_workers(mq_name1, mq_name2);
			
			// send computation requests to worker
			int y;
			for (y = 0; y < Y_PIXEL; y++) {
				req.y = y;
				mq_send(mq_fd_request, (char*) &req, sizeof(MQ_REQUEST_MESSAGE), 0);
			}
			
			shut_down(mq_name1, mq_name2, mq_fd_request, mq_fd_response);			
		}
	} else { // send and receieve on the same parent process
		MQ_REQUEST_MESSAGE	req;			/* request message */
		MQ_RESPONSE_MESSAGE	res;			/* response message */
		
		// start the worker children
		start_workers(mq_name1, mq_name2);
		
		// send computation requests to worker
		int y;
		while(n > 0) {
			if (y < Y_PIXEL) {
				mq_getattr(mq_fd_request, &attr);
				if(attr.mq_curmsgs < attr.mq_maxmsg) {
					req.y = y;
					mq_send(mq_fd_request, (char*) &req, sizeof(MQ_REQUEST_MESSAGE), 0);
					y++;
				}
			} else {
				req.y = -1;
				mq_getattr(mq_fd_request, &attr);
				if(attr.mq_curmsgs < attr.mq_maxmsg) {
					mq_send(mq_fd_request, (char*) &req, sizeof(MQ_REQUEST_MESSAGE), 0);
					--n;
				}
			}
			mq_getattr(mq_fd_response, &attr);
			if(attr.mq_curmsgs > 0) {
				mq_receive(mq_fd_response, (char*) &res, sizeof(MQ_RESPONSE_MESSAGE), NULL);	// read the result and store it in the response message
				output(&res);																	// draw the pixel in the output
			}
		}
		
		// receiveing any remaining messages
		mq_getattr(mq_fd_response, &attr);
		while(attr.mq_curmsgs > 0) {
			mq_receive(mq_fd_response, (char*) &res, sizeof(MQ_RESPONSE_MESSAGE), NULL);	// read the result and store it in the response message
			output(&res);																	// draw the pixel in the output
			mq_getattr(mq_fd_response, &attr);
		}
		
		shut_down(mq_name1, mq_name2, mq_fd_request, mq_fd_response);
	}
	
	// close the output
	output_end();
    
    return (0);
}
