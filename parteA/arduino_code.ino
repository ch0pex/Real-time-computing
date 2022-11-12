// --------------------------------------
// Include files
// --------------------------------------
#include <string.h>
#include <stdio.h>
#include <Wire.h>


// --------------------------------------
// Global Constants
// --------------------------------------

#define MESSAGE_SIZE 9

// --------------------------------------
// Global Variables
// --------------------------------------
double speed2 = 55.0;
double accel = 0.0;; 
int brake = 0;
int gas = 0; 
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

// PINS
int pin_gas = 13 ;
int pin_brake = 12;
int pin_mix = 11;
int pin_speed = 10;
int pin_switch_1 = 9;
int pin_switch_2 = 8;


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


double calc_speed(){
   accel = gas * 0.5 - brake * 0.5 + 0.25 * (-slope);
   speed2 += accel * 0.2;
   
   analogWrite(pin_speed, map(speed2, 40, 70, 0, 255));
   return speed2; 
}

// --------------------------------------
// Se lee la pendiente que viene indicada por el switch de tres posisiciones y se responde cuando se recibe la peticion
// por parte del controlador
// --------------------------------------
int slope_req(){
   if (request_received && !requested_answered && 0 == strcmp("SLP: REQ\n",request)){
     if (digitalRead(pin_switch_1) == LOW) {
         //Cuesta abajo 
        slope=-1;
        sprintf(answer,"SLP:DOWN\n"); 
      }
      else if (digitalRead(pin_switch_2) == LOW){
         //Cuesta arriba
         slope=1;
         sprintf(answer,"SLP:  UP\n"); 
      }
      else { 
         //Plano
         slope=0;
         sprintf(answer,"SLP:FLAT\n");
      }
      
      requested_answered = true;
   }
  

}


// --------------------------------------
// Se activa o desactiva el led del freno en funcion de las ordenes recibidas por el servidor
// --------------------------------------
int brake_req(){
   if(request_received && !requested_answered && 0 == strcmp("BRK: SET\n",request)){
      //Activar freno (encender led) 
      brake=1;
      digitalWrite(pin_brake, HIGH);
      sprintf(answer,"BRK:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("BRK: CLR\n",request)){
      //Desactivar freno (apagar led) 
      brake=0;
      digitalWrite(pin_brake, LOW);
      sprintf(answer,"BRK:  OK\n");
      requested_answered = true;
   }
}



// --------------------------------------
// Se activa o desactiva el led del acelerador en funcion de las ordenes recibidas por el servidor
// --------------------------------------
int gas_req(){
   if(request_received && !requested_answered && 0 == strcmp("GAS: SET\n",request)){
      // Activar acelerador (encender led)
      gas = 1;
      digitalWrite(pin_gas, HIGH);
      sprintf(answer,"GAS:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("GAS: CLR\n",request)){
      // Desactivar acelerador (apagar led)
      gas = 0;
      digitalWrite(pin_gas, LOW);
      sprintf(answer,"GAS:  OK\n");
      requested_answered = true;
   }
}


// --------------------------------------
// Se activa o desactiva el led del mixer en funcion de las ordenes recibidas por el servidor
//--------------------------------------
int mix_req(){
   if(request_received && !requested_answered && 0 == strcmp("MIX: SET\n",request)){
      //Activar mix (encender led)
      mix = true;
      digitalWrite(pin_mix, HIGH);
      sprintf(answer,"MIX:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("MIX: CLR\n",request)){
      //Desactivar mix (apagar led)
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
   pinMode(pin_gas, OUTPUT); // Acelerador
   pinMode(pin_brake, OUTPUT); // Freno
   pinMode(pin_mix, OUTPUT); // Mixer
   pinMode(pin_speed, OUTPUT); // Velocidad 
   pinMode(pin_switch_1, INPUT_PULLUP);  // Palanca slope
   pinMode(pin_switch_2, INPUT_PULLUP); // Palanca slope
   time_start = millis();
}

// --------------------------------------
// Function: loop
// --------------------------------------
void loop()
{
   //Se ejecutan las tareas del plan de ejecucion (solo un CS). 
   comm_server(); 
   speed_req();
   gas_req();
   brake_req();
   mix_req();
   slope_req();

   // Se calcula el tiempo que debe dormir
   time_end = millis();
   if(time_start >= time_end){ // Tiempo de inicio mayor que tiempo fin
     time_sleep = 200 - (MAX_TIME - time_start + time_end);
   }
   else { // Tiempo de inicio menor que tiempo de fin
      time_sleep = 200 - (time_end - time_start); 
   }
   if (time_sleep <= 10 && time_sleep > 0){ // DelayMicro si hay que dormir menos de 10ms
      delayMicroseconds(time_sleep * 1000);
   }
   else if(time_sleep > 10){ //Delay si hay que dormir mas de 10ms
      delay(time_sleep);
   }
   else{ // Caso de error. Si la ejecucion dura mas de 200ms time_sleep es < 0, se reinicia arduino.
     exit(-1);
   }  
   time_start += 200; // Sumamos tiempo teorico a time_start
   
}
