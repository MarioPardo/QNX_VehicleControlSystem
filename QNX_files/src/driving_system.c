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
    char  gear[2];                 // "D" or "R"
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
    context->gear[0] = 'D';
    context->gear[1] = '\0';
    printf("[DRIVING SYSTEM] Info Setup Complete\n");
}

static void processUserDriveInput(DriveContext* context, float throttleLevel, float steeringAngle, bool snowMode, const char* gear)
{
    context->snowMode = snowMode;

    // If snow mode is enabled, cap max throttle.
    context->maxThrottleLevel = context->snowMode ? 0.6f : 1.0f;

    // Steering input is passed through directly.
    context->currentSteeringAngle = steeringAngle;

    // Clamp throttle to [0, maxThrottleLevel]
    context->currentThrottleLevel = fmaxf(0.0f, fminf(throttleLevel, context->maxThrottleLevel));

    context->gear[0] = gear[0];
    context->gear[1] = '\0';

    /*
    printf("[DRIVING SYSTEM] User input -> throttle: %.2f (max %.2f) steering: %.2f snowMode: %s gear: %s\n",
           context->currentThrottleLevel,
           context->maxThrottleLevel,
           context->currentSteeringAngle,
           context->snowMode ? "true" : "false",
           context->gear);
           */
           
           
}

static void processVehicleDriveData(DriveContext* context, float speed_kmh)
{
    context->currentSpeed = speed_kmh;
   // printf("[DRIVING SYSTEM] Vehicle data -> speed: %.2f km/h\n", context->currentSpeed);
}

static int trySendMsg(int *coid, const char *name, const void *msg, int nbytes)
{
    const int numRetries = 3;

    if (*coid != -1 && MsgSend(*coid, msg, nbytes, NULL, 0) != -1)
        return 0; // worked

    // send failed — process may be down; try to reconnect and resend
    for (int i = 0; i < numRetries; i++) {
        if (*coid != -1) name_close(*coid);
        *coid = name_open(name, 0);
        if (*coid != -1 && MsgSend(*coid, msg, nbytes, NULL, 0) != -1)
            return 0; // reconnected and sent
        usleep(50000);
    }
    printf("[DRIVING] WARNING: could not send to %s\n", name);
    return -1;
}

static int trySendPulse(int *coid, const char *name, int code, int value)
{
    const int numRetries = 3;

    if (*coid != -1 && MsgSendPulse(*coid, -1, code, value) != -1)
        return 0; // worked

    // pulse failed — process may be down; try to reconnect and resend
    for (int i = 0; i < numRetries; i++) {
        if (*coid != -1) name_close(*coid);
        *coid = name_open(name, 0);
        if (*coid != -1 && MsgSendPulse(*coid, -1, code, value) != -1)
            return 0; // reconnected and sent
        usleep(50000);
    }
    printf("[DRIVING] WARNING: could not send pulse to %s\n", name);
    return -1;
}

static void dispatchDriveData(DriveContext* context, int *telemetry_coid, int *vehiclesender_coid)
{
    ProcessMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.subsys         = SUBSYS_DRIVE;
    msg.throttle_level = context->currentThrottleLevel;
    msg.steering_angle = context->currentSteeringAngle;
    msg.snowmode       = context->snowMode ? 1 : 0;
    msg.speed          = context->currentSpeed;
    strncpy(msg.toggleGear, context->gear, sizeof(msg.toggleGear) - 1);
    trySendMsg(vehiclesender_coid, "vehicle_sender",   &msg, sizeof(msg));
    trySendMsg(telemetry_coid,     "telemetry_system", &msg, sizeof(msg));
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
    printf("[DRIVING] %s\n", *telemetry_coid != -1 ? "Connected to telemetry" : "WARNING: telemetry_system unavailable, continuing");

    *vehiclesender_coid = connect_by_name_with_retries("vehicle_sender", max_retries, retry_sleep_s);
    printf("[DRIVING] %s\n", *vehiclesender_coid != -1 ? "Connected to vehicle sender" : "WARNING: vehicle_sender unavailable, continuing");

    return 0;
}

static void sendWatchdogHealthStatus(int *watchdog_coid)
{
    trySendPulse(watchdog_coid, "watchdog", PULSE_SUBSYSTEM_ALIVE, SUBSYS_DRIVE);
}

static void receiveMessage(DriveContext* driveContext, int rcvid, msg_packet* pkt)
{
    MsgReply(rcvid, 0, NULL, 0);

    if (strcmp(pkt->subsys, "Drive") != 0)
        return;

    if (strcmp(pkt->origin, "UserInput") == 0) {
        processUserDriveInput(driveContext, (float)pkt->msg.throttle, (float)pkt->msg.angle, (bool)pkt->msg.enabled, pkt->msg.toggleGear);
    } else if (strcmp(pkt->origin, "VehicleData") == 0) {
        processVehicleDriveData(driveContext, (float)pkt->msg.speed);
    }
}

static void chaosMode() // triggers a failure — watchdog will kill and restart
{
    printf("[DRIVING SYSTEM] CHAOS MODE ACTIVATED!\n");
    // Keep replying to incoming messages so client isn't blocked.
    // Stop heartbeating — watchdog detects the silence and kills us.
    msg_packet buf;
    while (1) {
        int rcvid = MsgReceive(attach->chid, &buf, sizeof(buf), NULL);
        if (rcvid > 0)
            MsgReply(rcvid, EOK, NULL, 0);
    }
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

    // connect to watchdog by name (for heartbeats)
    int watchdog_coid = -1;
    watchdog_coid = connect_by_name_with_retries("watchdog", 10, 1);
    printf("[DRIVING SYSTEM] %s\n", watchdog_coid != -1 ? "Connected to Watchdog" : "WARNING: Watchdog unavailable, continuing");

    int telemetry_coid     = -1;
    int vehiclesender_coid = -1;
    setupCommChannels(&telemetry_coid, &vehiclesender_coid);

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

    union {
        struct _pulse pulse;
        msg_packet    pkt;
    } buf;

    while (1)
    {
        int rcvid = MsgReceive(attach->chid, &buf, sizeof(buf), NULL);

        if (rcvid == 0) {
            if (buf.pulse.code == PULSE_SUBSYSTEM_INTERNAL) {
                sendWatchdogHealthStatus(&watchdog_coid);
                dispatchDriveData(&driveContext, &telemetry_coid, &vehiclesender_coid);
            }
            if (buf.pulse.code == PULSE_CHAOSMODE)
                chaosMode();
        } else if (rcvid > 0) {
            receiveMessage(&driveContext, rcvid, &buf.pkt);
        }
    }

    return 0;
}
