#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "idle.h"
#include "log.h"
#include "siglist.h"


/*
    The following static global variable is intended to hold a pointer to the 
    application context structure for use within the signal handler. This is 
    required due to the lack of means by which a contextual parameter can be 
    reliably passed to the signal handler.
*/

static IDLE * _pIdle = NULL;


static void _idle_handler( int nSignal );

static int _idle_kill( IDLE *pIdle, int nSignal );

static int _idle_read( IDLE *pIdle, float *pPercent );

static void _idle_signal( int nSignal );

static int _idle_start( IDLE *pIdle );

static int _idle_stop( IDLE *pIdle );


/*!
    \func static void _idle_handler( int nSignal )
    \brief Process signal handler function
    \param nSignal Received process signal number
    \returns No return value
*/

static void
_idle_handler( int nSignal ) {
    int nPid, nStatus;

    if( ! _pIdle )
        return;

    switch( nSignal ) {
        case SIGALRM:
            _pIdle->Flags |= F_TRIGGER;
            break;

        case SIGINT:
        case SIGTERM:
            _pIdle->Flags |= F_EXIT;
            break;

        case SIGCHLD:
            while( ( nPid = waitpid( -1, &nStatus, WNOHANG ) ) > 0 ) {
                if( _pIdle->Pid == nPid ) {
                    if( _pIdle->Flags & F_VERBOSE ) {
                        log_info( "Received %s for process %s (pid %d)",
                                siglist[ nSignal ],
                                _pIdle->Args[0],
                                _pIdle->Pid );
                    }
                    _pIdle->Pid = -1;
                }
            }
            break;

        default:
            break;
    }
}


static int 
_idle_kill( IDLE *pIdle, int nSignal ) {
    if( pIdle->Flags & F_VERBOSE ) {
        log_info( "Issuing %s to process %s (pid %d)", 
                siglist[ nSignal ],
                pIdle->Args[0],
                pIdle->Pid );
    }
    return kill( pIdle->Pid, nSignal );
}


/*!
    \func static int _idle_read( IDLE *pIdle, float *pPercent )
    \brief Return processor idle percentage over previous read interval
    \param pIdle Pointer to application context structure
    \param pPercent Pointer to single float result variable
    \param Returns zero on success, non-zero on error
*/

static int
_idle_read( IDLE *pIdle, float *pPercent ) {
    char sBuffer[LINE_MAX];
    uint64_t uTotal, uValue[7];
    int nCount;

    if( ! pIdle->File ) {
        if( ! ( pIdle->File = fopen( "/proc/stat", "r" ) ) ) 
            return -errno;
    }
    rewind( pIdle->File );
    fflush( pIdle->File );

    if( ! fgets( sBuffer, sizeof( sBuffer ), pIdle->File ) )
        return -errno;
    nCount = sscanf( sBuffer, "cpu %lu %lu %lu %lu %lu %lu %lu",
            &uValue[0],
            &uValue[1],
            &uValue[2],
            &uValue[3],
            &uValue[4],
            &uValue[5],
            &uValue[6] );
    if( nCount < 7 )
        return -EFAULT;

    uTotal = uValue[0] + uValue[1] + uValue[2] + uValue[3] + uValue[4] + uValue[5] + uValue[6];
    *pPercent = ( ( float ) ( ( uValue[3] - pIdle->Ticks.Idle ) * 100 ) ) / ( ( float ) ( uTotal - pIdle->Ticks.Total ) );

    pIdle->Ticks.Idle = uValue[3];
    pIdle->Ticks.Total = uTotal;

    return 0;
}


/*!
    \func static void _idle_signal( int nSignal )
    \brief Helper function to install process signal handler
    \param nSignal Process signal number
    \returns No return value
*/

static void
_idle_signal( int nSignal ) {
    struct sigaction sAction;

    sAction.sa_handler = _idle_handler;
    sigemptyset( &sAction.sa_mask );
    sigaddset( &sAction.sa_mask, nSignal );
    sAction.sa_flags = 0;

    ( void ) sigaction( nSignal, &sAction, NULL );
}


/*!
    \func static int _idle_start( IDLE *pIdle )
    \brief Launch or resume external application based upon process load
    \param pIdle Pointer to application context structure
    \returns No return value
*/

static int 
_idle_start( IDLE *pIdle ) {
    int nResult;

    if( ( pIdle->Flags & F_EXIT ) != 0 )
        return 0;

    nResult = 0;
    if( ( pIdle->Pid < 0 ) ||
            ( kill( pIdle->Pid, 0 ) != 0 ) ) {  //  Valid process identifier?  No?  Okay ..

        if( ( pIdle->Pid = fork() ) == 0 ) {
            if( execve( pIdle->Args[0], pIdle->Args, NULL ) < 0 ) 
                exit( 0 );
        }

        if( pIdle->Flags & F_VERBOSE ) {
             log_info( "Starting process %s (pid %d)", 
                     pIdle->Args[0], 
                     pIdle->Pid );
        }
    }
    else if( pIdle->Flags & F_SUSPEND ) 
        nResult = _idle_kill( pIdle, SIGCONT );

    pIdle->Flags |= F_RUN;
    return nResult;
}


