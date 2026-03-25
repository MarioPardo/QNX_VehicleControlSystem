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
name_attach_t *attach;

void shutdown_telemetry(int signo) {
    exit(0);  // triggers atexit handlers safely
}

void cleanup_telemetry(void) {
    printf("[TELEMETRY] Exiting, detaching name...\n");
    name_detach(attach, 0);
}

int main(int argc, char *argv[]) {
    printf("[TELEMETRY] Starting...\n");

    telemetry_msg state;
    memset(&state, 0, sizeof(state));

    // connect to watchdog by name
    int watchdog_coid = -1;
    while (watchdog_coid == -1) {
        watchdog_coid = name_open("watchdog", 0);
        if (watchdog_coid == -1) {
            printf("[TELEMETRY] Waiting for Watchdog...\n");
            sleep(1);
        }
    }
    printf("[TELEMETRY] Connected to watchdog\n");

    // register name so other subsystems can find us
    attach = name_attach(NULL, "telemetry_system", 0);
    if (!attach) {
        printf("[TELEMETRY] name_attach failed: %d \n",errno);
        return -1;
    }
    atexit(cleanup_telemetry);
    signal(SIGTERM, shutdown_telemetry);
    signal(SIGINT,  shutdown_telemetry);
    printf("[TELEMETRY] Registered as telemetry_system\n");
    


    // For udp connection to python dashboard ( Might do sending here the n receiving elsewhere )
    printf("[TELEMETRY] Setting up sender connection UDP \n");
    sender_setup();
    printf("[TELEMETRY] DONE - connection UDP \n");

    // connect to own channel for timer pulses
    int my_coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);

    // setup 100ms timer for sending telemetry
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, my_coid, SIGEV_PULSE_PRIO_INHERIT,
                     PULSE_SUBSYSTEM_INTERNAL, SUBSYS_TELEMETRY);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);
    struct itimerspec itime;
    itime.it_value.tv_sec  = 1;               // 1 second cycle (debug)
    itime.it_value.tv_nsec = 0;
    itime.it_interval      = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);

    //Telemetry process wakes up every 100 ms to prep the data so far and send to dashboard
    printf("[TELEMETRY] Running\n");
    
    // This is the main telemetry process function
    // main loop
    ProcessMsg msg;  // reuse for all incoming 
    
    struct _pulse pulse;

    while (1) {
        int rcvid = MsgReceive(attach->chid, &pulse, sizeof(pulse), NULL);

        if (rcvid == 0) {
            // timer fired - package and send to Python //Going to fix this after testing  , promise the PULSEBAKING
             if (pulse.code == PULSE_SUBSYSTEM_INTERNAL) 
             {

                //TODO should be two separate functions below
                {
                // build telemetry packet to be sent to dashboard with data so far
                telemetry_packet t;
                memset(&t, 0, sizeof(t));

                // This is what python is expecting in the process_packet function

                strcpy(t.type, "VehicleTelemetry");
                t.tel.speed          = state.speed;
                // t.tel.throttle       = state.throttle;
                // t.tel.brake          = state.brake;
                // t.tel.steering_angle = state.steering_angle;
                t.tel.safe_mode      = state.safe_mode;
                t.tel.snow_mode      = state.snow_mode;
                t.timestamp          = (float)time(NULL);

                // convert and send to Python
                char *json = telemetry_to_json(&t);
                sendto(sockfd, json, strlen(json), 0,
                       (struct sockaddr *)&dest, sizeof(dest));
                free(json);

                printf("[TELEMETRY] packet sent to Python\n");
                }

                // check in with watchdog
                MsgSendPulse(watchdog_coid, -1, PULSE_SUBSYSTEM_ALIVE, SUBSYS_TELEMETRY);
            }

        } else if (rcvid > 0) {
            // message from a subsystem
            MsgRead(rcvid, &msg, sizeof(msg), 0);

            switch (msg.subsys) {
                case SUBSYS_BRAKE:
                    printf("[TELEMETRY] Hello from brake \n");
                    state.speed = msg.data.brake.speed;
                    // state.brake = msg.data.brake.brake_level;
                    // printf("[TELEMETRY] Brake update - speed: %.1f\n", state.speed);
                    break;
                case SUBSYS_THROTTLE:
                    printf("[TELEMETRY] Hello from  \n");
                    state.throttle = msg.data.throttle.value;
                    break;
                case SUBSYS_STEERING:
                    printf("[TELEMETRY] Hello from brake \n");
                    state.steering_angle = msg.data.steering.angle;
                    break;
            }
            MsgReply(rcvid, EOK, NULL, 0);
        }
    }

    close(sockfd);
    return 0;
}
