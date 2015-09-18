/* 
 * Operating Systems  (2INC0)  Practical Assignment
 * Interprocess Communication
 *
 * S.S. Iyer (866094)
 * Xiang Teng (851499)
 * 
 * Contains definitions which are commonly used by the farmer and the workers
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include "settings.h"

// setting for student id
#define STUDENT_NAME_1 	"SurajIyer"
#define STUDENT_NAME_2 	"XiangTeng"
#define STUDENT_NR_1	866094
#define STUDENT_NR_2	851499

// Enable/disable parallel processing. Switch to 1 to enable parallel 
// processing. Switch to 0 for normal processing.
#define RUN_PARALLEL	0

// TODO: put your definitions of the data structures here
typedef struct
{
    int	y;	// y coordinate of the line of the image
} MQ_REQUEST_MESSAGE;

typedef struct
{
	int y;					// y coordinate of the line of the image
    int	pix_color[X_PIXEL];	// 24-bit color of the pixel
} MQ_RESPONSE_MESSAGE;

#endif

