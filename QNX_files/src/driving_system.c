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
    float currentSpeed;            // km/h
    float currentThrottleLevel;    // 0..1
    float maxThrottleLevel;        // 0..1
    float currentSteeringAngle;    // degrees (or whatever the sim uses)
    bool  snowMode;
} DriveContext;

int sockfd;
struct sockaddr_in dest;
name_attach_t *attach;

///// Drive Utilities ///////

static void driving_system_setup_driveinfo(DriveContext* context)
{
    context->currentSpeed = 0.0f;
    context->currentThrottleLevel = 0.0f;
    context->maxThrottleLevel = 1.0f;
    context->currentSteeringAngle = 0.0f;
    context->snowMode = false;
    printf("[DRIVING SYSTEM] Info Setup Complete\n");
}

static void processUserDriveInput(DriveContext* context, float throttleLevel, float steeringAngle, bool snowMode)
{
    context->snowMode = snowMode;

    // If snow mode is enabled, cap max throttle.
    context->maxThrottleLevel = context->snowMode ? 0.6f : 1.0f;

    // Steering input is passed through directly.
    context->currentSteeringAngle = steeringAngle;

    // Clamp throttle to [0, maxThrottleLevel]
    context->currentThrottleLevel = fmaxf(0.0f, fminf(throttleLevel, context->maxThrottleLevel));

    printf("[DRIVING SYSTEM] User input -> throttle: %.2f (max %.2f) steering: %.2f snowMode: %s\n",
           context->currentThrottleLevel,
           context->maxThrottleLevel,
           context->currentSteeringAngle,
           context->snowMode ? "true" : "false");
}

static void processVehicleDriveData(DriveContext* context, float speed_kmh)
{
    context->currentSpeed = speed_kmh;
    printf("[DRIVING SYSTEM] Vehicle data -> speed: %.2f km/h\n", context->currentSpeed);
}

static void dispatchDriveData(DriveContext* context, int telemetry_coid, int vehiclesender_coid)
{
    // TODO: send warnings to telemetry, and send throttle/steering to vehicle_sender.

}

/// Communication ///////

static int connect_by_name_with_retries(const char* name, int retries, unsigned sleep_seconds)
{
    if (!name || retries <= 0) return -1;

    for (int attempt = 0; attempt < retries; ++attempt)
    {
        int coid = name_open(name, 0);
        if (coid != -1) return coid;

        printf("[DRIVING] Waiting for %s...\n", name);
        sleep(sleep_seconds);
    }

    printf("[DRIVING] ERROR: timed out connecting to %s\n", name);
    return -1;
}

static int setupCommChannels(int* telemetry_coid, int* vehiclesender_coid)
{
    if (!telemetry_coid || !vehiclesender_coid) {
    printf("[DRIVING] ERROR: setupCommChannels received NULL pointer(s)\n");
        return -1;
    }

    const int max_retries = 10;
    const unsigned retry_sleep_s = 1;

    *telemetry_coid = connect_by_name_with_retries("telemetry_system", max_retries, retry_sleep_s);
    if (*telemetry_coid == -1) return -1;
    printf("[DRIVING] Connected to telemetry\n");

    *vehiclesender_coid = connect_by_name_with_retries("vehicle_sender", max_retries, retry_sleep_s);
    if (*vehiclesender_coid == -1) return -1;
    printf("[DRIVING] Connected to vehicle sender\n");

    return 0;
}

static void receiveMessage(DriveContext* driveContext, int rcvid)
{
   //Check if message is Vehicle Data (from simulator)  or UserInput(from dashboard)


    //if VehicleData

        //parse message
        // send data to ProcessVehicleDriveDataData;



    //if UserInput

        //parse input

        //send data to ProcessUserDriveInput
}

/// Process Handling ///////

static void shutdown_drive(int signo)
{
    (void)signo;
    exit(0);
}

static void cleanup_drive(void)
{
    printf("[DRIVING SYSTEM] Exiting, detaching name...\n");
    name_detach(attach, 0);
}

///// MAIN //////

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("[DRIVING SYSTEM] Hello from Driving System!\n");

    DriveContext driveContext;
    driving_system_setup_driveinfo(&driveContext);

    // register so other subsystems can find us
    attach = name_attach(NULL, "driving_system", 0);
    if (!attach) {
        printf("[DRIVING SYSTEM] name_attach failed\n");
        return -1;
    }

    atexit(cleanup_drive);
    signal(SIGTERM, shutdown_drive);
    signal(SIGINT,  shutdown_drive);
    printf("[DRIVING SYSTEM] Registered as driving_system\n");

    int telemetry_coid;
    int vehiclesender_coid;
    if (setupCommChannels(&telemetry_coid, &vehiclesender_coid) != 0) {
    printf("[DRIVING SYSTEM] Failed to set up communication channels\n");
        return -1;
    }

    // connect to own channel for timer pulses
    int my_coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);

    struct sigevent event;
    SIGEV_PULSE_INIT(&event, my_coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_SUBSYSTEM_INTERNAL, SUBSYS_DRIVE);

    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);

    struct itimerspec itime;
    itime.it_value.tv_sec  = SYS_DRIVE_RESPONSETIME_MS / 1000;
    itime.it_value.tv_nsec = (SYS_DRIVE_RESPONSETIME_MS % 1000) * 1000000;
    itime.it_interval = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);

    struct _pulse pulse;
    while (1)
    {
        int rcvid = MsgReceive(attach->chid, &pulse, sizeof(pulse), NULL);

        if (rcvid == 0) {
            if (pulse.code == PULSE_SUBSYSTEM_INTERNAL) {
                dispatchDriveData(&driveContext, telemetry_coid, vehiclesender_coid);
            }
            if (pulse.code == PULSE_CHAOSMODE) {
                // Optional: you can add chaos behavior later.
            }
        } else if (rcvid > 0) {
            receiveMessage(&driveContext, rcvid);
            MsgReply(rcvid, EOK, NULL, 0);
        }
    }

    return 0;
}
