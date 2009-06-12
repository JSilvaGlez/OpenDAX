/* modbus.c - Modbus (tm) Communications Library
 * Copyright (C) 2006 Phil Birkelbach
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//--#include <database.h>


#include <modlib.h>
#include <modbus.h>

static int openport(mb_port *);
static int openIPport(mb_port *);

void masterRTUthread(mb_port *);

/* Calculates the difference between the two times */
inline unsigned long long
timediff(struct timeval oldtime,struct timeval newtime)
{
    return (newtime.tv_sec - oldtime.tv_sec) * 1000 + 
           (newtime.tv_usec / 1000)-(oldtime.tv_usec / 1000);
};



/* Finds the modbus master command indexed by cmd.  Returns a pointer
   to the command when found and NULL on error. */
struct
mb_cmd *mb_get_cmd(struct mb_port *mp,unsigned int cmd)
{
    struct mb_cmd *node;
    unsigned int n = 0;
    if(mp == NULL) return NULL;
    
    if(mp->commands == NULL) {
        node = NULL;
    } else { 
        node = mp->commands;
        do {
            if(n++ == cmd) return node;
            node = node->next;
        } while(node != NULL);
    }
    return node;
}

/* Determines whether or not the port is a serial port or an IP
   socket and opens it appropriately */
int
mb_open_port(struct mb_port *m_port)
{
    int fd;
    
    /* if the LAN bit or the TCP bit are set then we need a network
        socket.  Otherwise we need a serial port opened */
    if(m_port->devtype == MB_NETWORK) {
        fd = openIPport(m_port);
    } else {
        fd = openport(m_port);
    }
    if(pthread_mutex_init (&m_port->port_mutex, NULL)) {
        dax_error("Problem Initilizing Mutex for port: %s", m_port->name);
        return -1;
    }
    if(fd > 0) return 0;
    return fd;
}

/* Opens the port passed in m_port and starts the thread that
   will handle the port */
int
mb_start_port(struct mb_port *m_port)
{
    pthread_attr_t attr;
    
    /* If the port is not already open */
    if(!m_port->fd) {
        if(mb_open_port(m_port)) {
            dax_error( "Unable to open port - %s", m_port->name);
            return -1;
        }
    }
    if(m_port->fd > 0) {
        /* Set up the thread attributes 
         * The thread is set up to be detached so that it's resources will
         * be deallocated automatically. */
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if(m_port->type == MB_MASTER) {
            if(pthread_create(&m_port->thread, &attr, (void *)&masterRTUthread, (void *)m_port)) {
                dax_error( "Unable to start thread for port - %s", m_port->name);
                return -1;
            } else {
                dax_debug(3, "Started Thread for port - %s", m_port->name);
                return 0;
            }
        } else if(m_port->type == MB_SLAVE) {
            ; /* TODO: start slave thread */
        } else {
            dax_error( "Unknown Port Type %d, on port %s", m_port->type, m_port->name);
            return -1;
        }
        return 0;
    } else {
        dax_error( "Unable to open IP Port: %s [%s:%d]", m_port->name, m_port->ipaddress, m_port->bindport);
        return -1;
    }
    return 0; //should never get here
}


/* Open and set up the serial port */
static int
openport(struct mb_port *m_port)
{
    int fd;
    struct termios options;
    
    /* the port is opened RW and reads will not block */
    fd = open(m_port->device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd == -1)  {
        dax_error("openport: %s", strerror(errno));
        return(-1);
    } else  {
    
    /* TODO: this should set the port up like the configuration says */
        fcntl(fd, F_SETFL, 0);
        tcgetattr(fd, &options);
        cfsetispeed(&options, m_port->baudrate);
        cfsetospeed(&options, m_port->baudrate);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
      
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
        options.c_oflag &= ~OPOST;
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 0; /* 1 sec */
        /* TODO: Should check for errors here */
        tcsetattr(fd, TCSANOW, &options);
    } 
    m_port->fd = fd;
    return fd;
}

/* Opens a IP socket instead of a serial port for both
   the TCP protocol and the LAN protocol. */
