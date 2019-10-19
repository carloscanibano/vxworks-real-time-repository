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

/**********************************************************
 *  Global Variables 
 *********************************************************/
float speed = 0.0;
int bright = 0;
int brk_gas = 0; //0 = BRK, 1 = GAS
int mix_status = 0; // 0 = OFF, 1 = ON
int mixing_time = 1;	// Time mixing or recharging (30 secs)
struct timespec slope_time;
struct timespec speed_time;
struct timespec mix_time;
struct timespec bright_time;
struct timespec current_time;
struct timespec time_difference;

#define NS_PER_S  1000000000
#define PERIOD_NS  500000000
#define TIME_TO_RELOAD   30
#define INIT_HIGH   0

// DISPLAY DELAY
#define TIME_DISPLAY_SEC 0
#define TIME_DISPLAY_NSEC 500000000


// FORCING ERRORS
#define TIME_BW_ERRORS   30
#define ERROR_DELAY_TIME 1

/**********************************************************
 *  Function: getClock 
 *********************************************************/
double getClock()
{
    struct timespec tp;
    double reloj;
    
    clock_gettime (CLOCK_REALTIME, &tp);
    reloj = ((double)tp.tv_sec) + 
	    ((double)tp.tv_nsec) / ((double)NS_PER_S);
    //printf ("%d:%d",tp.tv_sec,tp.tv_nsec);

    return (reloj);
}

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
 *  Function: compTime 
 *********************************************************/
int compTime(struct timespec t1, 
			  struct timespec t2)
{
	if (t1.tv_sec == t2.tv_sec) {
		if (t1.tv_nsec == t2.tv_nsec) {
			return (0);
		} else if (t1.tv_nsec > t2.tv_nsec) {
			return (1);
		} else if (t1.tv_nsec < t2.tv_nsec) {
			return (-1);
		}
	} else if (t1.tv_sec > t2.tv_sec) {
		return (1);
	} else if (t1.tv_sec < t2.tv_sec) {
		return (-1);
	} 
	return (0);
}

/**********************************************************
 *  Function: retraso 
 *********************************************************/
double retraso ()
{	
	static int init = 0;
    static int lastCount = 0;
    static struct timespec initClock;
    struct timespec actualClock, diffClock;
    int actualCount;
    int lapso;
    int ret = 0;

    if (0 == init) {
		clock_gettime(CLOCK_REALTIME, &initClock); 
    }
	clock_gettime(CLOCK_REALTIME, &actualClock); 
	diffTime(actualClock,initClock, &diffClock);
    lapso = diffClock.tv_sec;
    actualCount = lapso / TIME_BW_ERRORS;
    if (lastCount < actualCount) {
        ret = ERROR_DELAY_TIME * actualCount;
    } 
    return (ret);
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
		
	//uncomment to use the simulator
	simulator(request, answer);
		
	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);
	
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

	//uncomment to use the simulator
	simulator(request, answer);

	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);
	

	// display slope
	if (0 == strcmp(answer,"SLP:DOWN\n")) displaySlope(-1);	
	if (0 == strcmp(answer,"SLP:FLAT\n")) displaySlope(0);	
	if (0 == strcmp(answer,"SLP:  UP\n")) displaySlope(1);

	return 0;
}

/**********************************************************
 *  Function: task_gas
 *********************************************************/
int task_gas(){
	
	char request[10];
	char answer[10];
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
	if (speed <= 55) {
		if (brk_gas == 0) {
			memset(answer,'\0',10);
			simulator("BRK: CLR\n", answer);
			if (0 == strcmp(answer,"BRK:  OK\n")) displayBrake(0);
		}
		memset(answer,'\0',10);
		simulator("GAS: SET\n", answer);
		if (0 == strcmp(answer,"GAS:  OK\n")) displayGas(1);
		brk_gas = 1;
	}
	
	return 0;
}

/**********************************************************
 *  Function: task_brake
 *********************************************************/
int task_brake(){
	
	char request[10];
	char answer[10];
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
	if (speed > 55) {
		if (brk_gas == 1) {
			memset(answer,'\0',10);
			simulator("GAS: CLR\n", answer);
			if (0 == strcmp(answer,"GAS:  OK\n")) displayGas(0);
		}
		memset(answer,'\0',10);
		simulator("BRK: SET\n", answer);
		if (0 == strcmp(answer,"BRK:  OK\n")) displayBrake(1);
		brk_gas = 0;
	}	
	
	return 0;
}

