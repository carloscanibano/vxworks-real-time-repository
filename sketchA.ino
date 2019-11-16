// --------------------------------------
// Global Variables
// --------------------------------------
#include <string.h>
#include <stdio.h>

// --------------------------------------
// Global Variables
// --------------------------------------
unsigned long secondary_cycle = 100;
unsigned long executed_time; // Secondary cycle executed time

int cycle = 1;
int cycle_mod = 1;

float speed = 55.0;
float accel = 0;
int slope = 0; // 1 = UP, -1 = DOWN, 0 = FLAT
int gas = 0; // 1 = ON, 0 = OFF 
int brake = 0; // 1 = 0N, 0 = OFF
int mixer = 0; // 1 = ON, 0 = OFF
int executionMode = 0; // 0 = Distance selector, 1 = Tank approach, 2 = Stop mode, 3 = Emergency
unsigned long time;
unsigned long current_time;

// --------------------------------------
// Function: comm_server
// --------------------------------------
int comm_server()
{
    int i;
    char request[10];
    char answer[10];
    int speed_int;
    int speed_dec;
    
    // while there is enough data for a request
    if (Serial.available() >= 9) {
        // read the request
        i=0; 
        while ( (i<9) && (Serial.available() >= (9-i)) ) {
            // read one character
            request[i]=Serial.read();
           
            // if the new line is positioned wrong 
            if ( (i!=8) && (request[i]=='\n') ) {
                // Send error and start all over again
                sprintf(answer,"MSG: ERR\n");
                Serial.print(answer);
                memset(request,'\0', 9);
                i=0;
            } else {
              // read the next character
              i++;
            }
        }
        request[9]='\0';
        
        // cast the request
        if (0 == strcmp("SPD: REQ\n",request)) {
            // send the answer for speed request
            speed_int = (int)speed;
            speed_dec = ((int)(speed*10)) % 10;
            sprintf(answer,"SPD:%02d.%d\n",speed_int,speed_dec);
            Serial.print(answer);
        } else if (0 == strcmp("SLP: REQ\n",request)) {
            if (slope == -1){
                sprintf(answer, "SLP:DOWN\n");
            } else if (slope == 1) {
                sprintf(answer, "SLP:  UP\n");
            } else {
                sprintf(answer, "SLP:FLAT\n");
            }
            Serial.print(answer);  
        } else if (0 == strcmp("GAS: SET\n",request)) {
            gas = 1;
            sprintf(answer, "GAS:  OK\n");
            Serial.print(answer);
        } else if (0 == strcmp("BRK: SET\n",request)) {
            brake = 1;
            sprintf(answer, "BRK:  OK\n");
            Serial.print(answer);
        } else if (0 == strcmp("MIX: SET\n",request)) {
            mixer = 1;
            sprintf(answer, "MIX:  OK\n");
            Serial.print(answer);
        } else if (0 == strcmp("GAS: CLR\n",request)) {
            gas = 0;
            sprintf(answer, "GAS:  OK\n");
            Serial.print(answer);
        } else if (0 == strcmp("BRK: CLR\n",request)) {
            brake = 0;
            sprintf(answer, "BRK:  OK\n");
            Serial.print(answer);
        } else if (0 == strcmp("MIX: CLR\n",request)) {
            mixer = 0;
            sprintf(answer, "MIX:  OK\n");
            Serial.print(answer);
        } else {
            // error, send error message
            sprintf(answer,"MSG: ERR\n");
            Serial.print(answer);
        }
    }
    return 0;
}

// --------------------------------------
// Function: task_speed
// --------------------------------------
int task_speed() {
  // Speed should not be dropped below 0
  if ((executionMode == 2) || (speed < 0.0)) {
    speed = 0.0;
    accel = 0.0;
  } else {
    if ((gas == 1) && (slope == -1)) {
      accel = 0.75;
    } else if ((gas == 1) && (slope == 1)) {
      accel = 0.25;
    } else if ((brake == 1) && (slope == -1)) {
      accel = -0.25;
    } else if ((brake == 1) && (slope == 1)) {
      accel = -0.75;
    } else if (gas == 1) {
      accel = 0.5;
    } else if (brake == 1) {
      accel = -0.5;
    } else if (slope == 1) {
      accel = -0.25;
    } else if (slope == -1) {
      accel = 0.25;
    } else if (slope == 0) {
      accel = 0.0;
    }
    // Recalculating speed, time is always 0.1 seconds (secondary cycle)
    speed = speed + (accel * 0.1);
    // Speed may move between 0-70
    analogWrite(10, map(speed, 0, 70, 0, 255));
  }
  
  return 0;
}

// --------------------------------------
// Function: task_gas
// --------------------------------------
int task_gas() {
   if (gas == 1) {
     digitalWrite(13, HIGH);
   } else {
     digitalWrite(13, LOW);
   }
   return 0;
}

// --------------------------------------
// Function: task_brake
// --------------------------------------
int task_brake() {
  if (brake == 1) {
    digitalWrite(12, HIGH);
  } else {
    digitalWrite(12, LOW);
  }
  return 0;
}

// --------------------------------------
// Function: task_mixer
// --------------------------------------
int task_mixer() {
  if (mixer == 1) {
    digitalWrite(11, HIGH);
  } else {
    digitalWrite(11, LOW);
  }
  return 0;
}

// --------------------------------------
// Function: task_slope
// --------------------------------------
int task_slope() {
    if (digitalRead(8) == HIGH) {
        slope = -1;
    } else if (digitalRead(9) == HIGH) {
        slope = 1;
    } else {
        slope = 0;
    }
  return 0;  
}

// --------------------------------------
// Function: mode_0
// --------------------------------------
int mode_0() {
  if (cycle_mod != 0) {
    task_gas();
    task_brake();        
    task_mixer();    
    task_slope();        
    task_speed();               
    cycle = cycle + 1;
    cycle_mod = cycle % 2;
    current_time = millis();
  } else {    
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed();
    comm_server();   
    cycle = cycle + 1;
    cycle_mod = cycle % 2;
    current_time = millis();
  }
  executed_time = current_time - time;
  delay(secondary_cycle - executed_time);
  time = time + secondary_cycle;
  return 0;
}

// --------------------------------------
// Function: setup
// --------------------------------------
void setup() {  
    Serial.begin(9600);
    pinMode(13, OUTPUT); //GAS
    pinMode(12, OUTPUT); //BRAKE
    pinMode(11, OUTPUT); //MIXER
    pinMode(10, OUTPUT); //SPEED
    pinMode(9, INPUT);   //SLOPE UP
    pinMode(8, INPUT);   //SLOPE DOWN
    time = millis();
}

// --------------------------------------
// Function: loop
// --------------------------------------
void loop() {
  mode_0();
}
