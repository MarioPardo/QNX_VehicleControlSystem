#include "defs.h"

//Main

int main() {
    // setup socket
    setup();

    // launch both threads
    pthread_t recv_thread, send_thread;
    pthread_create(&recv_thread, NULL, recv_loop, NULL);
    pthread_create(&send_thread, NULL, send_loop, NULL);

    // keep main alive
    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sockfd);
    return 0;
}