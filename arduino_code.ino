// --------------------------------------
// Include files
// --------------------------------------
#include <string.h>
#include <stdio.h>
#include <Wire.h>

// --------------------------------------
// Global Constants
// --------------------------------------

#define MESSAGE_SIZE 8

// --------------------------------------
// Global Variables
// --------------------------------------
double speed = 55.0;
double accel = 0.0;; 
bool brake = false;
bool gas = false; 
bool mix = false; 
int slope = 0;
int pin_gas = 13; 
int pin_brake = 12; 
int pin_mix = 11;
// int pin swtich = 8 or 9; 
int pin_speed = 10; 
bool request_received = false;
bool requested_answered = false;
char request[MESSAGE_SIZE+1];
char answer[MESSAGE_SIZE+1];

// --------------------------------------
// Function: comm_server
// --------------------------------------
int comm_server()
{
   static int count = 0;
   char car_aux;

   // If there were a received msg, send the processed answer or ERROR if none.
   // then reset for the next request.
   // NOTE: this requires that between two calls of com_server all possible 
   //       answers have been processed.
   if (request_received) {
      // if there is an answer send it, else error
      if (requested_answered) {
          Serial.print(answer);
      } else {
          Serial.print("MSG: ERR\n");
      }  
      // reset flags and buffers
      request_received = false;
      requested_answered = false;
      memset(request,'\0', MESSAGE_SIZE+1);
      memset(answer,'\0', MESSAGE_SIZE+1);
   }

   while (Serial.available()) {
      // read one character
      car_aux =Serial.read();
        
      //skip if it is not a valid character
      if  ( ( (car_aux < 'A') || (car_aux > 'Z') ) &&
           (car_aux != ':') && (car_aux != ' ') && (car_aux != '\n') ) {
         continue;
      }
      
      //Store the character
      request[count] = car_aux;
      
      // If the last character is an enter or
      // There are 9th characters set an enter and finish.
      if ( (request[count] == '\n') || (count == 8) ) {
         request[count] = '\n';
         count = 0;
         request_received = true;
         break;
      }
      // Increment the count
      count++;
   }
}

// --------------------------------------
// Function: speed_req
// Intensidad del led en funcion de la velocidad
// Calculamos velocidad  
// --------------------------------------
int speed_req()
{
   // Calculo de la velocidad
   speed = calc_speed();
   // If there is a request not answered, check if this is the one
   if ( request_received && !requested_answered && (0 == strcmp("SPD: REQ\n",request)) ) {
      // send the answer for speed request
      char num_str[5];
      dtostrf(speed,4,1,num_str);
      sprintf(answer,"SPD:%s\n",num_str);
      // set request as answered
      requested_answered = true;
   }
   return 0;
}

double calc_speed(){
   /*
   double oldtime = 0;
   double time;
   double difftime;

   if (old_time == 0.0) {
        old_time = getClock();;
    }
   time = getclock();
   difftime = time - oldtime;
   */
   accel = 0;
   accel += 0.5 ? gas : 0; 
   accel -= 0.5 ? brake : 0;
   accel += 0.25 ? slope == -1 : 0;
   accel -= 0.25 ? slope == 1 : 0;
   speed += accel * 0.2;
   return speed; 
}

// --------------------------------------
// Se lee la pendiente y se responde
// --------------------------------------
int slope_req(){
   
   // Plano aceleracion + 0
   // Periodo de 10s
   // Cuesta arriba aceleracion - 0.25
   // Cuesta abajo aceleracion + 0.25



}


// --------------------------------------
// Se activa o desactiva un led en funcion de las ordenes recibidas por el servidor
// --------------------------------------
int brake_req(){
   //Si esta activo aceleracion - 0,5
   if(request_received && !requested_answered && 0 == strcmp("BRK: SET\n",request)){
      digitalWrite(pin_brake, HIGH);
      sprintf(answer,"BRK:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("BRK: CLR\n",request)){
      digitalrite(pin_brake, LOW);
      sprintf(answer,"BRK:  OK\n");
      requested_answered = true;
   }
}



// --------------------------------------
// Se activa o desactiva un led en funcion de las ordenes recibidas por el servidor
// --------------------------------------
int gas_req(){
   //Si esta activo aceleracion + 0,5
   if(request_received && !requested_answered && 0 == strcmp("GAS: SET\n",request)){
      gas = true;
      digitalWrite(pin_gas, HIGH);
      sprintf(answer,"GAS:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("GAS: CLR\n",request)){
      gas = false;
      digitalrite(pin_gas, LOW);
      sprintf(answer,"GAS:  OK\n");
      requested_answered = true;
   }
}




// --------------------------------------
// Se activa o desactiva un led en funcion de las ordenes recibidas por el servidor
//--------------------------------------
int mix_req(){

   if(request_received && !requested_answered && 0 == strcmp("GAS: SET\n",request)){
      mix = true;
      digitalWrite(pin_mix, HIGH);
      sprintf(answer,"GAS:OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("GAS: CLR\n",request)){
      mix = false;
      digitalrite(pin_mix, LOW);
      sprintf(answer,"GAS:OK\n");
      requested_answered = true;
   }
}



// --------------------------------------
// Function: setup
// --------------------------------------
void setup()
{
   // Setup Serial Monitor
   Serial.begin(9600);
   pinmode(pin_gas, OUTPUT);
   pinmode(pin_brake, OUTPUT);
   pinmode(pin_mix, OUTPUT);
   pinmode(pin_speed, OUTPUT);
   //pinmode(pin_switch, INPUT);
}

// --------------------------------------
// Function: loop
// --------------------------------------
void loop()
{
   comm_server();
   speed_req();
}
