#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <process.h>
#include <time.h>
#include "includes/defs.h"



// Just added some stuff to measure for now , telemetry and client


//Subsystem info
#define MAX_SUBSYSTEMS 10

	//index for each system in table
#define SYS_BRAKING 0
#define SYS_TELEMETRY 1
#define SYS_CLIENT 2

	//response times : how often we expect them to check in
#define SYS_BRAKING_RESPONSETIME_MS 500
#define SYS_CLIENT_RESPONSETIME_MS 1000
#define SYS_TELEMETRY_RESPONSETIME_MS 10000

	//critical times : how many MS since last check in such that we kill and restart process
#define SYS_BRAKING_CRITICALTIME_MS 3000




//table storing subsystem info
typedef struct {
    pid_t pid;
    uint64_t lastTimeReported;
    int responseTime;
    int isAlive;
    int chid;
    //TODO add string to store subsystem name
} SubsystemRecord;

SubsystemRecord processTable[MAX_SUBSYSTEMS];



////// FUNCTIONS///////////////////

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

    return; //temp, still have to implement logic

    /*

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
    */
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

////////


//// Subsystem Spawning

// Ok so I completely dettached this process so as to have one fucntion that can spawn all processes , will tweak this again in the future to make sure i can add priority. 

int spawn_subsystem(const char *path, const char *name, 
                    int table_index, int response_time_ms,
                    char *pid_str, char *chid_str) {
    
    pid_t pid = spawnl(P_NOWAIT, path, name,
                       "-p", pid_str, "-c", chid_str, NULL);
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
void beginSubsystems(int watchdog_chid) {
    printf("[WATCHDOG] Spawning Subsystems...\n");
    pid_t my_pid = getpid();
    char pid_str[16], chid_str[16];
    sprintf(pid_str,  "%d", my_pid);
    sprintf(chid_str, "%d", watchdog_chid);

    //Ill just add processes here instead , will be easier to scale instead of create a subsystem function for each process
    // Other work we'd need to do in the process can just go in the respective file 

    spawn_subsystem("./telemetry_system", "telemetry_system", SYS_TELEMETRY, SYS_TELEMETRY_RESPONSETIME_MS, pid_str, chid_str);
    spawn_subsystem("./braking_system",   "braking_system",   SYS_BRAKING,   SYS_BRAKING_RESPONSETIME_MS, pid_str, chid_str);
    spawn_subsystem("./client",           "client",           SYS_CLIENT,    SYS_CLIENT_RESPONSETIME_MS, pid_str, chid_str);
}


int main()
{
	printf("[WATCHDOG] Hello from Watchdog!\n");
    int watchdog_chid = ChannelCreate(0);
    int coid = ConnectAttach(ND_LOCAL_NODE, 0, watchdog_chid, _NTO_SIDE_CHANNEL, 0);
    printf("[WATCHDOG] CHID:%d, COID:%d   \n", watchdog_chid, coid);


    //create signal to wake itself up
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
                case PULSE_TELEMETRY_ALIVE:
                    update_last_reported(SYS_TELEMETRY);
                    break;
            }
        }
    }

    return 0;
}
