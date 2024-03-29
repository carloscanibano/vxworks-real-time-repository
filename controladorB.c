/**********************************************************
 *  INCLUDES
 *********************************************************/
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ioLib.h>
#include <time.h>
#include "displayA.h"
#include "serialcallLib.h"

/**********************************************************
 *  Constants
 **********************************************************/
#define NS_PER_S  1000000000

/**********************************************************
 *  Global Variables 
 *********************************************************/
float speed = 0.0;
int bright = 0;
int serial = 0;// 0 = SIMULATOR, 1 = SERIAL
int brk = 0; //0 = OFF, 1 = ON
int gas = 0; //0 = OFF, 1 = ON
int mix_status = 0;// 0 = OFF, 1 = ON
int light_status = 0;// 0 = OFF, 1 = ON
int mix_cycles = 0;
int cycle = 0;
int cycles = 6;
struct timespec sleep_time;
struct timespec cycle_time;
struct timespec finish_time;
struct timespec start_time;
struct timespec time_difference;

/**********************************************************
 *  Function: diffTime 
 *********************************************************/
void diffTime(struct timespec end, 
			  struct timespec start, 
			  struct timespec *diff) 
{
	if (end.tv_nsec < start.tv_nsec) {
		diff->tv_nsec = NS_PER_S - start.tv_nsec + end.tv_nsec;
		diff->tv_sec = end.tv_sec - (start.tv_sec+1);
	} else {
		diff->tv_nsec = end.tv_nsec - start.tv_nsec;
		diff->tv_sec = end.tv_sec - start.tv_sec;
	}
}

/**********************************************************
 *  Function: addTime 
 *********************************************************/
void addTime(struct timespec end, 
			  struct timespec start, 
			  struct timespec *add) 
{
	unsigned long aux;
	aux = start.tv_nsec + end.tv_nsec;
	add->tv_sec = start.tv_sec + end.tv_sec + 
			      (aux / NS_PER_S);
	add->tv_nsec = aux % NS_PER_S;
}

/**********************************************************
 *  Function: task_speed
 *********************************************************/
int task_speed()
{
	char request[10];
	char answer[10];
	
   	//--------------------------------
    //  request speed and display it 
    //--------------------------------
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
    
    // request speed
	strcpy(request,"SPD: REQ\n");
		
	if (!serial) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}
	
	// display speed
	if (1 == sscanf (answer,"SPD:%f\n",&speed)) {
		displaySpeed(speed);  
	}
	
	return 0;
}

/**********************************************************
 *  Function: task_slope
 *********************************************************/
int task_slope()
{
	char request[10];
	char answer[10];
	
	//--------------------------------
	//  request slope and display it 
	//--------------------------------

	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);

	// request slope
	strcpy(request,"SLP: REQ\n");

	if (!serial) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}

	// display slope
	if (0 == strcmp(answer,"SLP:DOWN\n")) displaySlope(-1);	
	if (0 == strcmp(answer,"SLP:FLAT\n")) displaySlope(0);	
	if (0 == strcmp(answer,"SLP:  UP\n")) displaySlope(1);

	return 0;
}

/**********************************************************
 *  Function: task_gas
 *********************************************************/
int task_gas()
{
	char request[10];
	char answer[10];
	
   	//--------------------------------
    //  request gas on or off 
    //--------------------------------
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
    
	
	if ( (gas == 0) && (speed <= 55) ) {
		// request gas on
		strcpy(request, "GAS: SET\n");
		
		if (!serial) {
			simulator(request, answer);
		} else {
			writeSerialMod_9(request);
			readSerialMod_9(answer);
		}	
		gas = 1;	
	} else if ( (gas == 1) && (speed > 55) ) {
		// request gas off
		strcpy(request, "GAS: CLR\n");
		
		if (!serial) {
			simulator(request, answer);
		} else {
			writeSerialMod_9(request);
			readSerialMod_9(answer);
		}
		
		gas = 0;
	}
	
	// display gas status
	if (0 == strcmp(answer,"GAS:  OK\n")) displayGas(gas);
	
	return 0;
}

/**********************************************************
 *  Function: task_brake
 *********************************************************/
int task_brake()
{
	char request[10];
	char answer[10];
	
   	//--------------------------------
    //  request brake on or off
    //--------------------------------
    
	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);
    			
	if ( (brk == 1) && (speed <= 55) ) {
		// request break off
		strcpy(request, "BRK: CLR\n");
		
		if (!serial) {
			simulator(request, answer);
		} else {
			writeSerialMod_9(request);
			readSerialMod_9(answer);
		}
		
		brk = 0;
	} else if ( (brk == 0) && (speed > 55) ){
		// request break on
		strcpy(request, "BRK: SET\n");
		
		if (!serial) {
			simulator(request, answer);
		} else {
			writeSerialMod_9(request);
			readSerialMod_9(answer);
		}

		brk = 1;
	}
	
	// display break status
	if (0 == strcmp(answer,"BRK:  OK\n")) displayBrake(brk);
	
	return 0;
}


