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

/* Source file for the process handling functions for the master process
 * handling program. */

#include <process.h>
#include <sys/time.h>
#include <logger.h>

/* This array is the dead process list.
   TODO: This should be improved to allow it to grow when needed.
*/
#ifndef DPQ_SIZE
  #define DPQ_SIZE 10
#endif
static dead_process _dpq[DPQ_SIZE];

static dax_process *_process_list = NULL;


/* Convert a string like "-d -x -y" to a NULL terminated array of
 * strings suitable as an arg list for the arg_list of an exec??()
 * function. */
static char **
_arglist_tok(char *path, char *str)
{
    char *temp, *token, *save;
    char **arr;
    int count = 1;
    int index = 1;
    save=temp=NULL;
    
    if(str!=NULL) {
        /* We need a non const char string for strtok */
        temp = strdup(str);
    }
    /* First we count the number of tokens */
    if(temp) {
        token = strtok_r(temp," ",&save);
        while(token) {
            count++;
            token = strtok_r(NULL," ",&save);
        }
    }
    /* Allocate the array, 1 extra for the NULL */
    if(path || temp) {
        arr = malloc((count + 1) * sizeof(char*));
        if(arr == NULL) return NULL; /* OOOPS No Memory left */
    } else { /* No path supplied either */
        return NULL;
    }
    
    arr[0] = strdup(path); /* Put the path into the first argument */
    
    /* Now we re-parse the string to add the tokens to the array
       First we have to copy str to temp again.  No sense in using
       strdup() this time since the string is the same and the memory
       has already been allocated */
    if(temp) {
        strcpy(temp, str);
        token = strtok_r(temp, " ", &save);
        while(token) {
            /* allocate and get a copy of the token and save it into the array */
            arr[index++] = strdup(token); /* TODO: ERROR CHECK THIS */
            token = strtok_r(NULL, " ", &save);
        }
        arr[index] = NULL;
        
        free(temp);
    }
    return arr;
}

dax_process *
process_add(char *name, char *path, char *arglist, unsigned int flags)
{
    dax_process *new, *this;
    xlog(LOG_CONFIG,"Adding process %s to configuration",name);
    
    new = malloc(sizeof(dax_process));
    if(new) {
        new->pty_fd = 0;
        new->fd = 0;
        new->efd = 0;
        new->pid = 0;
        
        /* Add the module path to the struct */
        if(path) {
            new->path = strdup(path);
            
            /* tokenize and set arglist */
            new->arglist = _arglist_tok(path, arglist);
        }
        /* name the module */
        new->name = strdup(name);
        new->next = NULL;

        if(_process_list == NULL) { /* List is empty */
            _process_list = new;
        } else {
            this = _process_list;
            while(this->next != NULL) this = this->next;
            this->next = new;
        }
        return new;
    } else {
        return NULL;
    }
}

static void
_free_process(dax_process *proc)
{
    char **node;

    /* free allocated memory */
    if(proc->path) free(proc->path);
    if(proc->name) free(proc->name);
    if(proc->arglist) {
        node=proc->arglist;
        while(*node) {
            free(*node);
            node++;
        }
        free(proc->arglist);
    }
    free(proc);
}

/* Deletes the module from the list and frees the memory */
int
process_del(dax_process *proc)
{
    dax_process *last, *this;

    if(proc) {

        if(proc == _process_list) { /* If this is the first process */
            _process_list = _process_list->next;
        } else {
            last = proc;
            while(this != NULL) {
                this = proc->next;
                if(this == proc) {
                    _free_process(proc);

                    return 0;
                }
                last = this;
                this = this->next;
            }
        }
    }
    return ERR_ARG;
}


/* Start all of the processes in the process list */
void
process_start_all(void)
{
    dax_process *this;
    struct timespec timeout, rem;
    int result;

    /* In case we ain't go no list */
    if(_process_list == NULL) return;

    this = _process_list;
    while(this != NULL) {
        process_start(this);

        if(this->delay > 0) {
            rem.tv_sec = this->delay / 1000;
            rem.tv_nsec = (this->delay % 1000) * 1000000;
            result = 1;
            while(result) {
                timeout = rem;
                result = nanosleep(&timeout, &rem);
                if(errno == EFAULT)
                    xfatal("Serious problem with nanosleep in function process_start_all()");
                assert(errno != EINVAL);
            }
        }
        this = this->next;
    }
}

