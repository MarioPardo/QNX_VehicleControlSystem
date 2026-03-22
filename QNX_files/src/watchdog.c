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

//table storing subsystem info
typedef struct {
    pid_t pid;
    uint64_t lastTimeReported;
    int responseTime; //ms 
    int criticalTime; //ms
    int isAlive;
    int chid;
    char* subsystemName;
} SubsystemRecord;

SubsystemRecord processTable[MAX_SUBSYSTEMS];



////// FUNCTIONS///////////////////

void watchdog_shutdown(int signo)
{
    printf("[WATCHDOG] Shutting down (signal %d), killing subsystems...\n", signo);
    for (int i = 0; i < MAX_SUBSYSTEMS; i++) {
        if (processTable[i].isAlive && processTable[i].pid > 0) {
            printf("[WATCHDOG] Killing PID %d\n", processTable[i].pid);
            kill(processTable[i].pid, SIGTERM);
        }
    }
    exit(0);
}

////helpers ///

uint64_t get_current_time_ms()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (time.tv_sec * 1000) + (time.tv_nsec / 1000000);
}


/// health monitoring////

void run_health_diagnostics()
{
    uint64_t now = get_current_time_ms();

    printf("[WATCHDOG] Monitoring Subsystem Health.. \n");


    for (int i = 0; i < MAX_SUBSYSTEMS; i++) 
    {

        if (processTable[i].isAlive) 
        {
            uint64_t timeSinceLastReport = now - processTable[i].lastTimeReported;

            //if last time they responded falls within threshold -OK
            if (timeSinceLastReport <= processTable[i].responseTime)
            {
                printf("[WATCHDOG] Subsystem %s is healthy.\n", processTable[i].subsystemName);
                continue;
            } 
            // if missed heartbeat
            else if ( (timeSinceLastReport >= (processTable[i].responseTime)) &&  timeSinceLastReport < processTable[i].criticalTime)
            {
                printf("[WATCHDOG] WARNING: Subsystem %s missed a heartbeat. MONITORING.\n", processTable[i].subsystemName);
            }

            else  { //critical
                printf("[WATCHDOG] CRITICAL: Subsystem %s failed. KILLING PID %d\n", processTable[i].subsystemName, processTable[i].pid);
                // kill(processTable[i].pid, SIGKILL);
                // Trigger respawn logic here
            }
        }
    }

}

void update_last_reported(int sys_id)
{
	printf("[WATCHDOG] Subsystem [%d]:%s  has checked in! \n", sys_id, processTable[sys_id].subsystemName);

	//update corresponding system's health
    if (sys_id >= 0 && sys_id < MAX_SUBSYSTEMS)
    {
        processTable[sys_id].lastTimeReported = get_current_time_ms();
    }
}

////////


//// Subsystem Spawning
int spawn_subsystem(const char *path, const char *name,
                    int table_index, int response_time_ms, char* subsystemName) {

    printf("[WATCHDOG]  -- %s\n", subsystemName);

    pid_t pid = spawnl(P_NOWAIT, path, name, NULL);
    usleep(50000);

    if (pid == -1) {
        printf("[WATCHDOG] ERROR spawning %s\n", name);
        return -1;
    }

    // update process table
    processTable[table_index].pid              = pid;
    processTable[table_index].lastTimeReported = get_current_time_ms();
    processTable[table_index].responseTime     = response_time_ms;
    processTable[table_index].isAlive          = 1;
    processTable[table_index].subsystemName    = strdup(subsystemName);

    // find its channel
    int coid = -1;
    int retries = 10;
    while (coid == -1 && retries-- > 0) {
        coid = name_open(name, 0);
        if (coid == -1) usleep(200000);  // 100ms retry
            }

    if (coid == -1) {
        printf("[WATCHDOG] Unable to find %s channel\n", name);
        processTable[table_index].isAlive = 0;
        return -1;
    }

    processTable[table_index].chid = coid;
    printf("[WATCHDOG] %s started. PID: %d\n", name, pid);
    return 0;
}

//spawn all subsystems, send them relevant info so they can communicate with watchdog
void beginSubsystems() {
    //possiblyTODO refactor into for loop
    printf("[WATCHDOG] Spawning Subsystems...\n");

    if (!spawn_subsystem("./telemetry_system", "telemetry_system", SUBSYS_TELEMETRY, SYS_TELEMETRY_RESPONSETIME_MS, "Telemetry System")) {
        printf("[WATCHDOG] Failed to spawn Telemetry System\n");
    }

    if (!spawn_subsystem("./braking_system",   "braking_system", SUBSYS_BRAKE, SYS_BRAKING_RESPONSETIME_MS, "Braking System")) {
        printf("[WATCHDOG] Failed to spawn Braking System\n");
    }

    if (!spawn_subsystem("./client", "client", SUBSYS_CLIENT, SYS_CLIENT_RESPONSETIME_MS, "Client")) {
        printf("[WATCHDOG] Failed to spawn Client\n");
    }
}


int main()
{
	printf("[WATCHDOG] Hello from Watchdog!\n");
    name_attach_t *attach = name_attach(NULL, "watchdog", 0);
    if (!attach) {
        printf("[WATCHDOG] name_attach failed: %d (stale instance may be running - run 'slay watchdog telemetry_system braking_system client')\n", errno);
        return -1;
    }
    int coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid, _NTO_SIDE_CHANNEL, 0);
    printf("[WATCHDOG] CHID:%d, COID:%d   \n", attach->chid, coid);


    //register function to shut down properly
    signal(SIGTERM, watchdog_shutdown);
    signal(SIGINT,  watchdog_shutdown);

    //create signal to wake itself up
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, PULSE_WATCHDOG_AUDIT, 0);
    timer_t timer_id;
    timer_create(CLOCK_MONOTONIC, &event, &timer_id);
    struct itimerspec itime;
    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 50000000 * 10; // 50ms audit cycle * 10 so its 500ms for debugging
    itime.it_interval = itime.it_value;
    timer_settime(timer_id, 0, &itime, NULL);


    beginSubsystems();

    struct _pulse pulse;
    printf("[WATCHDOG] Monitoring active...\n");

    while (1) {
        int rcvid = MsgReceive(attach->chid, &pulse, sizeof(pulse), NULL);

        if (rcvid == 0) {
            switch (pulse.code) 
            {
                case PULSE_WATCHDOG_AUDIT:
                    run_health_diagnostics();
                    break;
                case PULSE_SUBSYSTEM_ALIVE:
                    update_last_reported(pulse.value.sival_int);
                    break;

                
            }
        }
    }

    return 0;
}
