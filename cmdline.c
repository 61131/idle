#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include "cmdline.h"
#include "log.h"


#define IS_DIR_SEPARATOR( c )   ( ( (c) == '/' ) || ( (c) == '\\' ) )


extern char *optarg;


static int _cmdline_duplicateArgs( IDLE *pIdle, int nArgv, char **pArgv );

static int _cmdline_findPath( char *pExec, char *pResult );

static int _cmdline_isExecutable( char *pPath );

static int _cmdline_isFloat( char *pStr );

static int _cmdline_isInteger( char *pStr );

static void _cmdline_printHelp( IDLE *pIdle );


/*!
    \func static int _cmdline_duplicateArgs( IDLE *pIdle, int nArgv, char **pArgv )
    \brief Duplicate command line arguments for external application invocation
    \param pIdle Pointer to application context data structure
    \param nArgv Number of command line arguments
    \param pArgv Pointer to pointer array of command line arguments
    \returns Returns zero on success, non-zero on error
*/

static int
_cmdline_duplicateArgs( IDLE *pIdle, int nArgv, char **pArgv ) {
    int nIndex;

    if( nArgv < 1 )
        return -EINVAL;

    if( ( pIdle->Args = ( char ** ) calloc( nArgv + 1, sizeof( char * ) ) ) == NULL )
        return -errno;
    for( nIndex = 0; nIndex < nArgv; ++nIndex ) {
        if( ! pArgv[ nIndex ] ) 
            continue;
        if( ( pIdle->Args[ nIndex ] = strdup( pArgv[ nIndex ] ) ) == NULL )
            return -errno;
    }
    pIdle->Args[ nArgv ] = NULL;

    return 0;
}


/*!
    \func static int _cmdline_findPath( char *pExec, char *pResult )
    \brief Returns full path to excutable file
    \param pStr Pointer to character string 
    \param pResult Pointer to result character string
    \returns Returns zero on success
*/

static int
_cmdline_findPath( char *pExec, char *pResult ) {
    char *pEnd, *pPath, *pStart;
    int nFound, nLength, nStop;

    if( strchr( pExec, '/' ) != NULL ) {
        if( realpath( pExec, pResult ) == NULL )
            return -errno;
        return ( _cmdline_isExecutable( pResult ) == 0 );
    }

    if( ( pPath = getenv( "PATH" ) ) == NULL )
        return -errno;
    if( strlen( pPath ) <= 0 )
        return -ENOENT;

    pStart = pPath;
    for( nFound = 0, nStop = 0;
            ( ! nStop && ! nFound ); ) {
        if( ( pEnd = strchr( pStart, ':' ) ) == NULL ) {
            ( void ) strncpy( pResult, pStart, PATH_MAX );
            nLength = strlen( pResult );
            ++nStop;
        }
        else {
            ( void ) strncpy( pResult, pStart, pEnd - pStart );
            pResult[ pEnd - pStart ] = '\0';
            nLength = pEnd - pStart;
        }

        if( pResult[ nLength - 1 ] != '/' )
            ( void ) strncat( pResult, "/", 1 );

        ( void ) strncat( pResult, pExec, PATH_MAX - nLength );
        nFound = _cmdline_isExecutable( pResult );

        if( ! nStop ) 
            pStart = pEnd + 1;
    }
    return ( nFound == 0 );
}


/*!
    \func static int _cmdline_isExecutable( char *pStr )
    \brief Helper function to check whether a string represents a path to an executable file
    \param pStr Pointer to character string
    \returns Return non-zero where true, zero where false
*/

static int
_cmdline_isExecutable( char *pPath ) {
    struct stat sInfo;
    int nFd, nResult;

    if( pPath == NULL )
        return 0;

    if( ( nFd = open( pPath, O_RDONLY ) ) < 0 )
        return /* -errno */ 0;

    for( nResult = 0;; ) {
        if( fstat( nFd, &sInfo ) < 0 )
            break;
        if( ! S_ISREG( sInfo.st_mode ) )
            break;

        if( sInfo.st_mode == geteuid() ) 
            nResult = ( sInfo.st_mode & S_IXUSR );
        else if( sInfo.st_mode == getegid() )
            nResult = ( sInfo.st_mode & S_IXGRP );
        else
            nResult = ( sInfo.st_mode & S_IXOTH );

        break;
    }

    close( nFd );
    return ( nResult > 0 );
}


/*!
    \func static int _cmdline_isFloat( char *pStr )
    \brief Helper function to check whether a string represents a floating-point (decimal) number
    \param pStr Pointer to character string
    \returns Returns non-zero where true, zero where false
*/

