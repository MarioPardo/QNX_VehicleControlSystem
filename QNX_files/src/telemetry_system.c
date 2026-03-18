// Well

// Define this as the temporary telemetry process for now to work on this

// So we receive data from the various processes we have and from then on we have 

//Ok ill call this telemetry for now and use this to test out my data parser , and establish connection right now ,

// Pipeline I want working at bare minimum 

//Sending pipeline
//----------------------------------------------------------------
// brake.c    ──── MsgSend ────► telemetry.c ──── UDP ────► Python Dashboard


// telemetry.c
#include "includes/defs.h"

int sockfd;
struct sockaddr_in dest;

int main(int argc, char *argv[]) {
    printf("[TELEMETRY] Starting...\n");

    telemetry_msg state;
    memset(&state, 0, sizeof(state));

    // get watchdog info from args
    int watchdog_pid = -1, watchdog_chid = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) watchdog_pid  = atoi(argv[++i]);
        if (strcmp(argv[i], "-c") == 0) watchdog_chid = atoi(argv[++i]);
    }
    
    // connect to watchdog
    int watchdog_coid = -1;
    while (watchdog_coid == -1) {
        watchdog_coid = ConnectAttach(ND_LOCAL_NODE, watchdog_pid,
                                      watchdog_chid, _NTO_SIDE_CHANNEL, 0);
        sleep(1);
    }
    printf("[TELEMETRY] Connected to watchdog\n");

    // register name so other subsystems can find us
    name_attach_t *attach = name_attach(NULL, "telemetry_system", 0);
    if (!attach) {
        printf("[TELEMETRY] name_attach failed: %d \n",errno);
        return -1;
    }
    printf("[TELEMETRY] Registered as telemetry_system\n");
    


    // For udp connection to python dashboard ( Might do sending here the n receiving elsewhere )
    sender_setup();

    // use attach's channel instead of creating new one
    int my_chid = ChannelCreate(0);
    int my_coid = ConnectAttach(ND_LOCAL_NODE, 0, my_chid, _NTO_SIDE_CHANNEL, 0);
    
    // setup 100ms timer for sending telemetry
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, my_coid, SIGEV_PULSE_PRIO_INHERIT,
                     PULSE_TELEMETRY_INTERNAL, 0);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);
    struct itimerspec itime;
    itime.it_value.tv_sec  = 0;
    itime.it_value.tv_nsec = 100000000;  // 100ms
    itime.it_interval      = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);

    //Telemetry process wakes up every 100 ms to prep the data so far and send to dashboard
    printf("[TELEMETRY] Running\n");

    // This is the main telemetry process function
    // main loop
    ProcessMsg msg;  // reuse for all incoming 

    while (1) {
        int rcvid = MsgReceive(my_chid, &msg, sizeof(msg), NULL);

        if (rcvid == 0) {
            // timer fired - package and send to Python //Going to fix this after testing  , promise the PULSEBAKING
            if (((struct _pulse*)&msg)->code == PULSE_TELEMETRY_INTERNAL) {

                // build telemetry packet to be sent to dashboard with data so far
                telemetry_packet t;
                memset(&t, 0, sizeof(t));
                strcpy(t.type, "VehicleTelemetry");
                t.tel.speed          = state.speed;
                t.tel.throttle       = state.throttle;
                t.tel.brake          = state.brake;
                t.tel.steering_angle = state.steering_angle;
                t.tel.safe_mode      = state.safe_mode;
                t.tel.snow_mode      = state.snow_mode;
                t.timestamp          = (float)time(NULL);

                // convert and send to Python
                char *json = telemetry_to_json(&t);
                sendto(sockfd, json, strlen(json), 0,
                       (struct sockaddr *)&dest, sizeof(dest));
                free(json);

                printf("[TELEMETRY] packet sent to Python\n");

                // check in with watchdog
                MsgSendPulse(watchdog_coid, -1, PULSE_TELEMETRY_ALIVE, 0);
            }

        } else if (rcvid > 0) {
            // message from a subsystem
            MsgRead(rcvid, &msg, sizeof(msg), 0);

            switch (msg.subsys) {
                case SUBSYS_BRAKE:
                    state.speed = msg.data.brake.speed;
                    state.brake = msg.data.brake.brake_level;
                    printf("[TELEMETRY] Brake update - speed: %.1f\n", state.speed);
                    break;
                case SUBSYS_THROTTLE:
                    state.throttle = msg.data.throttle.value;
                    break;
                case SUBSYS_STEERING:
                    state.steering_angle = msg.data.steering.angle;
                    break;
            }
            MsgReply(rcvid, EOK, NULL, 0);
        }
    }

    close(sockfd);
    return 0;
}
