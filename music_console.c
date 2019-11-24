/**********************************************************
 *  INCLUDES
 *********************************************************/
#include <vxWorksCommon.h>
#include <vxWorks.h>
#include <stdio.h>
#include <fcntl.h>
#include <ioLib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <selectLib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sioLib.h>
#include <sched.h>
// Uncomment to test on the Arduino with the serial module
//#include "serialcallLib.h"
// Uncomment to test on the PC
#include "audiocallLib.h"

/**********************************************************
 *  CONSTANTS
 *********************************************************/
#define NSEC_PER_SEC 1000000000UL
#define DEV_NAME "/tyCo/1"

// Path of audio file in windows
#define FILE_NAME "host:/c/Users/str/Downloads/practica_2_2020/let_it_be_1bit.raw"


// Uncomment to test on the Arduino
//#define PERIOD_TASK_SEC	0			/* Period of Task   */
//Dividir el periodo entre 8 para aumentar la frecuencia
//#define PERIOD_TASK_NSEC  512000000	/* Period of Task   */
//#define SEND_SIZE 256    /* BYTES */

// Uncomment to test on the PC
#define PERIOD_TASK_SEC	1			/* Period of Task   */
#define PERIOD_TASK_NSEC  0	/* Period of Task   */
#define SEND_SIZE 500    /* BYTES */

/**********************************************************
 *  GLOBALS
 *********************************************************/
int fd_file = -1;
int fd_serie = -1;
int ret = 0;

struct timespec start,end,diff,cycle;
struct timespec currentTimeTask1, currentTimeTask2, currentTimeTask3;
struct timespec lastExecutionTask1, lastExecutionTask2, lastExecutionTask3;
struct timespec diff1, diff2, diff3;
struct timespec cycle1, cycle2, cycle3;
pthread_mutex_t m1;

int status = 1; // 0 = OFF, 1 = ON (SHARED)
unsigned char buf[SEND_SIZE];
int flag;

void* showStatus(void*);
void* readAndSend(void*);
void* readAndSendV2(void*);
void* stopOrResume(void*);

/**********************************************************
 *  IMPLEMENTATION
 *********************************************************/

/*
 * Function: unblockRead
 */
int unblockRead(int fd, char *buffer, int size)
{
    fd_set readfd;
    struct timeval wtime;
	int ret;
    
    FD_ZERO(&readfd);
    FD_SET(fd, &readfd);
    wtime.tv_sec=0;
	wtime.tv_usec=0;
    ret = select(2048,&readfd,NULL,NULL,&wtime);
    if (ret < 0) {
    	perror("ERROR: select");
		return ret;
    }
    if (FD_ISSET(fd, &readfd)) {
        ret = read(fd, buffer, size);
        return (ret);
    } else {
 		return (0);
	}
}

/*
 * Function: diffTime
 */
void diffTime(struct timespec end, 
			  struct timespec start, 
			  struct timespec *diff) 
{
	if (end.tv_nsec < start.tv_nsec) {
		diff->tv_nsec = NSEC_PER_SEC - start.tv_nsec + end.tv_nsec;
		diff->tv_sec = end.tv_sec - (start.tv_sec+1);
	} else {
		diff->tv_nsec = end.tv_nsec - start.tv_nsec;
		diff->tv_sec = end.tv_sec - start.tv_sec;
	}
}

/*
 * Function: addTime
 */
void addTime(struct timespec end, 
			  struct timespec start, 
			  struct timespec *add) 
{
	unsigned long aux;
	aux = start.tv_nsec + end.tv_nsec;
	add->tv_sec = start.tv_sec + end.tv_sec + 
			      (aux / NSEC_PER_SEC);
	add->tv_nsec = aux % NSEC_PER_SEC;
}

/*
 * Function: compTime
 */
int compTime(struct timespec t1, 
			  struct timespec t2)
{
	if (t1.tv_sec == t2.tv_sec) {
		if (t1.tv_nsec == t2.tv_nsec) {
			return (0);
		} else if (t1.tv_nsec > t2.tv_nsec) {
			return (1);
		} else if (t1.tv_sec < t2.tv_sec) {
			return (-1);
		}
	} else if (t1.tv_sec > t2.tv_sec) {
		return (1);
	} else if (t1.tv_sec < t2.tv_sec) {
		return (-1);
	} 
	return (0);
}

/*
 * Function: showStatus
 */
