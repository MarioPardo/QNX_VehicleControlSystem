#include "includes/defs.h"


// client.c
// Will change back to this , rn just testing it 
// void *receiveFromDashboard(int sockfd, int brake_coid, int throttle_coid, int steering_coid) {

int sockfd;
struct sockaddr_in dest;

void receiveFromDashboard(int sockfd, int brake_coid) {
    char buffer[1024];
    
    while (1) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
        if (n < 0) continue;
        buffer[n] = '\0';

        // parse JSON into msg_packet
        msg_packet p;
        json_to_msg_packet(buffer, &p);

        // route to correct process
        if (strcmp(p.msg_type, "BrakingInput") == 0) {
            MsgSend(brake_coid, &p.msg, sizeof(p.msg), NULL, 0);
            printf("[CLIENT] : Received braking data from Dashboard.");
        }
        // undo when implemented and done with testing
        // else if (strcmp(p.msg_type, "ThrottleInput") == 0) {
        //     MsgSend(throttle_coid, &p.msg, sizeof(p.msg), NULL, 0);
        // }
        // else if (strcmp(p.msg_type, "SteeringInput") == 0) {
        //     MsgSend(steering_coid, &p.msg, sizeof(p.msg), NULL, 0);
        // }
    }
}


// ---------------------------------------------------------------------------------------------------------------
//Might not need these anymore but have not decided yet if I will , might need them in the future 
// This defines the receiver and sender code so far without parsing 

//This is the receiver loop 
void* recv_loop(void* arg) {
    char buffer[1024];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    printf("Listening on port %d...\n", LISTEN_PORT);

    while(1) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr*)&sender, &sender_len);
        if(n < 0) continue;

        buffer[n] = '\0';
       // printf("[RECV] %s\n", buffer);

        // later use json parsing function 
    }

    return NULL;
}

// Responsible for sending data over network
// Load buffer to be sent over 
void* send_loop(void* arg) {
    int seq = 0;
    char payload[256];

    while(1) {
        // hand build JSON string for now
        snprintf(payload, sizeof(payload),
            "{\"msg_type\":\"SPEED\",\"value\":%d,\"seq\":%d}",
            60 + (seq % 20), seq);

        sendto(sockfd, payload, strlen(payload), 0,
               (struct sockaddr*)&dest, sizeof(dest));

        //printf("[SENT] %s\n", payload);

        seq++;
        sleep(1);
    }

    return NULL;
}

// ------------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

    name_attach_t *attach = name_attach(NULL, "client", 0);
    if (!attach) {
        printf("[CLIENT] name_attach failed\n");
        return -1;
    }
    printf("[CLIENT] Registered as client\n");

    int brake_coid = -1;
    while (brake_coid == -1) {
        brake_coid = name_open("braking_system", 0);
        if (brake_coid == -1) {
            printf("[CLIENT] Waiting for braking...\n");
            sleep(1);
        }
    }
    printf("[CLIENT] Connected to braking\n");

    // setup UDP socket for receiving from Python
    
    receiver_setup();

    printf("[CLIENT] Listening on port %d\n", LISTEN_PORT);

    // start receiving
    // receiveFromDashboard(sockfd, brake_coid, throttle_coid, steering_coid);
    
    // For testing purposes
    receiveFromDashboard(sockfd, brake_coid);


    return 0;
}
