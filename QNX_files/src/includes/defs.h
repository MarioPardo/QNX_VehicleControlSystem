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



//Networking
#define LISTEN_PORT 5555
#define SEND_PORT   5556
#define DEST_IP     "127.0.0.1"

extern int sockfd;
extern struct sockaddr_in dest;

void socket_setup();

void* send_loop(void* arg);
void* recv_loop(void* arg);


//Pulse Codes
typedef enum {
    PULSE_WATCHDOG_AUDIT = _PULSE_CODE_MINAVAIL + 1,
    PULSE_BRAKING_ALIVE,
    PULSE_BRAKING_INTERNAL
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
char *packet_to_json(msg_packet *p);
void json_to_packet(const char *json_str, telemetry_packet *t);

// -------------------------------------------------------------------------------------------------------

#endif
