#include "includes/defs.h"

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

// Responsible for sendngi
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



