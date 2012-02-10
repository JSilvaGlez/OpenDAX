/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2012 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*  Source code file for the daxmaster process handling application */

#include <common.h>
#include <process.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <sys/wait.h>
#include <opendax.h>
#include <mstr_config.h>

static int quitflag = 0;

void child_signal(int);
void quit_signal(int);
void catch_signal(int);

int
main(int argc, const char *argv[])
{
    struct sigaction sa;
    pthread_t message_thread;
	int result;
    
    /* Set up the signal handlers */
    memset (&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &quit_signal;
    sigaction (SIGQUIT, &sa, NULL);
    sigaction (SIGINT, &sa, NULL);
    sigaction (SIGTERM, &sa, NULL);
    
    sa.sa_handler = &catch_signal;
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_handler = &child_signal;
    sigaction (SIGCHLD, &sa, NULL);

    /* TODO: We should have individual configuration objects that we retrieve
     * from this function, instead of the global data in the source file. */
	opt_configure(argc, argv);
	process_start_all();
	_print_process_list();

	while(1) { /* Main loop */
        /* TODO: This might could be some kind of condition
           variable or signal thing instead of just the sleep(). */
        process_scan();
		sleep(1);
		       /* If the quit flag is set then we clean up and get out */
        if(quitflag) {
        	xlog(LOG_MAJOR, "Master quiting due to signal %d", quitflag);
            /* TODO: Need to kill the message_thread */
            /* TODO: Should stop all running modules */
            kill(0, SIGTERM); /* ...this'll do for now */
            exit(-1);
        }
 
	}
	
    exit(0);
}

/* this handles shutting down of the server */
/* TODO: There's the easy way out and then there is the hard way out.
 * I need to figure out which is which and then act appropriately.
 */
void
quit_signal(int sig)
{
    //xlog(LOG_MAJOR, "Quitting due to signal %d", sig);
    quitflag = sig;
}

/* TODO: May need to so something with signals like SIGPIPE etc */
void
catch_signal(int sig)
{
    xlog(LOG_MINOR, "Master received signal %d", sig);
}


/* Clean up any child modules that have shut down */
void
child_signal(int sig)
{
    int status;
    pid_t pid;

    do {
        pid = waitpid(-1, &status, WNOHANG);
        if(pid > 0) { 
            process_dpq_add(pid, status);
        }
    } while(pid > 0);
}
