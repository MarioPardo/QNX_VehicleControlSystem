#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <time.h>
#include <string.h>
#include "includes/defs.h"

typedef struct {
    float currentBrakeLevel;
    float currentSpeed;      
    bool absActive;
} BrakingContext;

void braking_system_setup_vehicleinfo(BrakingContext* context) {
    // TODO handle setup when process has crashed/been stopped
    // (e.g., check command line flags to see if this is a respawn)
    context->currentBrakeLevel = 0.0f;
    context->currentSpeed = 0.0f;
    context->absActive = false;
    printf("[BRAKE SYSTEM] Info Setup Complete\n");
}

void processBrakeInput(BrakingContext* context, int telemetry_coid) {
    // calculate
    context->currentSpeed -= 0.2f;  // natural deceleration
    if (context->currentBrakeLevel > 0)
        context->currentSpeed -= (context->currentBrakeLevel / 100.0f) * 12.0f;
    if (context->currentSpeed < 0) context->currentSpeed = 0;

    printf("[BRAKE] Speed: %.1f Brake: %.1f\n", 
           context->currentSpeed, context->currentBrakeLevel);

    // send to telemetry - this is ,uh yeah... ,  telemetry parsed
    ProcessMsg msg;

    msg.subsys                 = SUBSYS_BRAKE;
    msg.data.brake.speed       = context->currentSpeed;
    msg.data.brake.brake_level = context->currentBrakeLevel;
    MsgSend(telemetry_coid, &msg, sizeof(msg), NULL, 0);
    
    // printf("[BRAKE SYSTEM] Applying brakes at level: %f\n", context->currentBrakeLevel);
}

void receiveMessage(BrakingContext* context, int rcvid) {
    //actual working logic just to make sure pipeline is flowing 
    
    message msg;  // THIS is where actual data lands
    MsgRead(rcvid, &msg, sizeof(msg), 0);  // pull data out using ticket
    
    // now msg has the actual brake percentage from dashboard
    context->currentBrakeLevel = msg.percentage;
    
    //printf("[BRAKE SYSTEM] Parsing incoming message...\n");
    // check message type
    // appropriate message handling functions
}

void sendWatchdogHealthStatus(int watchdog_coid)
{

    if (watchdog_coid == -1)
    {
        printf("[BRAKE SYSTEM] cannot find CHID of watchdog \n");
        return;
    }
    printf("[BRAKE SYSTEM] Checking in! \n");
	MsgSendPulse(watchdog_coid, -1, PULSE_BRAKING_ALIVE, 0);
}

int main(int argc, char *argv[])
{
	printf("[BRAKE SYSTEM]]Hello from Braking System!");

	BrakingContext state;
    braking_system_setup_vehicleinfo(&state);

    //get watchdog info
    int watchdog_pid = -1, watchdog_chid = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) watchdog_pid = atoi(argv[++i]);
        if (strcmp(argv[i], "-c") == 0) watchdog_chid = atoi(argv[++i]);
    }

    //TODO refactor below into function

    //create channels and connect
    int watchdog_coid = -1;
    while (watchdog_coid == -1) {
        watchdog_coid = ConnectAttach(ND_LOCAL_NODE, watchdog_pid, watchdog_chid, _NTO_SIDE_CHANNEL, 0);
        if (watchdog_coid == -1) {
            printf("[BRAKE] Waiting for Watchdog channel...\n");
            sleep(1);
        }
    }
    printf("[BRAKE SYSTEM] Connected to Watchdog ");

    // register so other subsystems can find us
    name_attach_t *attach = name_attach(NULL, "braking_system", 0);
    if (!attach) {
        printf("[BRAKE] name_attach failed\n");
        return -1;
    }
    printf("[BRAKE] Registered as braking_system\n");


    // find telemetry by name - works because telemetry called name_attach
    int telemetry_coid = -1;
    while (telemetry_coid == -1) {
        telemetry_coid = name_open("telemetry_system", 0);
        if (telemetry_coid == -1) {
            printf("[BRAKE] Waiting for telemetry...\n");
            sleep(1);
        }
    }
    printf("[BRAKE] Connected to telemetry\n");

    //Creating own channel to begin with

    int my_chid = ChannelCreate(0);
    int my_coid = ConnectAttach(ND_LOCAL_NODE, 0, my_chid, _NTO_SIDE_CHANNEL, 0);

    printf("[BRAKE SYSTEM] MyCHID: %d,  myCOID%d \n",my_chid, my_coid );

    //Initialize own timer so process wakes up as needed
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, my_coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_BRAKING_INTERNAL, 0);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);

    struct itimerspec itime;
    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 20000000; // 20ms execution cycle TODO make var
    itime.it_interval = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);

    struct _pulse pulse;

    while (1)
    {
        int rcvid = MsgReceive(my_chid, &pulse, sizeof(pulse), NULL);

        if (rcvid == 0) {
            if (pulse.code == PULSE_BRAKING_INTERNAL)
            {
                processBrakeInput(&state, telemetry_coid);
                sendWatchdogHealthStatus(watchdog_coid);
            }
        } else if (rcvid > 0) {
            receiveMessage(&state, rcvid);
            MsgReply(rcvid, EOK, NULL, 0);
        }
    }
    return 0;
}
