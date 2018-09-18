# Name

idle - Run processes based on processor idle state

# Synopsis

    idle -hksvz -i secs -t percent -- /path/to/command [args]

# Description

The **idle** program allows for processes to be started and stopped based on the processor idle state. Through a combination of command line arguments, it can start a user-specified process when the processor idle, as reported by /proc/stat and expressed as a percentage of overall processor execution time, exceeds a specified threshold. If the processor idle subsequently drops below this threshold, the user-specified process can be either terminated or suspended.

The user specified command should appear on the command-line, with all command-line parameters as required for execution, after parameters passed to the **idle** application and a "--" separator. While the full path to the user-specified application should be specified, the **idle** application will search the current *PATH* environment variable for the matching executable if no path is specified.

# Options

**-i** *secs*
:   Configures interval time across which processor execution time is measured. The interval time is measured in seconds and is expected to be an integer value. The default interval is 30 seconds.

**-t** *percent*
:   Configures the trigger threshold of idle time as a percentage of overall processor execution time at which the user-specified process should be started and stopped. The default trigger threshold is 90.0% processor idle time.

**-k**
:   Sets the program to terminate the user-specified process using the *KILL* signal when the processor idle time drops below the trigger threshold. The use of this signal ensures termination of the user-specified process, but does not provide it with any opportunity to perform clean-up operations. 

**-s**
:   Sets the program to terminate the user-specified process using a *TERM* signal when the processor idle time drops below the trigger threshold. This is the default mode of operations. 

**-z**
:   Sets the program to suspend the user-specified process using a *STOP* signal when the processor idle time drops below the trigger threshold. Operations of the user-specified process are resumed through the issue of a *CONT* signal when the processor idle time again exceeds the trigger threshold. This mode of operations is incompatible with the **-k** and **-s** command line options.

**-v**
:   Displays verbose program output.

**-h**
:   Displays program usage message.

# Examples

### Logging system idle time

    idle -i 300 -t 90 -- logger -p daemon.info "System is idle"

:   Writes "System is idle" to the system log whenever processor idle time exceeds 90% over a 5 minute polling period.

### Running resource intensive processes

    idle -i 30 -t 75 -z -- updatedb -l 0 -o ~/.mlocate.db -U ~
    
:   Update a private mlocate database for the local user where the processor idle time exceeds 75% over a 30 second polling period. If the processor idle time drops below this threshold on subsequent polling, the updatedb process will be suspended using a *STOP* signal and then resumed with a *CONT* signal once processor idle time again exceeds this 75% threshold. The expectation is that this is a command which would be combined with a cron-job for scheduled execution.

# Author

Rob Casey <rcasey@gmail.com>

# See Also

    kill
