#include "includes/defs.h"

// Used to setup socket connection thus far
// Might be more to come 

void socket_setup(){
    // setup socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // bind so we can receive
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family      = AF_INET;
    local.sin_port        = htons(LISTEN_PORT);
    local.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (struct sockaddr*)&local, sizeof(local));

    // setup destination
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(SEND_PORT);
    inet_pton(AF_INET, DEST_IP, &dest.sin_addr);


}