static int
openIPport(struct mb_port *mp)
{
    int fd;
    struct sockaddr_in addr;
    int result;
    
    dax_debug(10, "Opening IP Port");
    if(mp->socket == TCP_SOCK) {
		fd = socket(AF_INET, SOCK_STREAM, 0);
	} else if (mp->socket == UDP_SOCK) {
		fd = socket(AF_INET, SOCK_DGRAM, 0);
	} else {
        dax_error( "Unknown socket type");
	    return -1;
	}
	
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(mp->ipaddress);
    addr.sin_port = htons(mp->bindport);
    
    result = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    dax_debug(10, "Connect returned %d", result);
    if(result == -1) {
        dax_error( "openIPport: %s", strerror(errno));
        return -1;
    }
    result = fcntl(fd, F_SETFL, O_NONBLOCK);
    if(result) {
        dax_error( "Unable to set socket to non blocking");
        return -1 ;
    }
    dax_debug(10, "Socket Connected, fd = %d", fd);
    mp->fd = fd;
    return fd;
}
        

/* This is the primary thread for a Modbus RTU master.  It calls the functions
   to send the request and recieve the responses.  It also takes care of the
   retries and the counters. */
/* TODO: There needs to be a mechanism to bail out of here.  The mp->running should
be set to 0 and the mp->fd closed.  This will cause the main while(1) loop to restart
the port */
void
masterRTUthread(struct mb_port *mp)
{
    long time_spent;
    struct mb_cmd *mc;
    struct timeval start, end;
    unsigned char bail = 0;
    
    mp->running = 1; /* Tells the world that we are going */
    mp->attempt = 0;
    /* If enable goes negative we bail at the next scan */
    while(mp->enable >= 0 && !bail) {
        gettimeofday(&start, NULL);
        if(mp->enable) { /* If enable=0 then pause for the scanrate and try again. */
            mc = mp->commands;
            while(mc != NULL && !bail) {
                /* Only if the command is enabled and the interval counter is over */
                if(mc->method && (++mc->icount >= mc->interval)) { 
                    mc->icount = 0;
                    if(mp->maxattempts) {
                        mp->attempt++;
                        DAX_DEBUG2("Incrementing attempt - %d", mp->attempt);
                    }
                    if( mb_send_command(mp, mc) > 0 )
                        mp->attempt = 0; /* Good response, reset counter */
                    
                    if((mp->maxattempts && mp->attempt >= mp->maxattempts) || mp->dienow) {
                        bail = 1;
                        mp->inhibit_temp = 0;
                        mp->inhibit = 1;
                    }
                }
                if(mp->delay > 0) usleep(mp->delay * 1000);
                mc = mc->next; /* get next command from the linked list */
            } /* End of while for sending commands */
        }
        /* This calculates the length of time that it took to send the messages on this port
           and then subtracts that time from the port's scanrate and calls usleep to hold
           for the right amount of time.  */
        gettimeofday(&end, NULL);
        time_spent = (end.tv_sec-start.tv_sec)*1000 + (end.tv_usec/1000 - start.tv_usec/1000);
        /* If it takes longer than the scanrate then just go again instead of sleeping */
        if(time_spent < mp->scanrate)
            usleep((mp->scanrate - time_spent) * 1000);
    }
     /* Close the port */
    if(close(mp->fd)) {
        dax_error("Could not close port");
    }
    dax_error("Too Many Errors: Shutting down port - %s", mp->name);
    mp->fd = 0;
    mp->dienow = 0;
    mp->running = 0;
}

