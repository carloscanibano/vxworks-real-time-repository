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
int distance = 0;
int serial = 0;// 0 = SIMULATOR, 1 = SERIAL
int loading = 1;// 0 = FINISH, 1 = NOT FINISH
int brk = 0; //0 = OFF, 1 = ON
int gas = 0; //0 = OFF, 1 = ON
int mix_status = 0;// 0 = OFF, 1 = ON
int light_status = 0;// 0 = OFF, 1 = ON
int mix_cycles = 0;
int cycle = 0;
int cycles = 6;
int cycles_unload = 3;
int change_mode = 0;// 0 = NO, 1 = YES
int mode = 0;// 0 = NORMAL, 1 = BRAKED, 2 = UNLOAD
struct timespec sleep_time;
struct timespec cycle_time;
struct timespec finish_time;
struct timespec start_time;
struct timespec time_difference;
struct timespec TS;

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
		
	if(!serial){
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

	if(!serial){
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
int task_gas(){
	
	char request[10];
	char answer[10];
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
    
    if (mode == 0) {
    	if ((gas == 0) && (speed <= 55)) {
    		strcpy(request, "GAS: SET\n");
    		
    		if(!serial){
    			simulator(request, answer);
    		} else {
    			writeSerialMod_9(request);
    			readSerialMod_9(answer);
    		}	
    		gas = 1;	
    	} else if((gas == 1) && (speed > 55)){
    		strcpy(request, "GAS: CLR\n");
    		
    		if(!serial){
    			simulator(request, answer);
    		} else {
    			writeSerialMod_9(request);
    			readSerialMod_9(answer);
    		}
    		
    		gas = 0;
    	}
    } else {
    	if ((gas == 0) && (speed <= 5)) {
    		strcpy(request, "GAS: SET\n");
    		
    		if(!serial){
    			simulator(request, answer);
    		} else {
    			writeSerialMod_9(request);
    			readSerialMod_9(answer);
    		}	
    		gas = 1;	
    	} else if((gas == 1) && (speed > 5)){
    		strcpy(request, "GAS: CLR\n");
    		
    		if(!serial){
    			simulator(request, answer);
    		} else {
    			writeSerialMod_9(request);
    			readSerialMod_9(answer);
    		}
    		
    		gas = 0;
    	}
    }
	
	if (0 == strcmp(answer,"GAS:  OK\n")) displayGas(gas);
	
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
   
	if (mode == 0) {
		if ((brk == 1) && (speed <= 55)) {
			strcpy(request, "BRK: CLR\n");
			
			if(!serial){
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
			brk = 0;
		} else if ((brk == 0) && (speed > 55)){
			strcpy(request, "BRK: SET\n");
			
			if(!serial){
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}

			brk = 1;
		}
	} else {
		if ((brk == 1) && (speed <= 5)) {
			strcpy(request, "BRK: CLR\n");
			
			if(!serial){
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
			brk = 0;
		}else if ((brk == 0) && (speed > 5)){
			strcpy(request, "BRK: SET\n");
			
			if(!serial){
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}

			brk = 1;
		}
	}
	
	if (0 == strcmp(answer,"BRK:  OK\n")) displayBrake(brk);
	
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
	
    
   	if (mix_cycles == 2) {
   		if (mix_status == 0) {
			strcpy(request, "MIX: SET\n");
			
			if(!serial){
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
    	    mix_status = 1;
    	} else {
    	    strcpy(request, "MIX: CLR\n");
    	    
			if(!serial){
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
			
    	    mix_status = 0;
    	}  
    	mix_cycles = 0;
    }
   	
   	if (0 == strcmp(answer,"MIX:  OK\n")) displayMix(mix_status);
   	mix_cycles += 1;
   	
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

	if(!serial){
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}
    	
    if (1 == sscanf (answer,"LIT: %d%\n",&bright)) {
    	if (bright < 20) {
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
int task_lamps(){
	char request[10];
	char answer[10];
    	
    //clear request and answer
    memset(request,'\0',10);
    memset(answer,'\0',10);
	
    if (mode == 0) {
    	if (bright < 20) {
    	    if (light_status == 0) {
    	    	strcpy(request, "LAM: SET\n");
    	    	
    			if(!serial){
    				simulator(request, answer);
    			} else {
    				writeSerialMod_9(request);
    				readSerialMod_9(answer);
    			}
    			
    	    	light_status = 1;
    	    }
    	} else {
    	    if(light_status == 1){
    	    	strcpy(request, "LAM: CLR\n");
    	    	
    			if(!serial){
    				simulator(request, answer);
    			} else {
    				writeSerialMod_9(request);
    				readSerialMod_9(answer);
    			}
    			
    	    	light_status = 0;
			}
    	}
    } else {
    	if (light_status == 0) {
    		strcpy(request, "LAM: SET\n");
			
    		if(!serial){
				simulator(request, answer);
			} else {
				writeSerialMod_9(request);
				readSerialMod_9(answer);
			}
    		
    		light_status = 1;
    	}
    }
    
    if (0 == strcmp(answer,"LAM:  OK\n")) displayLamps(light_status);
    
	return 0;
}

/**********************************************************
 *  Function: task_distance
 *********************************************************/
int task_distance()
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
	strcpy(request,"DS:  REQ\n");
		
	//uncomment to use the simulator
	simulator(request, answer);
		
	// uncoment to access serial module
	//writeSerialMod_9(request);
	//readSerialMod_9(answer);
	
	// display speed
	if (1 == sscanf (answer,"DS:%d\n",&distance)) {
		displayDistance(distance);  
	}
	
	return 0;
}

/**********************************************************
 *  Function: task_lamps
 *********************************************************/
int task_movement(){
	char request[10];
	char answer[10];
	    	
	//clear request and answer
	memset(request,'\0',10);
	memset(answer,'\0',10);
	
	strcpy(request,"STP: REQ\n");
		
	if(!serial){
		simulator(request, answer);
	} else {
		writeSerialMod_9(request);
		readSerialMod_9(answer);
	}
	    	
	if (0 == strcmp(answer,"STP:STOP\n")){
	    displayStop(1);
	} else if (0 == strcmp(answer,"STP:  GO\n")){
		loading = 0;
	    displayStop(0);
	}
	    
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
    	if(mode == 0){
    		if(cycle == 0){
    			task_speed();
    			task_slope();
    			task_lightSensor();
    			task_lamps();
    			task_mix();
    		}else if(cycle == 1){
    			task_distance();
    			task_gas();
    			task_brake();
    			task_lightSensor();
    			task_lamps();
    		}else if(cycle == 2){
    			task_speed();
    			task_slope();
    			task_lightSensor();
    			task_lamps();
    		}else if(cycle == 3){
    			task_gas();
    			task_brake();
    			task_lightSensor();
    			task_lamps();
    			task_mix();
    		}else if(cycle == 4){
    			task_speed();
    			task_slope();
    			task_lightSensor();
    			task_lamps();
    			task_distance();
    		}else if(cycle == 5){
    			task_gas();
    			task_brake();
    			task_lightSensor();
    			task_lamps();
    		}
    	} else if(mode == 1) {
    		if(cycle == 0){
    			task_speed();
    			task_gas();
    			task_brake();
    			task_distance();
    			task_mix();
    		}else if(cycle == 1){
    			task_speed();
    			task_gas();
    			task_brake();
    			task_slope();
    			task_lamps();
    		}else if(cycle == 2){
    			task_speed();
    			task_gas();
    			task_brake();
    			task_distance();
    		}else if(cycle == 3){
    			task_speed();
    			task_gas();
    			task_brake();
    			task_slope();
    			task_mix();
    		}else if(cycle == 4){
    			task_speed();
    			task_gas();
    			task_brake();
    			task_distance();
    		}else if(cycle == 5){
    			task_speed();
    			task_gas();
    			task_brake();
    			task_slope();
    		}
    	} else if(mode == 2){
    		if(cycle == 0){
    			task_movement();
    			task_lamps();
    			task_mix();
    		}else if(cycle == 1){
    			task_movement();
    			task_mix();
    		}else if(cycle == 2){
    			task_movement();
    			task_mix();
    		}
    		displaySpeed(0); 
    	}
    	clock_gettime(CLOCK_REALTIME, &finish_time);
    	if( ((distance >= 11000) && (mode != 0)) || ((distance <= 0) && (speed > 10) && (mode != 0)) || ((distance <= 0) && (loading == 0) && (mode != 0))){
    		if((loading == 0) && (distance > 0)){
    			loading = 1;
    		}
    		change_mode = 1;
    		mode = 0;
    	} else if((distance > 0) && (distance <= 11000) && (mode != 1)) {
    		if((loading == 0) && (distance > 0)){
    		   loading = 1;
    		}
    		change_mode = 1;
    		mode = 1;
    	} else if((speed <= 10) && (distance <= 0) && (loading == 1) && (mode != 2)){
    		change_mode = 1;
    		mode = 2;
    	}
    	
    	if(change_mode == 1){
    		cycle = 0;
    		change_mode = 0;
    	} else {
    		if (mode != 2) {
    			cycle = (cycle+1)% cycles;
    		} else {
    			cycle = (cycle+1)% cycles_unload;
    		}
    	}
    	
    	diffTime(finish_time, start_time, &time_difference);
    	diffTime(cycle_time, time_difference, &sleep_time);
    	clock_nanosleep(CLOCK_REALTIME,0, &sleep_time, NULL);
    	addTime(cycle_time,start_time, &start_time);
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
    
