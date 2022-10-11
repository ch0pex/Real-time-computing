//-Uncomment to compile with arduino support
//#define ARDUINO

//-------------------------------------
//-  Include files
//-------------------------------------
#include <termios.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <time.h>

#include <rtems.h>
#include <rtems/termiostypes.h>
#include <bsp.h>

#include "displayA.h"

//-------------------------------------
//-  Constants
//-------------------------------------
#define MSG_LEN 9 //8?
#define CICLO_SEC 10.0
//#define SLAVE_ADDR 0x8

//-------------------------------------
//-  Global Variables
//-------------------------------------
double speed = 0.0;
bool brake = false;
bool gas = false; 
bool mix = false; 
int slope = 0;

struct timespec time_msg = {0,400000000};

int fd_serie = -1;

//-------------------------------------
//-  Function: read_msg
//-------------------------------------
int read_msg(int fd, char *buffer, int max_size)
{
    char aux_buf[MSG_LEN+1];
    int count=0;
    char car_aux;

    //clear buffer and aux_buf
    memset(aux_buf, '\0', MSG_LEN+1);
    memset(buffer, '\0', MSG_LEN+1);

    while (1) {
        car_aux='\0';
        read(fd_serie, &car_aux, 1);
        // skip if it is not valid character
        if ( ( (car_aux < 'A') || (car_aux > 'Z') ) &&
             ( (car_aux < '0') || (car_aux > '9') ) &&
               (car_aux != ':')  && (car_aux != ' ') &&
               (car_aux != '\n') && (car_aux != '.') &&
               (car_aux != '%') ) {
            continue;
        }
        // store the character
        aux_buf[count] = car_aux;

        //increment count in a circular way
        count = count + 1;
        if (count == MSG_LEN) count = 0;

        // if character is new_line return answer
        if (car_aux == '\n') {
           int first_part_size = strlen(&(aux_buf[count]));
           memcpy(buffer,&(aux_buf[count]), first_part_size);
           memcpy(&(buffer[first_part_size]),aux_buf,count);
           return 0;
        }
    }
    strncpy(buffer,"MSG: ERR\n",MSG_LEN);
    return 0;
}

//-------------------------------------
//-  Function: task_speed
//-------------------------------------
int task_speed()
{
    char request[MSG_LEN+1];
    char answer[MSG_LEN+1];

    //--------------------------------
    //  request speed and display it
    //--------------------------------

    //clear request and answer
    memset(request, '\0', MSG_LEN+1);
    memset(answer, '\0', MSG_LEN+1);

    // request speed
    strcpy(request, "SPD: REQ\n");

#if defined(ARDUINO)
    // use UART serial module
    write(fd_serie, request, MSG_LEN);
    nanosleep(&time_msg, NULL);
    read_msg(fd_serie, answer, MSG_LEN);
#else
    //Use the simulator
    simulator(request, answer);
#endif

    // display speed
    if (1 == sscanf (answer, "SPD:%f\n", &speed)){
        displaySpeed(speed);
    }
    return 0;
}

//-------------------------------------
//-  Function: task_slope
//-------------------------------------
int task_slope()
{
    char request[MSG_LEN+1];
    char answer[MSG_LEN+1];

    //--------------------------------
    //  request slope and display it
    //--------------------------------

    //clear request and answer
    memset(request,'\0',MSG_LEN+1);
    memset(answer,'\0',MSG_LEN+1);

    // request slope
    strcpy(request, "SLP: REQ\n");

#if defined(ARDUINO)
    // use UART serial module
    write(fd_serie, request, MSG_LEN);
    nanosleep(&time_msg, NULL);
    read_msg(fd_serie, answer, MSG_LEN);
#else
    //Use the simulator
    simulator(request, answer);
#endif
    // display slope
    if (0 == strcmp(answer, "SLP:DOWN\n")) displaySlope(-1);
    if (0 == strcmp(answer, "SLP:FLAT\n")) displaySlope(0);
    if (0 == strcmp(answer, "SLP:  UP\n")) displaySlope(1);
    return 0;
}

//-------------------------------------
// A partir de los datos recibidos determina si activar o no el freno ( en funcion de la velocidad y la cuesta)
//-------------------------------------
int task_brake()
{   
    char request[MSG_LEN+1];
    char answer[MSG_LEN+1];

    memset(request,'\0',MSG_LEN+1);
    memset(answer,'\0',MSG_LEN+1);

     
    if(speed >= 56 && brake == false){
         //activar freno   
        strcpy(request, "BRK: SET\n");
    }
    if (speed < 56 && brake == true){
        //desactivar freno
        strcpy(request, "BRK: CLR\n");
    }

    if (0 == strcmp(request, "BRK: SET\n") || 0 == strcmp(request, "BRK: CLR\n")) {
#if defined(ARDUINO)
    // use UART serial module
        write(fd_serie, request, MSG_LEN);
        nanosleep(&time_msg, NULL);
        read_msg(fd_serie, answer, MSG_LEN);
#else
    //Use the simulator
        simulator(request, answer);
#endif
    }
    //
    if (0 == strcmp(answer, "BRK:  OK\n")) {
        brake = false ? brake : true;
        displayBrake(brake);
    }
    return 0; 
    //dispalyBrake(int brake);
}