/* This function formulates and sends the modbus master request */
static int
sendRTUrequest(struct mb_port *mp,struct mb_cmd *cmd)
{
    u_int8_t buff[MB_FRAME_LEN], length;
    u_int16_t crc, temp, result;
    
    /* build the request message */
    buff[0]=cmd->node;
    buff[1]=cmd->function;
    
    switch (cmd->function) {
        case 1:
        case 3:
        case 4:
            COPYWORD(&buff[2], &cmd->m_register);
            COPYWORD(&buff[4], &cmd->length);
            length = 6;
            break;
        case 5:
            //--temp = dt_getbit(cmd->handle, cmd->index);
            temp = 0;
            result = dt_getbits(cmd->idx, cmd->index, &temp, 1);
            //--printf("...getbits returned 0x%X for index %d\n", temp, cmd->index);
            /* TODO: Error Check result */
            if(cmd->method == MB_CONTINUOUS || (temp != cmd->lastcrc) || !cmd->firstrun ) {
                COPYWORD(&buff[2], &cmd->m_register);
                if(temp) buff[4] = 0xff;
                else     buff[4] = 0x00;
                buff[5] = 0x00;
                cmd->firstrun = 1;
                cmd->lastcrc = temp;
                length = 6;
                break;
            } else {
                return 0;
            }
            break;
        case 6:
            //--temp = dt_getword(cmd->address);
            result = dt_getwords(cmd->idx, cmd->index, &temp, 1);
            /* TODO: Error Check result */
            /* If the command is contiunous go, if conditional then 
             check the last checksum against the current datatable[] */
            if(cmd->method == MB_CONTINUOUS || (temp != cmd->lastcrc)) {
                COPYWORD(&buff[2], &cmd->m_register);
                COPYWORD(&buff[4], &temp);
                cmd->lastcrc = temp; /* Since it's a single just store the word */
                length = 6;
                break;
            } else {
                return 0;
            }
            default:
            break;
    }
    crc = crc16(buff,length);
    COPYWORD(&buff[length],&crc);
    /* Send Request */
    cmd->requests++; /* Increment the request counter */
    tcflush(mp->fd, TCIOFLUSH);
    /* Send the buffer to the callback routine. */
    if(mp->out_callback) {
        mp->out_callback(mp, buff, length + 2);
    }
    
    return write(mp->fd, buff, length + 2);
}

/* This function waits for one interbyte timeout (wait) period and then
 checks to see if there is data on the port.  If there is no data and
 we still have not received any data then it compares the current time
 against the time the loop was started to see about the timeout.  If there
 is data then it is written into the buffer.  If there is no data on the
 read() and we have received some data already then the full message should
 have been received and the function exits. 
 
 Returns 0 on timeout
 Returns -1 on CRC fail
 Returns the length of the message on success */
static int
getRTUresponse(u_int8_t *buff, struct mb_port *mp)
{
    unsigned int buffptr = 0;
    struct timeval oldtime, thistime;
    int result;
    
    gettimeofday(&oldtime, NULL);
    
    while(1) {
        usleep(mp->wait * 1000);
        result = read(mp->fd, &buff[buffptr], MB_FRAME_LEN);
        if(result > 0) { /* Get some data */
            buffptr += result; // TODO: WE NEED A BOUNDS CHECK HERE.  Seg Fault Commin'
        } else { /* Message is finished, good or bad */
            if(buffptr > 0) { /* Is there any data in buffer? */
                
                if(mp->in_callback) {
                    mp->in_callback(mp, buff, buffptr);
                }
                /* Check the checksum here. */
                result = crc16check(buff, buffptr);
                
                if(!result) return -1;
                else return buffptr;
                
            } else { /* No data in the buffer */
                gettimeofday(&thistime,NULL); 
                if(timediff(oldtime, thistime) > mp->timeout) {
                    return 0;
                }
            }
        }
    }
    return 0; /* Should never get here */
}

static int
sendASCIIrequest(struct mb_port *mp,struct mb_cmd *cmd)
{
    return 0;
}

static int
getASCIIresponse(u_int8_t *buff,struct mb_port *mp)
{
    return 0;
}

/* This function takes the message buffer and the current command and
 determines what to do with the message.  It may write data to the 
 datatable or just return if the message is an acknowledge of a write.
 This function is protocol indifferent so the checksums should be
 checked before getting here.  This function assumes that *buff looks
 like an RTU message so the ASCII response functions should translate
 the ASCII responses into RTUish messages */