void *showStatus(void* p) {
	while (1) {
		clock_gettime(CLOCK_REALTIME,&currentTimeTask3);
		pthread_mutex_lock(&m1);
		if (status == 1) {
			printf("Currently playing.\n");
		} else {
			printf("Stopped.\n");
		}
		pthread_mutex_unlock(&m1);
		clock_gettime(CLOCK_REALTIME,&lastExecutionTask3);
		diffTime(lastExecutionTask3,currentTimeTask3,&diff3);
		if (0 >= compTime(cycle3,diff3)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}
		diffTime(cycle3,diff3,&diff3);
		nanosleep(&diff3,NULL);   
		//addTime(start,cycle,&start);
	}
}

/*
 * Function: stopOrResume
 */
void *stopOrResume(void* p) {
	int flag;
	char buffer[1];
	while (1) {
		strcpy(buffer, "2");
		clock_gettime(CLOCK_REALTIME,&currentTimeTask2);
		flag = unblockRead(0, buffer, 1);
		//fwrite(buffer, 1, 1, stdout);
		if (flag > 0) {
			if (strcmp("0", buffer) == 0) {
				pthread_mutex_lock(&m1);
				status = 0;
				pthread_mutex_unlock(&m1);
			} else if (strcmp("1", buffer) == 0) {
				pthread_mutex_lock(&m1);
				status = 1;
				pthread_mutex_unlock(&m1);				
			}
		}
		clock_gettime(CLOCK_REALTIME,&lastExecutionTask2);
		diffTime(lastExecutionTask2,currentTimeTask2,&diff2);
		if (0 >= compTime(cycle2,diff2)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}
		diffTime(cycle2,diff2,&diff2);
		nanosleep(&diff2,NULL);   
		//addTime(start,cycle,&start);
	}
}

/*
 * Function: readAndSend
 * 
 * CAMBIAR ENTRE 256 (ARDUINO) y 500 (WINDOWS)
 */
void *readAndSend(void* p) {
    //unsigned char buf[SEND_SIZE];
	//unsigned char buf[SEND_SIZE];
	while (1) {
		clock_gettime(CLOCK_REALTIME,&currentTimeTask1);
		memset(buf, 0, sizeof(buf));
		
		pthread_mutex_lock(&m1);
		if (status == 0) {
			flag = 0;
		} else {
			flag = 1;
		}
		//printf("STATUS: %d\n", status);
		//printf("FLAG: %d\n", flag);
		pthread_mutex_unlock(&m1);
		
		
		if (flag == 0) {
			// write to serial port  		
			// Uncomment to test on the Arduino
			//ret = writeSerialMod_256 (buf);
		
			// Uncomment to test on the PC
			// Envia la musica de 1 bit, la version sin 4000 8 bit
			ret = reproducir_1bit_4000 (buf);
			if (ret < 0) {
				printf("write: error writting serial\n");
				return NULL;
			}				
		} else {
			// read from music file
			ret=read(fd_file,buf,SEND_SIZE);
			if (ret < 0) {
				printf("read: error reading file\n");
				return NULL;
			}

			// write to serial port  		
			// Uncomment to test on the Arduino
			//ret = writeSerialMod_256 (buf);
		
			// Uncomment to test on the PC
			// Envia la musica de 1 bit, la version sin 4000 8 bit
			ret = reproducir_1bit_4000 (buf);
			if (ret < 0) {
				printf("write: error writting serial\n");
				return NULL;
			}			
		}
		
		// get end time, calculate lapso and sleep
		clock_gettime(CLOCK_REALTIME,&lastExecutionTask1);
		diffTime(lastExecutionTask1,currentTimeTask1,&diff1);
		if (0 >= compTime(cycle1,diff1)) {
			printf("ERROR: lasted long than the cycle\n");
			//return NULL;
		}
		diffTime(cycle1,diff1,&diff1);
		nanosleep(&diff1,NULL);   
		//addTime(start,cycle,&start);
	}
}

void *readAndSendV2(void* p) {
	while (1) {
		clock_gettime(CLOCK_REALTIME,&currentTimeTask1);
		memset(buf, 0, sizeof(buf));
		
		pthread_mutex_lock(&m1);
		if (status == 0) {
			flag = 0;
		} else {
			flag = 1;
		}
		pthread_mutex_unlock(&m1);
		
		if (flag == 0) {
			// write to serial port  		
			// Uncomment to test on the Arduino
			//ret = writeSerialMod_256 (buf);
		
			// Uncomment to test on the PC
			// Envia la musica de 1 bit, la version sin 4000 8 bit
			ret = reproducir_1bit_4000 (buf);
			if (ret < 0) {
				printf("write: error writting serial\n");
				return NULL;
			}				
		} else {
			// read from music file
			ret=read(fd_file,buf,SEND_SIZE);
			if (ret < 0) {
				printf("read: error reading file\n");
				return NULL;
			}
			
			// write to serial port  		
			// Uncomment to test on the Arduino
			//ret = writeSerialMod_256 (buf);
		
			// Uncomment to test on the PC
			// Envia la musica de 1 bit, la version sin 4000 8 bit
			ret = reproducir_1bit_4000 (buf);
			if (ret < 0) {
				printf("write: error writting serial\n");
				return NULL;
			}			
		}
		
		// get end time, calculate lapso and sleep
		clock_gettime(CLOCK_REALTIME,&lastExecutionTask1);
		diffTime(lastExecutionTask1,currentTimeTask1,&diff1);
		if (0 >= compTime(cycle1,diff1)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
		}
		diffTime(cycle1,diff1,&diff1);
		nanosleep(&diff1,NULL);   
		//addTime(start,cycle,&start);
	}	
}