//-------------------------------------
// A partir de los datos recibidos determina si activar o no el acelerador ( en funcion de la velocidad y la cuesta)
//-------------------------------------
int task_gas()
{
    char request[MSG_LEN+1];
    char answer[MSG_LEN+1];

    memset(request,'\0',MSG_LEN+1);
    memset(answer,'\0',MSG_LEN+1);

     
    if(speed <= 54 && gas == false){
         //activar acelerador   
        strcpy(request, "GAS: SET\n");
    }
    if (speed > 54 && gas == true){
        //desactivar acelerador
        strcpy(request, "GAS: CLR\n");
    }


    if (0 == strcmp(request, "GAS: SET\n") || 0 == strcmp(request, "GAS: CLR\n")) {
#if defined(ARDUINO)
        write(fd_serie, request, MSG_LEN);
        nanosleep(&time_msg, NULL);
        read_msg(fd_serie, answer, MSG_LEN);
#else   
    //Use the simulator
        simulator(request, answer);
#endif
    }
    //
    if (0 == strcmp(answer, "GAS:  OK\n")) {
        gas = false ? gas : true;
        displayBrake(gas);
    }
    return 0; 
    //displayGas(int gas);
}

//-------------------------------------
// A partir de los datos recibidos determina si girar o no el mezclador ()
int task_mixer()
{
    char request[MSG_LEN+1];
    char answer[MSG_LEN+1];
    static double timeMix = -1;
    static double oldTime;
    static double time;  
    memset(request,'\0',MSG_LEN+1);
    memset(answer,'\0',MSG_LEN+1);

    time = getclock();
    if (timeMix != -1){
        timeMix +=  time - oldTime; 
    }
    oldTime = time;


    if (timeMix > 30 || timeMix == -1 && mix == false){
        //girar mezclador
        timeMix = 0;
        mix = true; 
        strcpy(request, "MIX: SET\n");
        
    }
    else if( timeMix > 30 && mix == true){
        //parar mezclador
        timeMix = 0;
        mix = false;
        strcpy(request, "MIX: CLR\n");
    }


    if (0 == strcmp(request, "MIX: SET\n") || 0 == strcmp(request, "MIX: CLR\n")) {
#if defined(ARDUINO)
    // use UART serial module
        write(fd_serie, request, MSG_LEN);
        nanosleep(&time_msg, NULL);
        read_msg(fd_serie, answer, MSG_LEN);
#else
    //Use the simulator
        simulator(request, answer);
#endif
    }
    //
    if (0 == strcmp(answer, "MIX:  OK\n")) {
        mix = false ? mix : true;
        displayBrake(mix);
    }
    return 0; 

    //displayMix(int mixer);
}


//-------------------------------------
//-  Function: controller
//-------------------------------------
void *controller(void *arg)
{
    static clock_t tiempo_inicio, tiempo_final;
    static double tiempo;
    static struct timespec seg_sleep = {0,0};
    static int seg=0;
    static int nseg=0;
    while(1) {
        // calling task of speed
        for(int i=0;i<3;i++){
            tiempo_inicio = clock();

            task_slope();
            task_speed();
            task_gas();
            task_brake();
            if (i==0 || i==2){
                task_mixer();
            } 
            tiempo_final = clock();
            difference = time_cycle - time_cycle_fin;

            tiempo = CICLO_SEC - (double)(tiempo_final - tiempo_inicio) / CLOCKS_PER_SEC; /*según que estes midiendo el tiempo en segundos es demasiado grande*/
            seg = (int)tiempo;
            nseg =(int)(1000000000*(tiempo - seg));
            seg_sleep = {seg,nseg};
            nanosleep(&seg_sleep, NULL);
           
    }
        // calling task of slope
        
    }
}

//-------------------------------------
//-  Function: Init
//-------------------------------------
rtems_task Init (rtems_task_argument ignored)
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

#if defined(ARDUINO)
    /* Open serial port */
    char serial_dev[]="/dev/com1";
    fd_serie = open (serial_dev, O_RDWR);
    if (fd_serie < 0) {
        printf("open: error opening serial %s\n", serial_dev);
        exit(-1);
    }

    struct termios portSettings;
    speed_t speed=B9600;

    tcgetattr(fd_serie, &portSettings);
    cfsetispeed(&portSettings, speed);
    cfsetospeed(&portSettings, speed);
    cfmakeraw(&portSettings);
    tcsetattr(fd_serie, TCSANOW, &portSettings);
#endif

    /* Create first thread */
    pthread_create(&thread_ctrl, NULL, controller, NULL);
    pthread_join (thread_ctrl, NULL);
    exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_MAXIMUM_TASKS 1
#define CONFIGURE_MAXIMUM_SEMAPHORES 10
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 30
#define CONFIGURE_MAXIMUM_DIRVER 10
#define CONFIGURE_MAXIMUM_POSIX_THREADS 2
#define CONFIGURE_MAXIMUM_POSIX_TIMERS 1

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
