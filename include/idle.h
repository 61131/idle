#ifndef __IDLE_H
#define __IDLE_H


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>


enum {
    F_NONE      = ( 0 << 0 ),
    F_EXIT      = ( 1 << 0 ),
    F_KILL      = ( 1 << 1 ),
    F_RUN       = ( 1 << 2 ),
    F_STOP      = ( 1 << 3 ),
    F_SUSPEND   = ( 1 << 4 ),
    F_TRIGGER   = ( 1 << 5 ),
    F_VERBOSE   = ( 1 << 6 ),
};


/*!
    \typedef IDLE
    \brief Application context data structure
*/

typedef struct __IDLE {

    FILE *File;

    sig_atomic_t Flags;

    char *Name;

    pid_t Pid;

    uint32_t Poll;

    int32_t Result;

    float Threshold;

    struct {

        uint64_t Idle;

        uint64_t Total;
    }
    Ticks;

    char **Args;
}
IDLE;


void idle_cleanup( IDLE *pIdle );

void idle_initialise( IDLE *pIdle );

int idle_loop( IDLE *pIdle );


#endif 