/* This function is used to start a module */
pid_t
process_start(dax_process *proc)
{
    pid_t child_pid;
    int result = 0;
    int pty_fd;
 
    if(proc) { /* We are the parent */
        if(result) xerror("Unable to properly set up pipes for %s\n", proc->name);
        child_pid = fork();
        if(child_pid > 0) { /* This is the parent */
            proc->pid = child_pid;
            proc->exit_status = 0;

            xlog(LOG_VERBOSE, "Starting Process - %s - %d",proc->path,child_pid);
            proc->starttime = time(NULL);
            proc->pty_fd = pty_fd;
            proc->state = PSTATE_STARTED;
            return child_pid;
        } else if(child_pid == 0) { /* Child */
            /* TODO: Environment???? */
            /* TODO: Change the UID of the process */
            if(execvp(proc->path, proc->arglist)) {
                xerror("start_module exec failed - %s - %s",
                       proc->path, strerror(errno));
                exit(errno);
            }
        } else { /* Error on the fork */
            xerror("start_module fork failed - %s - %s", proc->path, strerror(errno));
        }
    } else {
        return 0;
    }
    return 0;
}

/* Finds and returns a pointer to the process with the given PID */
static dax_process *
_get_process_pid(pid_t pid)
{
    dax_process *this;

    this = _process_list;
    while(this != NULL) {
        if(this->pid == pid) return this;
        this = this->next;
    }
    return NULL;
}

/* This function is called from the scan_modules function and is used 
 * to find and cleanup the module after it has died.  */
static int
_cleanup_process(pid_t pid, int status)
{
    dax_process *proc;
    
    proc = _get_process_pid(pid);

    /* at this point _current_mod should be pointing to a module with
     * the PID that we passed but we should check because there may not 
     * be a module with our PID */
    if(proc) {
        xlog(LOG_MINOR, "Cleaning up Process %d", pid);
        proc->pid = 0;
        proc->exit_status = status;
        proc->state = PSTATE_DEAD;
        return 0;
    } else {
        xerror("Process %d not found \n", pid);
        return ERR_NOTFOUND;
    }
    return 0;
}


/* This function scans the modules to see if there are any that need
   to be cleaned up or restarted.  This function should never be called
   from the start_module process or the child signal handler to avoid a
   race condition. */
void
process_scan(void)
{
    int n;
    dax_process *this;

    /* Check the dead module queue for pid's that need cleaning */
    for(n = 0; n < DPQ_SIZE; n++) {
        if(_dpq[n].pid != 0) {
            _cleanup_process(_dpq[n].pid, _dpq[n].status);
            _dpq[n].pid = 0;
        }
    }

    this = _process_list;
    while(this != NULL) {
        this = this->next;
    }
    /* TODO: Restart modules if necessary */
}


/* TODO: Program process_stop() */
int
process_stop(dax_process *proc)
{
    return 0;
}

/* Adds the dead module to the first blank spot in the list.  If the list
   is overflowed then it'll just overwrite the last one.
   TODO: If more than DMQ_SIZE modules dies all at once this will cause
         problems.  Should be fixed. */
void
process_dpq_add(pid_t pid, int status)
{
    int n = 0;
    while(_dpq[n].pid != 0 && n < DPQ_SIZE) {
        n++;
    }
    _dpq[n].pid = pid;
    _dpq[n].status = status;
}

void
_print_process_list(void)
{
    dax_process *this;
    this = _process_list;
    char **args;
    int n = 0;

    while(this != NULL) {
        printf("Process %s\n", this->name);
        printf("  path    = %s\n", this->path);
        args = this->arglist;
        while(args[n] != NULL) {
            printf("  arg[%d]    = %s\n", n, args[n]);
            n++;
        }
        printf("  user     = %s\n", this->user);
        printf("  uid      = %d\n", this->uid);
        printf("  group    = %s\n", this->group);
        printf("  gid      = %d\n", this->gid);
        printf("  env      = %s\n", this->env);
        printf("  delay    = %d\n", this->delay);
        printf("  cpu      = %f\n", this->cpu);
        printf("  mem      = %d kB\n", this->mem);
        printf("  pid      = %d\n", this->pid);
        printf("  pty_fd = %d\n", this->pty_fd);

        this = this->next;
    }
}