static int
handleresponse(u_int8_t *buff, struct mb_cmd *cmd)
{
    int n, result;
    u_int16_t *data;
    
    DAX_DEBUG2("handleresponse() - Function Code = %d", cmd->function);
    cmd->responses++;
    if(buff[1] >= 0x80) 
        return buff[2];
    if(buff[0] != cmd->node)
        return ME_WRONG_DEVICE;
    if(buff[1] != cmd->function)
        return ME_WRONG_FUNCTION;
    /* If we get this far the message should be good */
    switch (cmd->function) {
        case 1:
            //--dt_setbits(cmd->address, &buff[3], cmd->length);
            result = dt_setbits(cmd->idx, cmd->index, &buff[3], cmd->length);
            break;
        case 3:
        case 4:
            data = malloc(buff[2] / 2);
            if(data == NULL) return ERR_ALLOC;
            for(n = 0; n < (buff[2] / 2); n++) {
                COPYWORD(&data[n], &buff[(n * 2) + 3]);
            }
            //--if(dt_setword(cmd->address + n, temp)) return -1;
            /* TODO: There may be times when we get more data than cmd->length but how to deal with that? */
            result = dt_setwords(cmd->idx, cmd->index, data, cmd->length);
            free(data);
            break;
        case 5:
        case 6:
            //COPYWORD(&temp, &buff[2]);
            //if(dt_setword(cmd->address,temp)) return -1;
            break;
        default:
            break;
    }
    return 0;
}


/* External function to send a Modbus commaond (mc) to port (mp).  The function
 * sets some function pointers to the functions that handle the port protocol and
 * then uses those functions generically.  The retry loop tries the command for the
 * configured number of times and if successful returns 0.  If not, an error code
 * is returned. mb_buff should be at least MB_FRAME_LEN in length */

int
mb_send_command(struct mb_port *mp, struct mb_cmd *mc)
{
    u_int8_t buff[MB_FRAME_LEN]; /* Modbus Frame buffer */
    int try = 1;
    int result, msglen;
    static int (*sendrequest)(struct mb_port *, struct mb_cmd *) = NULL;
    static int (*getresponse)(u_int8_t *,struct mb_port *) = NULL;
    
    
    /*This sets up the function pointers so we don't have to constantly check
      which protocol we are using for communication.  From this point on the 
      code is generic for RTU or ASCII */
    if(mp->protocol == MB_RTU) {
        sendrequest = sendRTUrequest;
        getresponse = getRTUresponse;
    } else if(mp->protocol == MB_ASCII) {
        sendrequest = sendASCIIrequest;
        getresponse = getASCIIresponse;
    } else {
        return -1;
    }
    
    do { /* retry loop */
        pthread_mutex_lock(&mp->port_mutex); /* Lock the port */
                
		result = sendrequest(mp, mc);
		if(result > 0) {
			msglen = getresponse(buff, mp);
		    pthread_mutex_unlock(&mp->port_mutex);
        } else if(result == 0) {
            /* Should be 0 when a conditional command simply doesn't run */
            pthread_mutex_unlock(&mp->port_mutex);
            return result;
        } else {
            pthread_mutex_unlock(&mp->port_mutex);
            return -1;
        }
        
        if(msglen > 0) {
            result = handleresponse(buff,mc);
            if(result > 0) {
                mc->exceptions++;
                mc->lasterror = result | ME_EXCEPTION;
                DAX_DEBUG2("Exception Received - %d", result);
            } else { /* Everything is good */
                mc->lasterror = 0;
            }
            return 0; /* We got some kind of message so no sense in retrying */
        } else if(msglen == 0) {
            DAX_DEBUG("Timeout");
			mc->timeouts++;
			mc->lasterror = ME_TIMEOUT;
        } else {
            /* Checksum failed in response */
            DAX_DEBUG("Checksum");
			mc->crcerrors++;
            mc->lasterror = ME_CHECKSUM;
        }
    } while(try++ <= mp->retries);
    /* After all the retries get out with error */
    /* TODO: Should set error code?? */
    return -2;
}


