#ifndef DEFS_H      // "have I seen this file before?"
#define DEFS_H    

//Connection relevant libraries used 

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include "includes/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <sys/dispatch.h>



//Networking
#define LISTEN_PORT 5000
#define SEND_PORT   6000

// my ip address

#define DEST_IP     "192.168.56.1"

//This is my IP : lol this has to be the worst practice ever lol


//Telemetry aids

#define SUBSYS_BRAKE    0
#define SUBSYS_THROTTLE 1
#define SUBSYS_STEERING 2
#define SUBSYS_TELEMETRY 3

// define these first
typedef struct { float speed; float brake_level; } BrakeUpdate;
typedef struct { float value; } ThrottleUpdate;
typedef struct { float angle; } SteeringUpdate;

// NOW ProcessMsg works
typedef struct {
    int subsys;
    union {
        BrakeUpdate    brake;
        ThrottleUpdate throttle;
        SteeringUpdate steering;
    } data;
} ProcessMsg;

extern int sockfd;
extern struct sockaddr_in dest;

void socket_setup();
void receiver_setup();
void sender_setup();

void* send_loop(void* arg);
void* recv_loop(void* arg);


//Pulse Codes
typedef enum {
    PULSE_WATCHDOG_AUDIT = _PULSE_CODE_MINAVAIL + 1,
    PULSE_BRAKING_ALIVE,
    PULSE_BRAKING_INTERNAL,
    PULSE_TELEMETRY_INTERNAL,   // add
    PULSE_TELEMETRY_ALIVE,      // add
    PULSE_STEERING_ALIVE,       // add for future
    PULSE_THROTTLE_ALIVE        // add for future
} SystemPulseCodes;
//Type defs / ill clean up once i get tests done 

// Data_parser relevant files 
// -------------------------------------------------------------------------------------------------------

// Haruns- this is for the data coming in to qnx via udp . Webots and Dashboard schema
typedef struct 
{
    double percentage;
    double angle;
    int enabled;
    double speed;        // current speed from sim in webots

}message ;

typedef struct 
{
    char origin[32];    //Stores the origin [ Userinput | VehicleInput ]
    char subsys[32];
    // char msg_type[32]; //No longer need this 
    message msg;
} msg_packet ;



// Haruns - Telemetry data struct for now

typedef struct{
    float speed;
    int   snow_mode;    // bool
    char warnings [10][64];  
    int   warning_count;
    
}telemetry_msg;

//Haruns - The data to be sent to py dashboard to be used for display(schema)
typedef struct {
    char type [32];
    telemetry_msg tel;
    // float timestamp ;  //Not needed anymore
}telemetry_packet;


// webot sim data
typedef struct 
{
    char* subsys  [32];         // contains name of subsystem for that data
    vehicle_controls_data data;
}vehicle_controls;

typedef struct 
{
    double throttle_level;    // [  0 , 1]
    double brake_level;       // [  0 , 1]
    double steering_level;    // [ -1 , 1]
    char* toggleGear [2];         //[ D = 0 , R = 1]
}vehicle_controls_data;



void packet_init(msg_packet *c);
void vc_init(vehicle_controls *vc);
char *telemetry_to_json(telemetry_packet *t);
void json_to_msg_packet(const char *json_str, msg_packet *p);
char* sim_data_to_json(vehicle_controls *v);

// -------------------------------------------------------------------------------------------------------

#endif
