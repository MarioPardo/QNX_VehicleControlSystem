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

// Use this for processes sending data to telemetry.c
// This is the expected data archtype that the subsystems should be sending to telemetry.c with their potential updates
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

// Already defined in defs as used outside file as well
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



// Haruns- this is for the DATA COMING INTO QNX VIA UDP . 
//  Webots and Dashboard -> QNX

typedef struct 
{
    double percentage;      // this is the brake level input passed in as a percentage 
    double angle;           // this is the steering angle input from  dashboard
    int enabled;            // this is for the snowmode 
    double speed;           // current speed from sim in webots  
    double temp;            // temp value which wil be coming from webots
    char* toggleGear [2];         //[ D = 0 , R = 1]

}message ;

typedef struct 
{
    char origin[32];    //Stores the origin [ Userinput | VehicleInput ]
    char subsys[32];
    // char msg_type[32]; //No longer need this 
    message msg;
} msg_packet ;



// Haruns - Telemetry data struct for now
// I might also just use this for data from processes to 
// This is data from QNX -> PYTHON DASHBOARD

typedef struct{
    float speed;
    int   snow_mode;    // bool
    char warnings [10][64];  
    int   warning_count;
    
}telemetry_msg;

typedef struct {
    char type [32];
    telemetry_msg tel;
    // float timestamp ;  //Not needed anymore
}telemetry_packet;


// This is or data from QNX -> WEBOTS
// webot sim data
typedef struct 
{
    double throttle_level;    // [  0 , 1]
    double brake_level;       // [  0 , 1]
    double steering_level;    // [ -1 , 1]
    int   snow_mode;
    char* toggleGear [2];         //[ D = 0 , R = 1]
    
}vehicle_controls_data;

typedef struct 
{
    char* subsys  [32];         // contains name of subsystem for that data
    vehicle_controls_data data;
}vehicle_controls;




void packet_init(msg_packet *c);
void vc_init(vehicle_controls *vc);
char *telemetry_to_json(telemetry_packet *t);
void json_to_msg_packet(const char *json_str, msg_packet *p);
char* sim_data_to_json(vehicle_controls *v);

// -------------------------------------------------------------------------------------------------------

#endif
