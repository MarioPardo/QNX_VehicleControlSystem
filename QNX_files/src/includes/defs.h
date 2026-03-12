//Connection relevant libraries used 

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <includes/cJSON.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/neutrino.h>
#include <sys/siginfo.h>



//Networking
#define LISTEN_PORT 5555
#define SEND_PORT   5556
#define DEST_IP     "127.0.0.1"

extern int sockfd;
extern struct sockaddr_in dest;

void socket_setup();

void* send_loop(void* arg);
void* recv_loop(void* arg);


//Pulse Codes
typedef enum {
    PULSE_WATCHDOG_AUDIT = _PULSE_CODE_MINAVAIL + 1,
    PULSE_BRAKING_ALIVE,
    PULSE_BRAKING_INTERNAL
} SystemPulseCodes;
