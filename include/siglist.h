#ifndef __SIGLIST_H
#define __SIGLIST_H


#include <signal.h>


extern char * siglist[NSIG];


void siglist_initialise( void );


#endif
