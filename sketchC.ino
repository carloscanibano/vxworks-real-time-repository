// --------------------------------------
// Global Variables
// --------------------------------------
#include <string.h>
#include <stdio.h>

// --------------------------------------
// Global Variables
// --------------------------------------
unsigned long main_cycle = 200;
unsigned long secondary_cycle = 100;
unsigned long executed_time;
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
int distanceSelector = 0; // Potentiometer value (distance)
int selectedDistance = 0; // Used distance value
int distanceDifference = 0; //Numeric checking
int currentDistance = 0; // Distante left to arrive at final destination
int executionMode = 0; // 0 = Distance selector, 1 = Tank approach, 2 = Stop mode
int buttonMode = 0; // 0 = OFF, 1 = ON
unsigned long time;
unsigned long current_time;
unsigned long distance_time;
unsigned long current_distance_time;

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
            int converter = map(darkness, 0, 255, 00, 99);
            sprintf(answer,"LIT: %02d%%\n",converter);
            Serial.print(answer);
        } else if (0 == strcmp("LAM: REQ\n",request)) {
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
            if (executionMode == 2) {
              sprintf(answer, "DS:STOP\n");
            } else if (executionMode == 1) {
              sprintf(answer,"DS:%05d\n", distanceDifference);  
            } else {
              sprintf(answer, "DS:00000\n"); 
            }
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
  if (executionMode == 2) {
    speed = 0.0;
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
    }

    speed = speed + (accel * 0.1);
    analogWrite(10, map(speed, 40, 70, 0, 255));
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
  distanceSelector = map(analogRead(A1), 0, 1022, 1, 10);
  return 0;
}

// --------------------------------------
// Function: task_displayDistance
// --------------------------------------
int task_displayDistance() {
  int digit;
  
  if (executionMode == 0) {
    digit = distanceSelector;
  } else if (executionMode == 1) {
    // x = x0 + (v0 * t) + (1/2 * a * t^2)
    currentDistance += (speed * 0.1) + (0.5 * accel * pow(0.1, 2));
    distanceDifference = (selectedDistance - currentDistance);
    if (distanceDifference >= 10000) {
      digit = distanceDifference / 10000;
    } else {
      digit = 1;
    }

    if (distanceDifference <= 0) {
      if (speed > 10.0) {
        executionMode = 0;
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
  if (digitalRead(6) == HIGH) {
    buttonMode = 1;
  } else if (buttonMode == 1) {
    if (executionMode == 0) {
      selectedDistance = distanceSelector * 10000;
      //selectedDistance = 100;
      distanceSelector = 0;
      executionMode = 1;
    } else {
      selectedDistance = 0;
      executionMode = 0;
    }
    buttonMode = 0;
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
      if (cycle_mod != 0) {
      task_gas();
      task_brake();
      task_mixer();
      task_slope();
      task_speed();
      task_lightSensor();
      task_lamps();
      if (executionMode == 0) task_distanceSelector();
      task_displayDistance();
      if ((executionMode == 0) || (executionMode == 2)) task_distanceSelect();
      current_time = millis();
      cycle = cycle + 1;
      cycle_mod = cycle % 2;
    } else {
      task_gas();
      task_brake();
      task_mixer();
      task_slope();
      task_speed();
      task_lightSensor();
      task_lamps();
      if (executionMode == 0) task_distanceSelector();
      task_displayDistance();
      if ((executionMode == 0) || (executionMode == 2)) task_distanceSelect();
      comm_server();
      current_time = millis();
      cycle = cycle + 1;
      cycle_mod = cycle % 2;
    }

    executed_time = current_time - time;
    delay(secondary_cycle - executed_time);
    time = time + secondary_cycle;
}