/*****************************************************************************
 * Function: main()
 *****************************************************************************/
int main()
{	
	// Uncomment to test on the Arduino
    //fd_serie = initSerialMod_WIN_115200 ();

	// Uncomment to test on the PC
	iniciarAudio_Windows ();

	/* Open music file */
	printf("open file %s begin\n",FILE_NAME);
	fd_file = open (FILE_NAME, O_RDONLY, 0644);
	if (fd_file < 0) {
		printf("open: error opening file\n");
		return -1;
	}
	
	pthread_mutexattr_t attrm1;
	
	pthread_mutexattr_setprotocol(&attrm1, PTHREAD_PRIO_PROTECT);
	pthread_mutexattr_setprioceiling(&attrm1, 3);
	
	pthread_mutex_init(&m1, &attrm1);
	
	pthread_attr_t attr1, attr2, attr3;
	
	pthread_attr_init(&attr1);
	pthread_attr_init(&attr2);
	pthread_attr_init(&attr3);
	
	struct sched_param sched1, sched2, sched3;
	
	sched1.sched_priority = 3;
	sched2.sched_priority = 2;
	sched3.sched_priority = 1;
	
	pthread_attr_setscope(&attr1, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&attr1, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr1, SCHED_FIFO);
	pthread_attr_setschedparam(&attr1, &sched1);
	
	pthread_attr_setscope(&attr2, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&attr2, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr2, SCHED_FIFO);
	pthread_attr_setschedparam(&attr2, &sched2);
	
	pthread_attr_setscope(&attr3, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setinheritsched(&attr3, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr3, SCHED_FIFO);
	pthread_attr_setschedparam(&attr3, &sched3);
	
	pthread_t th1, th2, th3;
	
	// PC
	cycle1.tv_sec = 1;
	cycle1.tv_nsec = 0;
	
	// 1st module
	//cycle1.tv_sec = 0.5;
	//cycle1.tv_nsec = 0;
	
	// 2nd module
	//cycle1.tv_sec = 0.067;
	//cycle1.tv_nsec = 0;
	
	cycle2.tv_sec = 2;
	cycle2.tv_nsec = 0;
	
	cycle3.tv_sec = 5;
	cycle3.tv_nsec = 0;
	
	if (pthread_create(&th1, NULL, readAndSend, NULL)) {
		fprintf(stdout, "Error creating thread\n");
		return -1;
	}
	if (pthread_create(&th2, &attr2, stopOrResume, NULL)) {
		fprintf(stdout, "Error creating thread\n");
		return -1;
	}
	if (pthread_create(&th3, &attr3, showStatus, NULL)) {
		fprintf(stdout, "Error creating thread\n");
		return -1;
	}
	
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	pthread_join(th3, NULL);
	
	/*
    // loading cycle time
    cycle.tv_sec=PERIOD_TASK_SEC;
    cycle.tv_nsec=PERIOD_TASK_NSEC;
    

	while (1) {
		unsigned char buf[SEND_SIZE];
		// read from music file
		ret=read(fd_file,buf,SEND_SIZE);
		if (ret < 0) {
			printf("read: error reading file\n");
			return NULL;
		}
		
		// write to serial port  		
		// Uncomment to test on the Arduino
		//ret = writeSerialMod_256 (buf);

		// Uncomment to test on the PC
		// Envia la musica de 1 bit, la version sin 4000 8 bit
		ret = reproducir_1bit_4000 (buf);
		if (ret < 0) {
			printf("write: error writting serial\n");
			return NULL;
		}
		
		// get end time, calculate lapso and sleep
	    clock_gettime(CLOCK_REALTIME,&end);
	    diffTime(end,start,&diff);
	    if (0 >= compTime(cycle,diff)) {
			printf("ERROR: lasted long than the cycle\n");
			return NULL;
	    }
	    diffTime(cycle,diff,&diff);
		nanosleep(&diff,NULL);   
	    addTime(start,cycle,&start);
	}
	*/
}
