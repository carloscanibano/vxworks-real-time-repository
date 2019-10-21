// --------------------------------------
// Global Variables
// --------------------------------------
#include <string.h>
#include <stdio.h>

// --------------------------------------
// Global Variables
// --------------------------------------

//Test
unsigned long main_cycle = 200;
unsigned long secondary_cycle = 100;
unsigned long executed_time;
int cycle = 1;

float speed = 55.0;
int slope = 0; // 1 = UP, -1 = DOWN, 0 = FLAT
int gas = 0; // 1 = ON, 0 = OFF 
int brake = 1; // 1 = 0N, 0 = OFF
int mixer = 0; // 1 = ON, 0 = OFF
unsigned long time = 0;
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
  float accel = 0;
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
  }
  
  //current_time = millis();
  speed = speed + (accel * 0.1);
  analogWrite(10, map(speed, 0, 255, 40, 70));
  //time = millis();
  
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
}

// --------------------------------------
// Function: loop
// --------------------------------------
void loop() {
  if (cycle % 2 != 0) {
    //time = millis();
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed();
    current_time = millis();
  } else {
    //time = millis();
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed();
    comm_server();
    current_time = millis();
  }
  cycle = cycle + 1;
  executed_time = current_time - time;
  delay(main_cycle - executed_time);
  time = time + main_cycle;
}
