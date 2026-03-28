#include "includes/defs.h"

// Used to setup socket connection thus far
// Might be more to come 

int sender_setup(const char *ip, int port , struct sockaddr_in *dst) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("[SOCKET] Failed to create send socket\n");
        return -1;
    }

    
    memset(dst, 0, sizeof(*dst));
    dst->sin_family = AF_INET;
    dst->sin_port   = htons(port);
    inet_pton(AF_INET, ip, &dst->sin_addr);

    return fd;
}

int receiver_setup(){

     // setup socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    // bind so we can receive
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family      = AF_INET;
    local.sin_port        = htons(LISTEN_PORT);
    local.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (struct sockaddr*)&local, sizeof(local));
    return fd;
}

