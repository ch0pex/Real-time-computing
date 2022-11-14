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
int bright = 0; 
int slope = 0;
bool movimiento = true;
bool request_received = false;
bool requested_answered = false;
bool emergency = false;
char request[MESSAGE_SIZE+1];
char answer[MESSAGE_SIZE+1];
unsigned long time_start; 
unsigned long time_end; 
unsigned long time_sleep; 
unsigned long MAX_TIME = (unsigned long) -1;

//Variables necesarias para la funcion de elegir distancia
int display_a = 2;
int display_b = 3;
int display_c = 4;
int display_d = 5;
double valor_pot;
double valorMapeado_pot;
int valorDisplay;
const int  buttonPin = 6; 
long distance = 99999;
int buttonState = 0;        
int lastButtonState = 0;


// --------------------------------------
// PINS
// --------------------------------------
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
// Se calcula la velocidad y se regula la intensidad del led en funcion a esta 
// --------------------------------------
double calc_speed(){
   accel = gas * 0.5 - brake * 0.5 + 0.25 * (-slope);
   speed2 += accel * 0.2;
   if(speed2>40){
     analogWrite(pin_speed, map(speed2, 40, 70, 0, 255));
   }
   else{
     analogWrite(pin_speed, 0);
   }
   if(speed2 <= 0){
     speed2 = 0;
   }
   return speed2; 
}



// --------------------------------------
// Se responde al servidor con la velocidad actual de la carretilla  
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



// --------------------------------------
// Funcion para leer el brillo de la fotoresistencia
// --------------------------------------
int read_bright(){
   bright = map(analogRead(A0), 0, 1023, 0, 100);
}



