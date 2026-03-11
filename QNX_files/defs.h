//Connection relevant libraries used 

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <cjson/cJSON.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>




#define LISTEN_PORT 5555
#define SEND_PORT   5556
#define DEST_IP     "127.0.0.1"

extern int sockfd;
extern struct sockaddr_in dest;

void setup();

void* send_loop(void* arg);
void* recv_loop(void* arg);
