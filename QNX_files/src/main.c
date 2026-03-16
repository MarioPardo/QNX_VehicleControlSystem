#include "includes/defs.h"

//Main

int sockfd;
struct sockaddr_in dest;

int main() {

	printf("Starting Vehicle Control System");

    // setup socket
    socket_setup();

    // launch networking threads
    pthread_t recv_thread, send_thread; 
    pthread_create(&recv_thread, NULL, recv_loop, NULL);
    pthread_create(&send_thread, NULL, send_loop, NULL);


    //spawn watchdog as process
	printf("[MAIN] Spawning Watchdog Process...\n");
	pid_t watchdog_pid = spawnl(P_NOWAIT, "./watchdog", "watchdog", NULL);

	if (watchdog_pid == -1) {
		perror("[MAIN] Failed to spawn Watchdog");
		return -1;
	}

    // keep main alive
    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sockfd);

    return 0;
}
