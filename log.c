#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "log.h"


static char * _pLevel[] = {
        "emerg",
        "alert",
        "crit",
        "error",
        "warn",
        "notice",
        "info",
        "debug"
};


int log_level = LOG_DEBUG;


void
log_message( int nPriority, const char *pFormat, ... ) {
    va_list sArgs;

    if( nPriority <= log_level ) {
        va_start( sArgs, pFormat );

        fprintf( stderr, "%s: ", _pLevel[ nPriority ] );
        vfprintf( stderr, pFormat, sArgs );
        fprintf( stderr, "\n" );
        fflush( stderr );

        va_end( sArgs );
    }

    if( nPriority == LOG_EMERG ) 
        exit( 1 );
}