static int
_cmdline_isFloat( char *pStr ) {
    char *pIndex;
    int nSep;

    if( pStr == NULL )
        return 0;

    nSep = 0;
    for( pIndex = pStr; *pIndex != '\0'; ++pIndex ) {
        if( isdigit( *pIndex ) )
            continue;
        else if( ( *pIndex == '.' ) &&
                ( nSep++ == 0 ) )
            continue;
        else
            return 0;
    }
    return 1;
}


/*!
    \func static int _cmdline_isInteger( char *pStr )
    \brief Helper function to check whether a string represents an integer
    \param pStr Pointer to character string
    \returns Returns non-zero where true, zero where false
*/

static int
_cmdline_isInteger( char *pStr ) {
    char *pIndex;

    if( pStr == NULL ) 
        return 0;

    for( pIndex = pStr; *pIndex != '\0'; ++pIndex ) {
        if( ! isdigit( *pIndex ) )
            return 0;
    }
    return 1;
}


static void
_cmdline_printHelp( IDLE *pIdle ) {
    fprintf( stderr, "Usage: %s -hksvz -i secs -t percent -- /path/to/command [args]\n", pIdle->Name );
    fprintf( stderr, "Run processes based on processor idle state\n\n" );
    fprintf( stderr, "  -i secs       Polling interval time (in seconds)\n" );
    fprintf( stderr, "  -t percent    Trigger threshold for processor idle (in percent)\n" );
    fprintf( stderr, "  -k            Terminate user-specified process with KILL\n" );
    fprintf( stderr, "  -s            Terminate user-specified process with TERM (default)\n" );
    fprintf( stderr, "  -z            Suspend/resume user-specified process with STOP/CONT\n" );
    fprintf( stderr, "  -v            Display verbose output\n" );
    fprintf( stderr, "  -h            Display usage message\n\n" );
}


/*!
    \func int cmdline_parse( IDLE *pIdle, int nArgv, char **pArgv )
    \brief Parse application command line arguments
    \param pIdle Pointer to application context data structure
    \param nArgv Number of command line arguments
    \param pArgv Pointer to pointer array of command line arguments
    \returns Returns zero on success, non-zero on error
*/

int
cmdline_parse( IDLE *pIdle, int nArgv, char **pArgv ) {
    uint32_t uMask;
    char sExec[PATH_MAX];
    int nOpt;

    pIdle->Name = pArgv[0] + strlen( pArgv[0] );
    while( ( pIdle->Name != pArgv[0] ) &&
            ( ! IS_DIR_SEPARATOR( pIdle->Name[-1] ) ) ) 
        --pIdle->Name;

    opterr = 0;
    while( ( nOpt = getopt( nArgv, pArgv, "hi:kst:vz" ) ) != -1 ) {
        switch( nOpt ) {
            case 'i':
                if( ! _cmdline_isInteger( optarg ) ) {
                    fprintf( stderr, "%s: Invalid polling interval\n",
                            pIdle->Name );
                    return -1;
                }
                pIdle->Poll = strtoul( optarg, NULL, 10 );
                break;

            case 'k':
                pIdle->Flags |= F_KILL;
                /* break; */
            case 's':
                pIdle->Flags |= F_STOP;
                break;

            case 't':
                if( ! _cmdline_isFloat( optarg ) ) {
                    fprintf( stderr, "%s: Invalid trigger threshold\n",
                            pIdle->Name );
                    return -1;
                }
                pIdle->Threshold = strtof( optarg, NULL );
                break;

            case 'z':
                pIdle->Flags |= F_SUSPEND;
                break;

            case 'v':
                pIdle->Flags |= F_VERBOSE;
                break;

            case 'h':
                _cmdline_printHelp( pIdle );
               return -1;

            default:
                fprintf( stderr, "%s: Invalid option -- '%c'\n",
                        pIdle->Name,
                        optopt );
                return -1;
        }
    }

    uMask = ( F_STOP | F_SUSPEND );
    if( ( pIdle->Flags & uMask ) == 0 )
        pIdle->Flags |= F_STOP;
    if( ( pIdle->Flags & uMask ) == uMask )
        return -1;

    if( optind >= nArgv ) {
        fprintf( stderr, "%s: Missing executable argument\n",
                pIdle->Name );
        return -1;
    }

    sExec[0] = '\0';
    if( _cmdline_findPath( pArgv[ optind ], sExec ) != 0 ) {
        fprintf( stderr, "%s: Cannot find executable: %s\n",
                pIdle->Name,
                pArgv[ optind ] );
        return -1;
    }
    pArgv[ optind ] = sExec;    //  Ensure fully qualified path to executable, strdup?

    return _cmdline_duplicateArgs( pIdle, nArgv - optind, &pArgv[ optind ] );
}