/**********************************************************
 *  Function: task_mix
 *********************************************************/
int task_mix()
{
	char request[10];
	char answer[10];
	
   	//--------------------------------
    //  request mix on or off 
    //--------------------------------
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
    
   	if (mix_cycles == 2) {
   		if (mix_status == 0) {
   			// request mix on
			strcpy(request, "MIX: SET\n");
			
			if (!serial) {
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
    	    mix_status = 1;
    	} else {
   			// request mix off
    	    strcpy(request, "MIX: CLR\n");
    	    
			if (!serial) {
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
    	    mix_status = 0;
    	}  
    	mix_cycles = 0;
    }
   	
	// display mix status
   	if (0 == strcmp(answer,"MIX:  OK\n")) displayMix(mix_status);
   	mix_cycles += 1;
   	
	return 0;
}

/**********************************************************
 *  Function: task_lightSensor
 *********************************************************/
int task_lightSensor()
{
	char request[10];
	char answer[10];
	
   	//--------------------------------
    //  request bright and display it
    //--------------------------------
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
    // request light
    strcpy(request,"LIT: REQ\n");

	if (!serial) {
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}
    
	// display light sensor status
    if (1 == sscanf (answer,"LIT: %2d%\n",&bright)) {
    	if (bright < 50) {
    		displayLightSensor(1);  
    	} else {
    		displayLightSensor(0);  
    	}
    }

    return 0;
}


/**********************************************************
 *  Function: task_lamps
 *********************************************************/
int task_lamps()
{
	char request[10];
	char answer[10];
	
   	//--------------------------------
    //  request lamps on or off
    //--------------------------------
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
	if (bright < 50) {
	    if (light_status == 0) {
	        // request lights on
	    	strcpy(request, "LAM: SET\n");
	    	
			if (!serial) {
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
	    	light_status = 1;
	    }
	} else {
	    if (light_status == 1) {
	        // request lights off
	    	strcpy(request, "LAM: CLR\n");
	    	
			if (!serial) {
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
	    	light_status = 0;
		}
	}
	
	// display lights status
    if (0 == strcmp(answer,"LAM:  OK\n")) displayLamps(light_status);
 
	return 0;
}

/**********************************************************
 *  Function: finish_cycle
 *********************************************************/
int finish_cycle()
{
	//complete the secundary cycle time
	cycle = (cycle+1)% cycles;
	diffTime(finish_time, start_time, &time_difference);
	diffTime(cycle_time, time_difference, &sleep_time);
	clock_nanosleep(CLOCK_REALTIME,0, &sleep_time, NULL);
	addTime(cycle_time,start_time, &start_time);
	
	return 0;
}

/**********************************************************
 *  Function: controller
 *********************************************************/
void *controller(void *arg)
{    
	cycle_time.tv_sec = 5;
	cycle_time.tv_nsec = 0;
	clock_gettime(CLOCK_REALTIME, &start_time);
	// Endless loop
	while(1) {
	    if (cycle == 0) {
	    	task_speed();
	    	task_slope();
	    	task_lightSensor();
	    	task_lamps();
	    	task_mix();
	    }else if (cycle == 1) {
	    	task_gas();
	    	task_brake();
	    	task_lightSensor();
	    	task_lamps();
	    }else if (cycle == 2) {
	    	task_speed();
	    	task_slope();
	    	task_lightSensor();
	    	task_lamps();
	    }else if (cycle == 3) {
	    	task_gas();
	    	task_brake();
	    	task_lightSensor();
	    	task_lamps();
	    	task_mix();
	    }else if (cycle == 4) {
	    	task_speed();
	    	task_slope();
	    	task_lightSensor();
	    	task_lamps();
	    }else if (cycle == 5) {
	    	task_gas();
	    	task_brake();
	    	task_lightSensor();
	    	task_lamps();
	    }
	    clock_gettime(CLOCK_REALTIME, &finish_time);
	    finish_cycle();
	}
}

/**********************************************************
 *  Function: main
 *********************************************************/
int main ()
{
    pthread_t thread_ctrl;
	sigset_t alarm_sig;
	int i;

	/* Block all real time signals so they can be used for the timers.
	   Note: this has to be done in main() before any threads are created
	   so they all inherit the same mask. Doing it later is subject to
	   race conditions */
	sigemptyset (&alarm_sig);
	for (i = SIGRTMIN; i <= SIGRTMAX; i++) {
		sigaddset (&alarm_sig, i);
	}
	sigprocmask (SIG_BLOCK, &alarm_sig, NULL);

    // init display
	displayInit(SIGRTMAX);
	
	// initSerialMod_9600 uncomment to work with serial module
	//initSerialMod_WIN_9600 ();

    /* Create first thread */
    pthread_create (&thread_ctrl, NULL, controller, NULL);
    pthread_join (thread_ctrl, NULL);
    return (0);
}
    
