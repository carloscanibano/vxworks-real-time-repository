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
int lamps = 0; // 1 = ON, 0 = OFF
int darkness = 0; // Light ratio
unsigned long distanceSelector = 0; // Potentiometer value (distance)
int selectedDistance = 0; // Used distance value
float distanceDifference = 0; //Numeric checking
float currentDistance = 0; // Distante left to arrive at final destination
int executionMode = 0; // 0 = Distance selector, 1 = Tank approach, 2 = Stop mode, 3 = Emergency
int buttonMode = 0; // 0 = OFF, 1 = ON
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
        } else if (0 == strcmp("LIT: REQ\n",request)) {
            // Value range may change depending on light
            int converter = map(darkness, 0, 1000, 00, 99);
            sprintf(answer,"LIT: %02d%%\n",converter);
            Serial.print(answer);
        } else if (0 == strcmp("LAM: SET\n",request)) {
            lamps = 1;
            sprintf(answer,"LAM:  OK\n");
            Serial.print(answer);
        } else if (0 == strcmp("LAM: CLR\n",request)) {
            lamps = 0;
            sprintf(answer,"LAM:  OK\n");
            Serial.print(answer);
        } else if (0 == strcmp("STP: REQ\n",request)) {
            if (executionMode != 2) {
              sprintf(answer,"STP:  GO\n");
            } else {
              sprintf(answer,"STP:STOP\n");
            }
            Serial.print(answer);
        } else if (0 == strcmp("DS:  REQ\n",request)) {
            if (executionMode == 1) {
              // Distance left to arrive at point
              sprintf(answer,"DS:%05d\n", (int) distanceDifference);  
            } else if (executionMode == 0) {
               sprintf(answer, "MSG: ERR\n");
            } else {
              sprintf(answer, "DS:00000\n");
            }
            Serial.print(answer);
        } else if (0 == strcmp("ERR: SET\n",request)) {
            executionMode = 3;
            sprintf(answer, "ERR:  OK\n");
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
// Function: task_lightSensor
// --------------------------------------
int task_lightSensor() {
  darkness = analogRead(A0);
  return 0;
}

// --------------------------------------
// Function: task_lamps
// --------------------------------------
int task_lamps() {
  if (lamps == 1) {
    digitalWrite(7, HIGH);
  } else {
    digitalWrite(7, LOW);
  }
  return 0;
}

// --------------------------------------
// Function: task_distanceSelector
// --------------------------------------
int task_distanceSelector() {
  // Potentiometer value range may change and would need a manual change
  distanceSelector = map(analogRead(A1), 508, 1000, 10000, 90000);
  return 0;
}

// --------------------------------------
// Function: task_displayDistance
// --------------------------------------
int task_displayDistance() {
  int digit;
  
  if (executionMode == 0) {
    digit = distanceSelector / 10000;
  } else if (executionMode == 1) {
    // x = x0 + (v0 * t) + (1/2 * a * t^2)
    currentDistance += (speed * 0.1) + (0.5 * accel * pow(0.1, 2));
    // Distance left to arrive at point
    distanceDifference = (selectedDistance - currentDistance);
    // Taking care with numbers under 10000
    if (distanceDifference >= 10000) {
      digit = distanceDifference / 10000;
    } else {
      digit = 1;
    }

    if (distanceDifference <= 0) {
      // Too fast, returning to distance selection mode
      if (speed > 10.0) {
        executionMode = 0;
        currentDistance = 0;
      // Able to stop
      } else {
        digit = 0;
        executionMode = 2;
      }
    }
  } else {
    digit = 0;
  }

  int bcdPins[] = {5, 4, 3, 2};  // {D C B A)
  byte bcdCode[10][4] = {
    //  D  C  B  A
    { 0, 0, 0, 0},  // 0
    { 0, 0, 0, 1},  // 1
    { 0, 0, 1, 0},  // 2
    { 0, 0, 1, 1},  // 3
    { 0, 1, 0, 0},  // 4
    { 0, 1, 0, 1},  // 5
    { 0, 1, 1, 0},  // 6
    { 0, 1, 1, 1},  // 7
    { 1, 0, 0, 0},  // 8
    { 1, 0, 0, 1}   // 9
  };
  
  for (int i=0; i < 4; i++) {
    digitalWrite(bcdPins[i], bcdCode[digit][i]);
  }
  
  return 0;
}

// --------------------------------------
// Function: task_distanceSelect
// --------------------------------------
int task_distanceSelect() {
  // Once we push the button we turn this flag ON
  if (digitalRead(6) == HIGH) {
    buttonMode = 1;
  // Once the button is released and the flag is ON, go ahead
  } else if (buttonMode == 1) {
    if (executionMode == 0) {
      selectedDistance = distanceSelector;
      distanceSelector = 0;
      executionMode = 1;
    } else {
      selectedDistance = 0;
      currentDistance = 0;
      executionMode = 0;
    }
    // The flag needs to be restarted everytime
    buttonMode = 0;
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
    task_lamps();         
    task_distanceSelector();        
    task_lightSensor();        
    task_displayDistance();   
    task_distanceSelect();
    cycle = cycle + 1;
    cycle_mod = cycle % 2;
    current_time = millis();
  } else {    
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed();
    task_lamps();
    task_distanceSelector();
    task_lightSensor();
    task_displayDistance();
    task_distanceSelect();
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
// Function: mode_1
// --------------------------------------
int mode_1() {
  if (cycle_mod != 0) {
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed(); 
    task_lamps();
    task_lightSensor();
    task_displayDistance();
    cycle = cycle + 1;
    cycle_mod = cycle % 2;
    current_time = millis();
  } else {
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed();
    task_lamps();
    task_lightSensor();
    task_displayDistance();
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
// Function: mode_2
// --------------------------------------
int mode_2() {
  if (cycle_mod != 0) {
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed(); 
    task_lamps();
    task_lightSensor();
    task_displayDistance();
    task_distanceSelect();
    cycle = cycle + 1;
    cycle_mod = cycle % 2;
    current_time = millis();
  } else {
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed();
    task_lamps();
    task_lightSensor();
    task_displayDistance();
    task_distanceSelect();
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
// Function: mode_3
// --------------------------------------
int mode_3() {
  if (cycle_mod != 0) {
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed(); 
    task_lamps();
    cycle = cycle + 1;
    cycle_mod = cycle % 2;
    current_time = millis();
  } else {
    task_gas();
    task_brake();
    task_mixer();
    task_slope();
    task_speed();
    task_lamps();
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
    pinMode(7, OUTPUT);  //LAMPS ON/OFF
    pinMode(6, INPUT);   //Validate distance
    pinMode(5, OUTPUT);  // D
    pinMode(4, OUTPUT);  // C
    pinMode(3, OUTPUT);  // B
    pinMode(2, OUTPUT);  // A
    time = millis();
}

// --------------------------------------
// Function: loop
// --------------------------------------
void loop() {
  if (executionMode == 0) mode_0();
  if (executionMode == 1) mode_1();
  if (executionMode == 2) mode_2();
  if (executionMode == 3) mode_3();
}
