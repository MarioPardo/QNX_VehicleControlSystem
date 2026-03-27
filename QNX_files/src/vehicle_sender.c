//This process communicates with webots through UDP connection by sending over the webots data received from the processes .
// Sort of identical to telemetry.c but now instead of sending dashboard data to dashboard its send the vehicle data to webots .
// Has to connect to webots via  UDP
// Has to create and store its own state before sending the sim data 
// The vehicle control data basically

// vehicle_sender.c
// Receives vehicle control data from subsystem processes (brake, drive, steering)
// and forwards it to the Webots sim via UDP.
// Mirror of telemetry.c but targets Webots instead of the Python dashboard.

#include "includes/defs.h"

int sockfd;
struct sockaddr_in dest;
name_attach_t *attach;

void shutdown_vehicle_sender(int signo) {
    exit(0);
}

void cleanup_vehicle_sender(void) {
    printf("[VEHICLE_SENDER] Exiting, detaching name...\n");
    name_detach(attach, 0);
}

int main(int argc, char *argv[]) {
    printf("[VEHICLE_SENDER] Starting...\n");

    // connect to watchdog
    int watchdog_coid = -1;
    while (watchdog_coid == -1) {
        watchdog_coid = name_open("watchdog", 0);
        if (watchdog_coid == -1) {
            printf("[VEHICLE_SENDER] Waiting for watchdog...\n");
            sleep(1);
        }
    }
    printf("[VEHICLE_SENDER] Connected to watchdog\n");

    // register name so subsystems can find us
    attach = name_attach(NULL, "vehicle_sender", 0);
    if (!attach) {
        printf("[VEHICLE_SENDER] name_attach failed: %d\n", errno);
        return -1;
    }
    atexit(cleanup_vehicle_sender);
    signal(SIGTERM, shutdown_vehicle_sender);
    signal(SIGINT,  shutdown_vehicle_sender);
    printf("[VEHICLE_SENDER] Registered as vehicle_sender\n");

    // UDP setup to Webots
    // Webots listens on WEBOTS_PORT, we send to WEBOTS_IP
    // You'll want to add these to defs.h:
    //   #define WEBOTS_IP   "192.168.56.1"
    //   #define WEBOTS_PORT  5001
    sender_setup();  // or a dedicated webots_sender_setup() if ports differ
    printf("[VEHICLE_SENDER] UDP to Webots ready\n");

    // connect to own channel for timer pulses
    int my_coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);

    // 100ms send timer
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, my_coid, SIGEV_PULSE_PRIO_INHERIT,
                     PULSE_SUBSYSTEM_INTERNAL, 0);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);
    struct itimerspec itime;
    itime.it_value.tv_sec  = 0;
    itime.it_value.tv_nsec = 100000000;   // 100ms
    itime.it_interval      = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);

    printf("[VEHICLE_SENDER] Running\n");

    // local state — holds the latest values from each subsystem
    vehicle_controls state;
    vc_init(&state);
    strcpy(state.subsys, "vehicle_sender");

    ProcessMsg msg;
    struct _pulse pulse;

    while (1) {
        int rcvid = MsgReceive(attach->chid, &pulse, sizeof(pulse), NULL);

        if (rcvid == 0) {
            // timer fired — package current state and send to Webots
            if (pulse.code == PULSE_SUBSYSTEM_INTERNAL) {

                char *json = sim_data_to_json(&state);
                sendto(sockfd, json, strlen(json), 0,
                       (struct sockaddr *)&dest, sizeof(dest));
                printf("[VEHICLE_SENDER] sent to Webots: %s\n", json);
                free(json);

                // check in with watchdog
                MsgSendPulse(watchdog_coid, -1, PULSE_SUBSYSTEM_ALIVE, SUBSYS_TELEMETRY); 
                // TODO: add SUBSYS_VEHICLE_SENDER to SubsystemIDs enum in defs.h
            }

        } else if (rcvid > 0) {
            // message from a subsystem process
            MsgRead(rcvid, &msg, sizeof(msg), 0);

            switch (msg.subsys) {
                case SUBSYS_BRAKE:
                    printf("[VEHICLE_SENDER] Brake update received\n");
                    state.data.brake_level = msg.data.brake.brake_level;
                    break;

                case SUBSYS_DRIVE:
                    printf("[VEHICLE_SENDER] Drive update received\n");
                    state.data.throttle_level = msg.data.throttle.value;
                    break;

                case SUBSYS_STEERING:
                    printf("[VEHICLE_SENDER] Steering update received\n");
                    state.data.steering_level = msg.data.steering.angle;
                    break;

                default:
                    printf("[VEHICLE_SENDER] Unknown subsystem: %d\n", msg.subsys);
                    break;
            }
            MsgReply(rcvid, EOK, NULL, 0);
        }
    }

    close(sockfd);
    return 0;
}