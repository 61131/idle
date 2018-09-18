#include <string.h>

#include "siglist.h"


char * siglist[NSIG];


void
siglist_initialise( void ) {
    int nIndex;

    for( nIndex = 0; nIndex < NSIG; ++nIndex )
        siglist[ nIndex ] = NULL;

#ifdef SIGCHLD
    siglist[ SIGCHLD ] = "CHLD";
#endif
#ifdef SIGCONT
    siglist[ SIGCONT ] = "CONT";
#endif
#ifdef SIGINT
    siglist[ SIGINT ] = "INT";
#endif
#ifdef SIGKILL
    siglist[ SIGKILL ] = "KILL";
#endif
#ifdef SIGSTOP
    siglist[ SIGSTOP ] = "STOP";
#endif
#ifdef SIGTERM
    siglist[ SIGTERM ] = "TERM";
#endif
}
