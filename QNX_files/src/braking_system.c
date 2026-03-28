#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <sys/neutrino.h>
#include <time.h>
#include <string.h>
#include "includes/defs.h"

typedef struct {
    float currentBrakeLevel;
    float currentBrakeTemp;
    float maxSafeBrakeLevel;
} BrakingContext;

int sockfd;
struct sockaddr_in dest;
bool chaos = false;
name_attach_t *attach;


float maxBrakeTemp = 200.0f; //celcius


///// Braking Utilities ///////

void braking_system_setup_vehicleinfo(BrakingContext* context) 
{

//TODO we should probably have webots send us brake level and temp
//so we can recover from crash with correct info, must discuss

    context->currentBrakeLevel = 0.0f;
    context->currentBrakeTemp = 0.0f;
    context->maxSafeBrakeLevel = 1.0f;
    printf("[BRAKE SYSTEM] Info Setup Complete\n");
}



void processUserBrakeInput(BrakingContext* context, float brakeLevel) 
{
    //parse input

    float newBrakeLevel = fmax(0.0f, fmin(brakeLevel, context->maxSafeBrakeLevel)); // clamp to safe range
    context->currentBrakeLevel = newBrakeLevel;

    //print received data and new brake level
    printf("[BRAKE SYSTEM] Received User Brake Input: %.2f, New Brake Level: %.2f \n", brakeLevel, context->currentBrakeLevel);
}


void processVehicleBrakeData(BrakingContext* context, float brakeTemp)
{
    //parse input


    context->currentBrakeTemp = brakeTemp;

    //if temp is too high, lower brake level
    if (brakeTemp > maxBrakeTemp) 
        context->maxSafeBrakeLevel = 0.8f;  // reduce brake level by 20%
    else
        context->maxSafeBrakeLevel = 1.0f; // reset to max if temp is safe


    //print received data and new safe brake level
    printf("[BRAKE SYSTEM] Received Brake Temp: %.2f, New Max Safe Brake Level: %.2f\n", brakeTemp, context->maxSafeBrakeLevel);

    return;
}

void dispatchBrakeData(BrakingContext* context, int telemetry_coid, int vehiclesender_coid)
{
    ProcessMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.subsys      = SUBSYS_BRAKE;
    msg.brake_level = context->currentBrakeLevel;
    MsgSend(vehiclesender_coid, &msg, sizeof(msg), NULL, 0);
    MsgSend(telemetry_coid,     &msg, sizeof(msg), NULL, 0);
}

/// Communication ///////

static int connect_by_name_with_retries(const char* name, int retries, unsigned sleep_seconds)
{
    if (!name || retries <= 0) return -1;

    for (int attempt = 0; attempt < retries; ++attempt)
    {
        int coid = name_open(name, 0);
        if (coid != -1) return coid;

        printf("[BRAKE] Waiting for %s...\n", name);
        sleep(sleep_seconds);
    }

    printf("[BRAKE] ERROR: timed out connecting to %s\n", name);
    return -1;
}

int setupCommChannels(int* watchdog_coid, int* telemetry_coid, int* vehiclesender_coid) 
{

    if (!watchdog_coid || !telemetry_coid || !vehiclesender_coid) {
        printf("[BRAKE] ERROR: setupCommChannels received NULL pointer(s)\n");
        return -1;
    }

    const int max_retries = 10;
    const unsigned retry_sleep_s = 1;

    // connect to watchdog by name
    *watchdog_coid = connect_by_name_with_retries("watchdog", max_retries, retry_sleep_s);
    if (*watchdog_coid == -1) return -1;
    printf("[BRAKE] Connected to Watchdog\n");
    

    // connect to telemetry
    *telemetry_coid = connect_by_name_with_retries("telemetry_system", max_retries, retry_sleep_s);
    if (*telemetry_coid == -1) return -1;
    printf("[BRAKE] Connected to telemetry\n");


    //connect to vehicle sender
   *vehiclesender_coid = connect_by_name_with_retries("vehicle_sender", max_retries, retry_sleep_s);
    if (*vehiclesender_coid == -1) return -1;
    printf("[BRAKE] Connected to vehicle sender\n");



    return 0;
}