/*!
    \func static int _idle_stop( IDLE *pIdle )
    \brief Terminate or suspend external application based upon process load
    \param pIdle Pointer to application context structure
    \returns No return value
*/

static int 
_idle_stop( IDLE *pIdle ) {
    int nSignal;

    if( ( pIdle->Pid >= 0 ) &&
            ( kill( pIdle->Pid, 0 ) == 0 ) ) {  //  Valid process identifier?  Yes?  Okay ..

        nSignal = 0;
        switch( pIdle->Flags & ( F_STOP | F_SUSPEND ) ) {
            case F_STOP:
                nSignal = ( pIdle->Flags & F_KILL ) ? SIGKILL : SIGTERM;
                break;
                
            case F_SUSPEND:
                nSignal = SIGSTOP;
                break;

            default:
                break;
        }

        _idle_kill( pIdle, nSignal );
    }

    pIdle->Flags &= ~F_RUN;
    return 0;
}


/*!
    \func void idle_cleanup( IDLE *pIdle )
    \brief Graceful clean-up for application context structure
    \param pIdle Pointer to application context structure
    \returns No return value
*/

void
idle_cleanup( IDLE *pIdle ) {
    int nIndex;

    if( pIdle->Pid >= 0 )
        kill( pIdle->Pid, SIGKILL );

    if( pIdle->Args != NULL ) {
        for( nIndex = 0; pIdle->Args[ nIndex ] != NULL; ++nIndex )
            free( pIdle->Args[ nIndex ] );
        free( pIdle->Args ); 
    }
    if( pIdle->File != NULL ) 
        fclose( pIdle->File );
}


/*!
    \func int idle_initialise( IDLE *pIdle )
    \brief Initialise default values for application context structure
    \param pIdle Pointer to application context structure
    \returns No return value
*/

void
idle_initialise( IDLE *pIdle ) {
    _pIdle = pIdle;

    pIdle->File = NULL;
    pIdle->Flags = F_NONE;
    pIdle->Pid = -1;
    pIdle->Poll = 30;
    pIdle->Result = 0;
    pIdle->Threshold = 90.0;
    pIdle->Ticks.Idle = 0;
    pIdle->Ticks.Total = 0;
    pIdle->Args = NULL;

    siglist_initialise();       //  Remove siglist.c?

    _idle_signal( SIGALRM );
    _idle_signal( SIGCHLD );
    _idle_signal( SIGINT );
    _idle_signal( SIGTERM );
}


int
idle_loop( IDLE *pIdle ) {
    struct itimerval sTv;
    float fIdle;
    bool bThreshold;

    if( pIdle->Poll < 1 ) 
        pIdle->Poll = 1;

    sTv.it_value.tv_sec = pIdle->Poll;
    sTv.it_value.tv_usec = 0;
    sTv.it_interval.tv_sec = sTv.it_value.tv_sec;
    sTv.it_interval.tv_usec = sTv.it_value.tv_usec;

    if( setitimer( ITIMER_REAL, &sTv, NULL ) != 0 ) 
        return -errno;

    pIdle->Result = 0;
    while( ( pIdle->Flags & F_EXIT ) == 0 ) {

        if( pIdle->Flags & F_TRIGGER ) {

            fIdle = 0.0;
            if( ( pIdle->Result = _idle_read( pIdle, &fIdle ) ) != 0 ) {
                pIdle->Flags |= F_EXIT;
                break;
            }

            bThreshold = ( ( fIdle - pIdle->Threshold ) > 0.0 );
            switch( pIdle->Flags & F_RUN ) {
                case F_RUN:
                    if( bThreshold )
                        break;

                    if( pIdle->Flags & F_VERBOSE )
                        log_debug( "Processor idle %.1f%% under trigger threshold %.1f%%",
                                fIdle,
                                pIdle->Threshold );
                    _idle_stop( pIdle );
                    break;

                default:
                    if( ! bThreshold )
                        break;

                    if( pIdle->Flags & F_VERBOSE )
                        log_debug( "Processor idle %.1f%% over trigger threshold %.1f%%",
                                fIdle,
                                pIdle->Threshold );
                    _idle_start( pIdle );
                    break;
            }

            pIdle->Flags &= ~F_TRIGGER;
        }

        ( void ) select( 0, NULL, NULL, NULL, NULL );
    }
    return pIdle->Result;
}

