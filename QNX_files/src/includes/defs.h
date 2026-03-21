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
// Haruns- This is the message i have for now but will later use FIELDS and type defs to have more range or we could always have them as null.
typedef struct 
{
    double percentage;
    double angle;
    int enabled;
}message ;

typedef struct 
{
    char subsys[32];
    char msg_type[32];
    message msg;
} msg_packet ;



// Haruns - Telemetry data struct for now
typedef struct{
    float speed;
    float throttle;
    float brake;
    float steering_angle;
    int   safe_mode;    // bool
    int   snow_mode;    // bool
    
}telemetry_msg;

//Haruns - The data to be received from py dashboard to be used (schema)
typedef struct {
    char type [32];
    telemetry_msg tel;
    float timestamp ;
}telemetry_packet;


void packet_init(msg_packet *c);
char *telemetry_to_json(telemetry_packet *t);
void json_to_msg_packet(const char *json_str, msg_packet *p);

// -------------------------------------------------------------------------------------------------------

#endif
