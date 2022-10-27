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
double speed2 = 55.0;
double accel = 0.0;; 
int brake = 0;
bool lam= false;
int gas = 0; 
int bright = 0;
bool mix = false; 
int slope = 0;
bool request_received = false;
bool requested_answered = false;
char request[MESSAGE_SIZE+1];
char answer[MESSAGE_SIZE+1];
unsigned long time_start; 
unsigned long time_end; 
unsigned long time_sleep; 
unsigned long MAX_TIME = (unsigned long) -1;
int pin_gas = 13 ;
int pin_brake = 12;
int pin_mix = 11;
int pin_speed = 10;
int pin_switch_1 = 9;
int pin_switch_2 = 8;
int pin_lam=7;
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
   speed2 = calc_speed();
   // If there is a request not answered, check if this is the one
   if ( request_received && !requested_answered && (0 == strcmp("SPD: REQ\n",request)) ) {
      // send the answer for speed request
      char num_str[5];
      dtostrf(speed2,4,1,num_str);
      sprintf(answer,"SPD:%s\n",num_str);
      // set request as answered
      requested_answered = true;
   }
   return 0;
}

int bright_req()
{
   // Calculo de la velocidad
   bright = random(0,100);
   // If there is a request not answered, check if this is the one
   if ( request_received && !requested_answered && (0 == strcmp("LIT: REQ\n",request)) ) {
      // send the answer for speed request
      char num_str[5];
      sprintf(num_str, "%d", bright);
      sprintf(answer,"LIT: %s%%\n",num_str);
      // set request as answered
      requested_answered = true;
   }
   return 0;
}
int lam_req(){
   //Si esta activo aceleracion - 0,5
   if(request_received && !requested_answered && 0 == strcmp("LAM: SET\n",request)){
     lam=true;
      digitalWrite(pin_lam, HIGH);
      sprintf(answer,"LAM:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("LAM: CLR\n",request)){
     lam=false;
      digitalWrite(pin_lam, LOW);
      sprintf(answer,"LAM:  OK\n");
      requested_answered = true;
   }
}



double calc_speed(){
   accel = gas * 0.5 - brake * 0.5 + 0.25 * (-slope);
   speed2 += accel * 0.2;
   
   analogWrite(pin_speed, map(speed2, 40, 70, 0, 255));
   return speed2; 
}


// --------------------------------------
// Se lee la pendiente y se responde
// --------------------------------------
int slope_req(){
   
   // Plano aceleracion + 0
   // Periodo de 10s
   // Cuesta arriba aceleracion - 0.25
   // Cuesta abajo aceleracion + 0.25
   if (request_received && !requested_answered && 0 == strcmp("SLP: REQ\n",request)){
     if (digitalRead(pin_switch_1) == LOW) {
        // Se enciende el LED:
        slope=-1;
        sprintf(answer,"SLP:DOWN\n"); 
      }
      else if (digitalRead(pin_switch_2) == LOW){
        // Se apaga el LED:
        slope=1;
       sprintf(answer,"SLP:  UP\n"); 
      }
      else { 
        slope=0;
        sprintf(answer,"SLP:FLAT\n");
      }
      
      requested_answered = true;
   }
  

}


// --------------------------------------
// Se activa o desactiva un led en funcion de las ordenes recibidas por el servidor
// --------------------------------------
int brake_req(){
   //Si esta activo aceleracion - 0,5
   if(request_received && !requested_answered && 0 == strcmp("BRK: SET\n",request)){
     brake=1;
      digitalWrite(pin_brake, HIGH);
      sprintf(answer,"BRK:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("BRK: CLR\n",request)){
     brake=0;
      digitalWrite(pin_brake, LOW);
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
      gas = 1;
      digitalWrite(pin_gas, HIGH);
      sprintf(answer,"GAS:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("GAS: CLR\n",request)){
      gas = 0;
      digitalWrite(pin_gas, LOW);
      sprintf(answer,"GAS:  OK\n");
      requested_answered = true;
   }
}


// --------------------------------------
// Se activa o desactiva un led en funcion de las ordenes recibidas por el servidor
//--------------------------------------
int mix_req(){

   if(request_received && !requested_answered && 0 == strcmp("MIX: SET\n",request)){
      mix = true;
      digitalWrite(pin_mix, HIGH);
      sprintf(answer,"MIX:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("MIX: CLR\n",request)){
      mix = false;
      digitalWrite(pin_mix, LOW);
      sprintf(answer,"MIX:  OK\n");
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
   pinMode(pin_gas, OUTPUT);
   pinMode(pin_brake, OUTPUT);
   pinMode(pin_mix, OUTPUT);
   pinMode(pin_speed, OUTPUT);
   pinMode(pin_switch_1, INPUT_PULLUP);
   pinMode(pin_switch_2, INPUT_PULLUP);
   time_start = millis();
}

// --------------------------------------
// Function: loop
// --------------------------------------
void loop()
{
   comm_server();
   speed_req();
   
   gas_req();
   brake_req();
   mix_req();
   slope_req();
   bright_req();
   lam_req();
   
   time_end = millis();
   
  
   if(time_start >= time_end){
     time_sleep = 200 - (MAX_TIME - time_start + time_end);
   }
   else {
      time_sleep = 200 - (time_end - time_start);
      
   }
   if (time_sleep <= 10 && time_sleep > 0){
      delayMicroseconds(time_sleep * 1000);
   }
   else if(time_sleep > 10){
     delay(time_sleep);
   }
   else{
     Serial.print("MSG: E2R\n");
     exit(-1);
   }
   
   time_start += 200; 
   
}
