#include "includes/defs.h"

//Main

int sockfd;
struct sockaddr_in dest;

pid_t watchdog_pid = -1;

void main_shutdown(int signo) {
    if (watchdog_pid > 0) {
        printf("[MAIN] Forwarding shutdown to watchdog (PID %d)...\n", watchdog_pid);
        kill(watchdog_pid, SIGTERM);
    }
    exit(0);
}

int main() {

	printf("Starting Vehicle Control System");

    signal(SIGTERM, main_shutdown);
    signal(SIGINT,  main_shutdown);

    //spawn everything as a processes using watchdog
    // This is what im doing right now

    //spawn watchdog as process
	printf("[MAIN] Spawning Watchdog Process...\n");
	watchdog_pid = spawnl(P_NOWAIT, "./watchdog", "watchdog", NULL);

	if (watchdog_pid == -1) {
		perror("[MAIN] Failed to spawn Watchdog");
		return -1;
	}
    //Give abit of time before closing main so that watchdogs can fuly spawn

    waitpid(watchdog_pid, NULL, 0);


    // // keep main alive
    // pthread_join(recv_thread, NULL);
    // pthread_join(send_thread, NULL);

    // close(sockfd);

    return 0;
}
