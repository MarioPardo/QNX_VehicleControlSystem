#include "includes/defs.h"


// client.c
// Will change back to this , rn just testing it
// void *receiveFromDashboard(int sockfd, int brake_coid, int throttle_coid, int steering_coid) {

int sockfd;
struct sockaddr_in dest;
name_attach_t *attach;
int watchdog_coid = -1;

void shutdown_client(int signo) {
    exit(0);  // triggers atexit handlers safely
}

void cleanup_client(void) {
    printf("[CLIENT] Exiting, detaching name...\n");
    name_detach(attach, 0);
}

void sendWatchdogHealthStatus(void)
{
    if (watchdog_coid == -1) return;
    MsgSendPulse(watchdog_coid, -1, PULSE_SUBSYSTEM_ALIVE, SUBSYS_CLIENT);
}

void *watchdog_thread(void *arg)
{
    int my_coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);

    struct sigevent event;
    SIGEV_PULSE_INIT(&event, my_coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_SUBSYSTEM_INTERNAL, SUBSYS_CLIENT);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);

    struct itimerspec itime;
    itime.it_value.tv_sec  = SYS_CLIENT_RESPONSETIME_MS / 1000;
    itime.it_value.tv_nsec = (SYS_CLIENT_RESPONSETIME_MS % 1000) * 1000000;
    itime.it_interval = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);

    struct _pulse pulse;
    while (1) {
        int rcvid = MsgReceive(attach->chid, &pulse, sizeof(pulse), NULL);
        if (rcvid == 0 && pulse.code == PULSE_SUBSYSTEM_INTERNAL)
            sendWatchdogHealthStatus();
    }
    return NULL;
}

void main_qnx_receiver(int sockfd, int brake_coid , int drive_coid) {
    char buffer[1024];
    
    while (1) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
        if (n < 0) continue;
        buffer[n] = '\0';

        // parse JSON into msg_packet
        msg_packet p;
        json_to_msg_packet(buffer, &p);

        //From here then on send the data to appropriate processes , braking etc 
        if (strcmp(p.subsys, "Brake") == 0) {
            MsgSend(brake_coid, &p, sizeof(p), NULL, 0);
           // printf("[CLIENT] : Received Braking data from Dashboard \n");
        }
        else if (strcmp(p.subsys, "Driving") == 0) {
            
            MsgSend(drive_coid, &p, sizeof(p), NULL, 0);
           // printf("[CLIENT] : Received driving data from Dashboard \n");
        }
        else if (strcmp(p.subsys, "Mode") == 0) {
            
            // TODO Mode process for this to be implemented
           // printf("[CLIENT] : Received Mode data from Dashboard \n");
        }


        // Testing comment to identify source of data 

        if (strcmp(p.origin, "VehicleData") == 0) {
            printf("[CLIENT] : Received Vehicle Data from Webots \n");

        }
        else if (strcmp(p.origin, "UserInput") == 0) {
          //  printf("[CLIENT] : Received User input data from dashboard \n");
        } 

    }      
    
}


//TODO if client is a process it must start checking in with watchdog potentially to prevent it from dying as well

// ------------------------------------------------------------------------------------------------------------------------
void connect_to_processes(int *brake_coid , int *driving_coid){
    while (*brake_coid == -1) {
        *brake_coid = name_open("braking_system", 0);
        if (*brake_coid == -1) {
            printf("[CLIENT] Waiting for braking...\n");
            sleep(1);
        }
    }
    printf("[CLIENT] Connected to braking\n");

    while (*driving_coid == -1) {
        *driving_coid = name_open("driving_system", 0);
        if (*driving_coid == -1) {
            printf("[CLIENT] Waiting for driving system...\n");
            sleep(1);
        }
    }
    printf("[CLIENT] Connected to driving\n");



}


int main(int argc, char *argv[]) {

    attach = name_attach(NULL, "client", 0);
    if (!attach) {
        printf("[CLIENT] name_attach failed\n");
        return -1;
    }
    atexit(cleanup_client);
    signal(SIGTERM, shutdown_client);
    signal(SIGINT,  shutdown_client);
    printf("[CLIENT] Registered as client\n");

    
    // Connect to all the proccess required
    
    // int mode_coid = -1;
    int brake_coid = -1;
    int driving_coid = -1;

    connect_to_processes(&brake_coid , &driving_coid);

    // connect to watchdog
    while (watchdog_coid == -1) {
        watchdog_coid = name_open("watchdog", 0);
        if (watchdog_coid == -1) {
            printf("[CLIENT] Waiting for watchdog...\n");
            sleep(1);
        }
    }
    printf("[CLIENT] Connected to watchdog\n");

    // start watchdog heartbeat thread
    pthread_t wd_thread;
    pthread_create(&wd_thread, NULL, watchdog_thread, NULL);

    // setup UDP socket for receiving from Python    
    sockfd=receiver_setup();

    printf("[CLIENT] Listening on port %d\n", LISTEN_PORT);

    // start receiving
   
    //Receives all incoming traffic into QNX port , contains all process id's that would be needed in transfer of data
    main_qnx_receiver(sockfd, brake_coid , driving_coid );


    return 0;
}


/*
// MODELING FOR REC FOR THE JSON BIT:

receive(){
    ccheck source:
        if from dash
            check subsytem()
                msgsend  to proper subsytem dependent}

        if from vehicle 
            check subsystem()
                msgsend to proper subsytem dependent } 

*/


/*
-> Client has to connect with all the processes it will want to communicate with.
-> This means to pass the coid of each of thee processes into the main receive function or atleast an array of them
-> Best case is to do this once since we dont have too many processes and its a processes that only happens oce when starting up 
    the client process.
-> The data structure being sent has to align with what the function on the other end expects in terms of subsys , type and data

-> These processes include:
        - Braking
        - Driving
        - Steering

    */
