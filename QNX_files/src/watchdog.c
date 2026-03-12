#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <process.h>
#include <time.h>
#include "includes/defs.h"


//Subsystem info
#define MAX_SUBSYSTEMS 10
#define SYS_BRAKING 0

typedef struct {
    pid_t pid;
    uint64_t lastTimeReported;
    int pingFrequencyMs;
    int isAlive;
    int chid;
    //TODO add string to store subsystem name
} SubsystemRecord;

SubsystemRecord processTable[MAX_SUBSYSTEMS];



////// FUNCTIONS
uint64_t get_current_time_ms()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
}

void run_health_diagnostics()
{
    uint64_t now = get_current_time_ms();

    printf("[WATCHDOG] Monitoring Subsystem Health.. \n");

    return; //temp

    for (int i = 0; i < MAX_SUBSYSTEMS; i++) {

        if (processTable[i].isAlive) {
            uint64_t timeSinceLastReport = now - processTable[i].lastTimeReported;

            //if last time they responded falls within threshold -OK
            if (timeSinceLastReport <= processTable[i].pingFrequencyMs)
            {
                continue;
            }
            // if not, check if it sits within warning threshhold
            else if (timeSinceLastReport <= (processTable[i].pingFrequencyMs * 2)) //TODO set up proper warning threshold
            {
                printf("[WATCHDOG] WARNING: Subsystem %d missed a heartbeat.\n", i);
            }

            else {// if critical, restart process
                printf("[WATCHDOG] CRITICAL: Subsystem %d failed. Executing PID %d\n", i, processTable[i].pid);
                // kill(processTable[i].pid, SIGKILL);
                // Trigger respawn logic here
            }
        }
    }
}

void update_last_reported(int sys_id)
{
	printf("[WATCHDOG] SysID:%d  has checked in! \n", sys_id);

	//update corresponding system's health
    if (sys_id >= 0 && sys_id < MAX_SUBSYSTEMS)
    {
        processTable[sys_id].lastTimeReported = get_current_time_ms();
    }
}

// TODO make own function for spawning each subsystem


void spawn_braking_system(char* pid_str, char* chid_str)
{
    pid_t braking_pid = spawnl(P_NOWAIT, "./braking_system", "braking_system", "-p", pid_str, "-c", chid_str, NULL);
    usleep(50000);

    if (braking_pid != -1) {
        processTable[SYS_BRAKING].pid = braking_pid;
        processTable[SYS_BRAKING].lastTimeReported = get_current_time_ms();
        processTable[SYS_BRAKING].pingFrequencyMs = 50;
        processTable[SYS_BRAKING].isAlive = 1;
        int braking_coid = name_open("braking_system", 0);
        if( braking_coid == -1)
        		printf("[WATCHDOG]Unable to find COID CHID \n");
        processTable[SYS_BRAKING].chid = braking_coid;
        printf("[WATCHDOG] Braking System started. PID: %d\n", braking_pid);
    }
}

void beginSubsystems(int watchdog_chid)
{
	printf("[WATCHDOG] Spawning Subsystems.. \n");
    pid_t my_pid = getpid();
    char pid_str[16], chid_str[16];
    sprintf(pid_str, "%d", my_pid);
    sprintf(chid_str, "%d", watchdog_chid);

    spawn_braking_system(pid_str, chid_str);
}

int main()
{

    int watchdog_chid = ChannelCreate(0);
    int coid = ConnectAttach(ND_LOCAL_NODE, 0, watchdog_chid, _NTO_SIDE_CHANNEL, 0);
    printf("[WATCHDOG] CHID:%d, COID:%d   \n", watchdog_chid, coid);


    //create signal to wake itseulf up
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_WATCHDOG_AUDIT, 0);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);
    struct itimerspec itime;
    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 50000000; // 50ms audit cycle
    itime.it_interval = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);


    beginSubsystems(watchdog_chid);

    struct _pulse pulse;
    printf("[WATCHDOG] Monitoring active...\n");

    while (1) {
        int rcvid = MsgReceive(watchdog_chid, &pulse, sizeof(pulse), NULL);

        if (rcvid == 0) {
            switch (pulse.code) {
                case PULSE_WATCHDOG_AUDIT:
                    run_health_diagnostics();
                    break;
                case PULSE_BRAKING_ALIVE:
                    update_last_reported(SYS_BRAKING);
                    break;
            }
        }
    }

    return 0;
}