void receiveMessage(BrakingContext* brakeContext, int rcvid, msg_packet* pkt)
{
    MsgReply(rcvid, 0, NULL, 0);

    if (strcmp(pkt->subsys, "Brake") != 0) //ensure meant for brake
        return;

    //process data according to source
    if (strcmp(pkt->origin, "UserInput") == 0) 
        processUserBrakeInput(brakeContext, (float)pkt->msg.percentage);
     else if (strcmp(pkt->origin, "VehicleData") == 0) 
        processVehicleBrakeData(brakeContext, (float)pkt->msg.temp);
    
}

/// Process Handling ///////


void sendWatchdogHealthStatus(int watchdog_coid)
{

    if (watchdog_coid == -1)
    {
        printf("[BRAKE SYSTEM] cannot find CHID of watchdog \n");
        return;
    }
	MsgSendPulse(watchdog_coid, -1, PULSE_SUBSYSTEM_ALIVE, SUBSYS_BRAKE);
}

void chaosMode()// triggers a failure. must be restarted
{
    printf("[BRAKE SYSTEM] CHAOS MODE ACTIVATED! \n");
    chaos = true;
    return;
    
    while(true)
    {
        //do nothing
    }
}

void shutdown_braking(int signo) {
    exit(0);  // triggers atexit handlers safely
}

void cleanup_braking(void) 
{
    printf("[BRAKE SYSTEM] Exiting, detaching name...\n");
    name_detach(attach, 0);
}

///// MAIN //////

int main(int argc, char *argv[])
{
	printf("[BRAKE SYSTEM]]Hello from Braking System! \n");

	BrakingContext brakeContext;
    braking_system_setup_vehicleinfo(&brakeContext);


    // register so other subsystems can find us
    attach = name_attach(NULL, "braking_system", 0);
    if (!attach) {
        printf("[BRAKE SYSTEM] name_attach failed\n");
        return -1;
    }
    
    //set up clean exit of process
    atexit(cleanup_braking);
    signal(SIGTERM, shutdown_braking);
    signal(SIGINT,  shutdown_braking);
    printf("[BRAKE SYSTEM] Registered as braking_system\n");

    //comm channels
    int watchdog_coid;
    int telemetry_coid;
    int vehiclesender_coid;

    // set up communication channels
    int result = setupCommChannels(&watchdog_coid, &telemetry_coid, &vehiclesender_coid);
    if (result != 0) {
        printf("[BRAKE SYSTEM] Failed to set up communication channels\n");
        return -1;
    }

    // connect to own channel for timer pulses
    int my_coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);

    //Initialize own timer so process wakes up as needed
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, my_coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_SUBSYSTEM_INTERNAL, SUBSYS_BRAKE);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);

    struct itimerspec itime;
    itime.it_value.tv_sec = SYS_BRAKING_RESPONSETIME_MS / 1000;
    itime.it_value.tv_nsec = (SYS_BRAKING_RESPONSETIME_MS % 1000) * 1000000;
    itime.it_interval = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);

    union {
        struct _pulse pulse;
        msg_packet    pkt;
    } buf;

    while (1)
    {
        int rcvid = MsgReceive(attach->chid, &buf, sizeof(buf), NULL);

        if (rcvid == 0) {
            if (buf.pulse.code == PULSE_SUBSYSTEM_INTERNAL)
            {
                sendWatchdogHealthStatus(watchdog_coid);
                dispatchBrakeData(&brakeContext, telemetry_coid, vehiclesender_coid);
            }
            if (buf.pulse.code == PULSE_CHAOSMODE)
                chaosMode();

        } else if (rcvid > 0) {
            receiveMessage(&brakeContext, rcvid, &buf.pkt);
        }
    }
    return 0;
}