// --------------------------------------
// Se recibe una peticion para devolver el brillo leido
// --------------------------------------
int bright_req()
{
   // Calculo de la velocidad
   read_bright();
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



// --------------------------------------
// Se recibe una peticion para encender o apagar los focos
// --------------------------------------
int lam_req(){
   //Si esta activo aceleracion - 0,5
   if(request_received && !requested_answered && 0 == strcmp("LAM: SET\n",request)){
     
      digitalWrite(pin_lam, HIGH);
      sprintf(answer,"LAM:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("LAM: CLR\n",request)){
     
      digitalWrite(pin_lam, LOW);
      sprintf(answer,"LAM:  OK\n");
      requested_answered = true;
   }
}



// --------------------------------------
// Se lee la pendiente y se responde
// --------------------------------------
int slope_req(){
   if (request_received && !requested_answered && 0 == strcmp("SLP: REQ\n",request)){
     if (digitalRead(pin_switch_1) == LOW) {
        // Switch puesto en cuesta abajo 
        slope=-1;
        sprintf(answer,"SLP:DOWN\n"); 
      }
      else if (digitalRead(pin_switch_2) == LOW){
        // Switch puesto en cuesta arriba 
        slope=1;
       sprintf(answer,"SLP:  UP\n"); 
      }
      else { 
        // Switch puesto en plano
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
      // Se activa el freno (encender led)
      brake=1;
      digitalWrite(pin_brake, HIGH);
      sprintf(answer,"BRK:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("BRK: CLR\n",request)){
      // Se desactiva el freno (apagar led)
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
   if(request_received && !requested_answered && 0 == strcmp("GAS: SET\n",request)){
      // Se activa el acelerador (encender led)
      gas = 1;
      digitalWrite(pin_gas, HIGH);
      sprintf(answer,"GAS:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("GAS: CLR\n",request)){
      // Se desactiva el acelerador (apagar led)
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
      // Se activa el mixer (encender led)
      digitalWrite(pin_mix, HIGH);
      sprintf(answer,"MIX:  OK\n");
      requested_answered = true;
   }
   else if (request_received && !requested_answered && 0 == strcmp("MIX: CLR\n",request)){
      // Se desactiva el mixer (encender led)
      digitalWrite(pin_mix, LOW);
      sprintf(answer,"MIX:  OK\n");
      requested_answered = true;
      
   }
}



// --------------------------------------
// Cuando recibe una peticion por parte del servidor, responde con el estado de movimiento actual de la carretilla
//--------------------------------------
int move_req()
{
   if ( request_received && !requested_answered && (0 == strcmp("STP: REQ\n",request)) ) {
      if (movimiento == false){
        // si la carretilla esta parada responde STOP 
        sprintf(answer,"STP:STOP\n");
      }
      else{
        // si la carretilla se mueve responde GO 
        sprintf(answer,"STP:  GO\n");
      }
      requested_answered = true;
   }
   return 0;
}




//--------------------------------------
// Funcion que regula el display tanto para el PLAN 1 como para el PLAN 2
//--------------------------------------
int show_distance(bool modo){ //cuando le llegue 0 es que esta en el plan 1 y la distancia no se mueve
  //Se determina el valor del display
  if (modo==false){
  valorDisplay = map(valorMapeado_pot, 10000, 90000, 1, 9);
  }
  else {
    distance = distance - (speed2*0.2) + (0.5*accel*0.2*0.2);
    valorDisplay = map(distance, 0, 90000, 0, 9);
    
  }
  //Imprimimos por el monitor serie
  switch (valorDisplay){
    case 0:
      digitalWrite(display_a,LOW);
      digitalWrite(display_b,LOW);
      digitalWrite(display_c,LOW);
      digitalWrite(display_d,LOW);
      break;
    case 1:
      digitalWrite(display_a,HIGH);
      digitalWrite(display_b,LOW);
      digitalWrite(display_c,LOW);
      digitalWrite(display_d,LOW);
      break;
    case 2:
      digitalWrite(display_a,LOW);
      digitalWrite(display_b,HIGH);
      digitalWrite(display_c,LOW);
      digitalWrite(display_d,LOW);
      break;
    case 3:
      digitalWrite(display_a,HIGH);
      digitalWrite(display_b,HIGH);
      digitalWrite(display_c,LOW);
      digitalWrite(display_d,LOW);
      break;
    case 4:
      digitalWrite(display_a,LOW);
      digitalWrite(display_b,LOW);
      digitalWrite(display_c,HIGH);
      digitalWrite(display_d,LOW);
      break;
      
    case 5:
      digitalWrite(display_a,HIGH);
      digitalWrite(display_b,LOW);
      digitalWrite(display_c,HIGH);
      digitalWrite(display_d,LOW);
      
      break;
    case 6:
      digitalWrite(display_a,LOW);
      digitalWrite(display_b,HIGH);
      digitalWrite(display_c,HIGH);
      digitalWrite(display_d,LOW);
      break;
      
    case 7:
      digitalWrite(display_a,HIGH);
      digitalWrite(display_b,HIGH);
      digitalWrite(display_c,HIGH);
      digitalWrite(display_d,LOW);
      
      break;
    case 8:
      digitalWrite(display_a,LOW);
      digitalWrite(display_b,LOW);
      digitalWrite(display_c,LOW);
      digitalWrite(display_d,HIGH);
      
      break;
    case 9:
      digitalWrite(display_a,HIGH);
      digitalWrite(display_b,LOW);
      digitalWrite(display_c,LOW);
      digitalWrite(display_d,HIGH);
      break;
  }
}



//--------------------------------------
// Funcion para seleccionar la distancia en el PLAN 1
//--------------------------------------
double select_distance()
{
  valor_pot = analogRead(A1);
  valorMapeado_pot = map(valor_pot, 0, 1023, 10000, 90000);
    
  
}  



//--------------------------------------
// Funcion del boton en PLAN 1, una vez elegida la distancia con el potenciometro 
// se pulsa el boton y se cambia de plan 
//--------------------------------------
int button_press()
{
      buttonState = digitalRead(buttonPin);     
      if (buttonState != lastButtonState) {
        if (buttonState == HIGH) {
          if(movimiento = true){
            distance = valorMapeado_pot;
          }
        } 
      lastButtonState = buttonState;
    
        
    }
}



//--------------------------------------
// Funcion del boton en PLAN 2, reactiva el movimiento y pone a 99999 la distancia
//--------------------------------------
int button_press2()
{
      buttonState = digitalRead(buttonPin);
      if (buttonState != lastButtonState) {
        if (buttonState == HIGH) {        
          if (movimiento==false){
            movimiento = true;
            distance = 99999;
          }
        } 
      lastButtonState = buttonState;
    }
}



//--------------------------------------
// Funcion que devuelve la distancia a la que se encuentra la carretilla cuando recibe una peticion por parte del servidor
//--------------------------------------
int distance_req()
{
   if ( request_received && !requested_answered && (0 == strcmp("DS:  REQ\n",request)) ) {
      char num_str[5];
      dtostrf(distance,5,0,num_str);
      sprintf(answer,"DS:%s\n",num_str);
      requested_answered = true;
   }
   return 0;
}



//--------------------------------------
// Recibe la peticion de modo de emergencia por parte del controlador y la act
//--------------------------------------
int emergency_req()
{
  if ( request_received && !requested_answered && (0 == strcmp("ERR: SET\n",request)) ) {
      emergency = true;
      sprintf(answer,"ERR:  OK\n");
      requested_answered = true;
   }
   
   return 0;
}


// --------------------------------------
// Function: setup
// --------------------------------------
void setup()
{
   // Setup Serial Monitor
   Serial.begin(9600);
   pinMode(pin_gas, OUTPUT); // Pin acelerador 
   pinMode(pin_brake, OUTPUT); // Pin freno 
   pinMode(pin_mix, OUTPUT); // Pin mixer 
   pinMode(pin_speed, OUTPUT); // Pin velocidad
   pinMode(pin_switch_1, INPUT_PULLUP); // Pin switch slope
   pinMode(pin_switch_2, INPUT_PULLUP); // Pin switch slope
   pinMode(display_a,OUTPUT); // Pin display
   pinMode(display_b,OUTPUT); // Pin display
   pinMode(display_c,OUTPUT); // Pin display
   pinMode(display_d,OUTPUT); // Pin display
   pinMode(buttonPin, INPUT); // Pin boton 
  time_start = millis(); // Primer tiempo de inicio 
}



// --------------------------------------
// PLAN 1: Modo seleccion de distancia 
// --------------------------------------
int plan1(){
   // Ejecucion de tareas del plan de ejecucion (solo un CS)
   comm_server();
   emergency_req();
   speed_req();
   gas_req();
   brake_req();
   mix_req();
   slope_req();
   bright_req();
   lam_req();
   select_distance(); 
   distance_req();
   show_distance(0);
   button_press();
   move_req();

   // Se calcula el tiempo que se ha de dormir 
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
     exit(-1);
   }
   
   time_start += 200;  // Sumamos tiempo teorico al tiempo de inicio del ciclo
   
   if (emergency == true){ // Si el modo de emergencia ha sido activado se cambia al plan 4 
     return 4;
   }
   
   if(distance==99999){ // Si no se ha seleccionado una distnacia permanecemos en el modo de seleccion de distancia 
     return 1;
   }
   else{ // Una vez seleccionada la distancia se cambia de modo de ejecucion 
     return 2; 
   }
}



// --------------------------------------
// PLAN 2: Modo de acercamiento al deposito  
// --------------------------------------
int plan2(){
   // Ejecucion de tareas del plan de ejecucion (solo un CS)
   comm_server();
   emergency_req();
   speed_req();
   gas_req();
   brake_req();
   mix_req();
   slope_req();
   bright_req();
   lam_req();
   show_distance(1);
   distance_req();
   move_req();

   // Se calcula el tiempo que se ha de dormir 
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
     exit(-1);
   }
   
   time_start += 200;  // Sumamos tiempo teorico al tiempo de inicio 
   
   if (emergency == true){ 
     // Si el modo de emergencia ha sido activado se cambia al plan 4 
     return 4;
   }
   
   if(distance<=0 && speed2<=10){
     // Si la carretilla esta en el punto de parada y su velocidad es inferior a 10ms se cambia al modo de  parada 
     movimiento = false;
     distance=0;
     return 3;
   }
   else if(distance<=0 && speed2>10){
     /* Si la carretilla esta en el punto de parada y su velocidad es mayor de 10ms la parada a fallado
     y se cambia al modo de seleccion de distancia */
     distance=99999;
     return 1;
   }
   return 2;
}



// --------------------------------------
// PLAN 3: Modo de parada 
// --------------------------------------
int plan3(){
   // Ejecucion de tareas del plan de ejecucion (solo un CS)
   comm_server();
   emergency_req();
   speed_req();
   gas_req();
   brake_req();
   mix_req();
   slope_req();
   bright_req();
   lam_req();
   button_press2();
   distance_req();
   move_req();
   
   // Se calcula el tiempo que se ha de dormir 
   time_end = millis();
   if (movimiento==false){
       speed2=0;
   }
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
     exit(-1);
   }
   
   time_start += 200; // Se suma el tiempo teorico al tiempo de inicio 

   if (emergency == true){
     // Si el modo de emergencia ha sido activado se cambia al plan 4 
     return 4;
   }
   if(movimiento == true){
     // Si la carretilla se mueve cambiamos al plan de seleccion de distancia 
     return 1;
   }
   return 3;
   
}



// --------------------------------------
// PLAN 3: Modo de emergencia 
// --------------------------------------
int plan4(){
   // Ejecucion de tareas del plan de ejecucion (solo un CS)
   comm_server();
   speed_req();
   gas_req();
   brake_req();
   mix_req();
   slope_req();
   lam_req();
  
   // Se calcula el tiempo que se ha de dormir
   time_end = millis();
   if (movimiento==false){
       speed2=0;
   }
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
     exit(-1);
   }
   
   time_start += 200; // Se suma el tiempo teorico al tiempo de inicio 

   return 4;
   
}



// --------------------------------------
// Function: loop
// --------------------------------------
void loop()
{
  static int plan = 1;
    

    switch (plan){
    /*Se ejecuta el plan de ejecucion correspondiente en en cada ciclo.
    Los propios planes devuelven el valor que determina si se cambia de plan o no*/ 
		case 1:
			plan = plan1(); // Modo de seleccion de distancia 
			break;
		case 2:
			plan = plan2(); // Modo de acercamiento  al desposito
			break;
    case 3:
			plan = plan3();  // Modo de parada 
			break;
		default:
			plan = plan4(); // Modo de emergencia 
			break;
    }
    
   
}
