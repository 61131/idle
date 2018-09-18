#include "cmdline.h"
#include "idle.h"


int
main( int nArgv, char **pArgv ) {
    IDLE sIdle;
    int nResult;

    idle_initialise( &sIdle );
    if( ( nResult = cmdline_parse( &sIdle, nArgv, pArgv ) ) != 0 ) 
        return nResult;

    nResult = idle_loop( &sIdle );
    idle_cleanup( &sIdle );

    return nResult;
}