/**********************************************************
 *  Function: task_mix
 *********************************************************/
int task_mix(){
	char request[10];
	char answer[10];
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
    if (mixing_time == 1) {
    	if (mix_status == 0) {
    		strcpy(request, "MIX: SET\n");
    		simulator(request, answer);
    		if (0 == strcmp(answer,"MIX:  OK\n")) displayMix(1);
    		mix_status = 1;
    		mixing_time = 0;
    	} else {
    		strcpy(request, "MIX: CLR\n");
    		simulator(request, answer);
    		if (0 == strcmp(answer,"MIX:  OK\n")) displayMix(0);
    		mix_status = 0;
    		mixing_time = 0;
    	}  	
    } else {
    	mixing_time = 1;
    }

	return 0;
}

/**********************************************************
 *  Function: task_lightSensor
 *********************************************************/
int task_lightSensor(){
	char request[10];
	char answer[10];
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
    // request light
    strcpy(request,"LIT: REQ\n");

    //uncomment to use the simulator
    simulator(request, answer);

    // uncoment to access serial module
    //writeSerialMod_9(request);
    //readSerialMod_9(answer);
    	

    if (1 == sscanf (answer,"LIT: %d%\n",&bright)) {
    	if(bright < 10){
    		displayLightSensor(1);  
    	}else{
    		displayLightSensor(0);  
    	}
    }

    return 0;
}

/**********************************************************
 *  Function: task_lamps
 *********************************************************/
int task_lamps(){
	char request[10];
	char answer[10];
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
    if (bright < 10) {
    	strcpy(request, "LAM: SET\n");
    	simulator(request, answer);
    	if (0 == strcmp(answer,"LAM:  OK\n")) displayLamps(1);
    } else {
    	strcpy(request, "LAM: CLR\n");
    	simulator(request, answer);
    	if (0 == strcmp(answer,"LAM:  OK\n")) displayLamps(0);
    }  	
 
	return 0;
}


/**********************************************************
 *  Function: controller
 *********************************************************/
void *controller(void *arg)
{    
	clock_gettime(CLOCK_REALTIME, &slope_time);
	clock_gettime(CLOCK_REALTIME, &speed_time);
	clock_gettime(CLOCK_REALTIME, &mix_time);
	clock_gettime(CLOCK_REALTIME, &bright_time);
	clock_gettime(CLOCK_REALTIME, &current_time);
	
	task_speed();
	task_slope();
	task_gas();
	task_brake();
	task_mix();
	task_lightSensor();
	task_lamps();
	
    // Endless loop
    while(1) {
    	task_slope();
    	task_speed();
    	task_lightSensor();
    	task_lamps();
    	
    	/*
    	// calling task of speed
    	clock_gettime(CLOCK_REALTIME, &current_time);
    	diffTime(current_time, speed_time, &time_difference);
    	if (time_difference.tv_sec >= 5) {
    	   	check_speed();
    	   	clock_gettime(CLOCK_REALTIME, &speed_time);
    	}
    	
    	clock_gettime(CLOCK_REALTIME, &current_time);
    	diffTime(current_time, brigth_time, &time_difference);
    	if (time_difference.tv_sec >= 6) {
			task_lightSensor();
			task_lamps();
			clock_gettime(CLOCK_REALTIME, &bright_time);
    	}
    	*/
    	
		// caling task of slope
    	clock_gettime(CLOCK_REALTIME, &current_time);
    	diffTime(current_time, slope_time, &time_difference);
		if (time_difference.tv_sec >= 10) {
			task_slope();
			task_speed();
			task_gas();
			task_brake();
	    	clock_gettime(CLOCK_REALTIME, &slope_time);
		}
		
    	clock_gettime(CLOCK_REALTIME, &current_time);
    	diffTime(current_time, mix_time, &time_difference);
		if (time_difference.tv_sec >= 15) {
	    	task_mix();
	    	clock_gettime(CLOCK_REALTIME, &mix_time);
		}
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
    
