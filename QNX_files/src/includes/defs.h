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



//Networking ///
#define LISTEN_PORT 5000
#define SEND_PORT   6000

#define DEST_IP     "192.168.56.1"

extern int sockfd;
extern struct sockaddr_in dest;

void socket_setup();
void receiver_setup();
void sender_setup();

void* send_loop(void* arg);
void* recv_loop(void* arg);



// Dashboard -> QNX Messaging
typedef struct { float speed; float brake_level; } BrakeUpdate;
typedef struct { float value; } ThrottleUpdate;
typedef struct { float angle; } SteeringUpdate;


typedef struct {
    int subsys;
    union {
        BrakeUpdate    brake;
        ThrottleUpdate throttle;
        SteeringUpdate steering;
    } data;
} ProcessMsg;


//Pulse Codes
typedef enum {
    PULSE_WATCHDOG_AUDIT = _PULSE_CODE_MINAVAIL + 1,
    PULSE_SUBSYSTEM_INTERNAL,
    PULSE_SUBSYSTEM_ALIVE,     
    PULSE_CHAOSMODE,

} SystemPulseCodes;


/// Subsystem Info

//response times : how often we expect them to check in * 1000 for debug
#define SYS_BRAKING_RESPONSETIME_MS 5000
#define SYS_DRIVE_RESPONSETIME_MS 3000
#define SYS_CLIENT_RESPONSETIME_MS 1000
#define SYS_TELEMETRY_RESPONSETIME_MS 2000

//critical times : how many MS since last check in such that we kill and restart process
#define SYS_BRAKING_CRITICALTIME_MS 10000 
#define SYS_DRIVE_CRITICALTIME_MS 6000
#define SYS_CLIENT_CRITICALTIME_MS 2000 
#define SYS_TELEMETRY_CRITICALTIME_MS 4000 


typedef enum {
    SUBSYS_BRAKE=0,
    SUBSYS_DRIVE=1,
    SUBSYS_STEERING=2,
    SUBSYS_TELEMETRY=3,
    SUBSYS_CLIENT=4
} SubsystemIDs;

typedef enum {
    ALIVE,
    DEAD,
    RESTARTING
} ProcessLifeStatus;



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
