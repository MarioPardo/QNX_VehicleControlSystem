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

// ----- TEST FUNCTION - remove once processes are sending real data -----
void build_test_vehicle_controls(vehicle_controls *state, int tick) {

    // throttle ramps up over 10 ticks then resets
    state->data.throttle_level = (double)(tick % 10) / 10.0;

    // brake pulses on and off every 5 ticks
    state->data.brake_level = (tick % 5 == 0) ? 0.8 : 0.0;

    // steering sweeps left to right
    state->data.steering_level = sin((double)tick * 0.3);  // needs #include <math.h>

    // snow mode toggles every 8 ticks
    state->data.snow_mode = (tick / 8) % 2;

    // gear stays D for now
    strncpy(state->data.toggleGear, "D", sizeof(state->data.toggleGear) - 1);
    state->data.toggleGear[1] = '\0';
}
// -----------------------------------------------------------------------

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
    sockfd = sender_setup(DEST_IP, WEBOTS_SEND_PORT, &dest);
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
    int tick=0;

    while (1) {
        int rcvid = MsgReceive(attach->chid, &pulse, sizeof(pulse), NULL);

        if (rcvid == 0) {
            // timer fired — package current state and send to Webots
            if (pulse.code == PULSE_SUBSYSTEM_INTERNAL) {

                // TEMP: fake data for pipeline testing
                build_test_vehicle_controls(&state, tick++);
                //  remove once real process data flows in

                char *json = sim_data_to_json(&state);
                sendto(sockfd, json, strlen(json), 0,
                       (struct sockaddr *)&dest, sizeof(dest));
                printf("[VEHICLE_SENDER-JSON-STRING] sent to Webots: %s\n", json);
                free(json);

                // check in with watchdog
                MsgSendPulse(watchdog_coid, -1, PULSE_SUBSYSTEM_ALIVE, SUBSYS_VEHICLE_SENDER); 
                // TODO: add SUBSYS_VEHICLE_SENDER to SubsystemIDs enum in defs.h
            }

        } else if (rcvid > 0) {
            // message from a subsystem process
            MsgRead(rcvid, &msg, sizeof(msg), 0);

            switch (msg.subsys) {
                case SUBSYS_BRAKE:
                    printf("[VEHICLE_SENDER] Brake update received\n");
                    state.data.brake_level = msg.brake_level;
                    break;

                case SUBSYS_DRIVE:
                    printf("[VEHICLE_SENDER] Drive update received\n");
                    state.data.throttle_level = msg.throttle_level;

                    strncpy(state.data.toggleGear, msg.toggleGear, sizeof(state.data.toggleGear) - 1);
                    state.data.toggleGear[sizeof(state.data.toggleGear) - 1] = '\0';
                    break;

                case SUBSYS_STEERING:
                    printf("[VEHICLE_SENDER] Steering update received\n");
                    state.data.steering_level = msg.steering_angle;
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