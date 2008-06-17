/* @(#) $Id$ */

/* Copyright (C) 2006, 2008 Third Brigade, Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 3) as published by the FSF - Free Software
 * Foundation
 */


#include "shared.h"
#include "agentd.h"

#include "os_net/os_net.h"


/** void connect_server()
 *  Attempts to connect to all configured servers.
 */
int connect_server(int initial_id)
{
    int attempts = 2;
    int rc = initial_id;


    /* Checking if the initial is zero, meaning we have to rotate to the
     * beginning.
     */
    if(logr->rip[initial_id] == NULL)
    {
        rc = 0;
        initial_id = 0;
    }


    /* Closing socket if available. */
    if(logr->sock >= 0)
    {
        /* Waiting for other threads to settle. */
        sleep(2);
        close(logr->sock);
        logr->sock = -1;
    }
    
    
    while(logr->rip[rc])
    {
        verbose("%s: INFO: Connecting to server (%s:%d).", ARGV0,
                logr->rip[rc],
                logr->port);

        logr->sock = OS_ConnectUDP(logr->port, logr->rip[rc]);
        if(logr->sock < 0)
        {
            merror(CONNS_ERROR, ARGV0, logr->rip[rc]);
            rc++;

            if(logr->rip[rc] == NULL)
            {
                attempts += 10;
                
                /* Only log that if we have more than 1 server configured. */
                if(logr->rip[1])
                    merror("%s: ERROR: Unable to connect to any server.", ARGV0);
                    
                sleep(attempts);
                rc = 0;
            }
        }
        else
        {
            /* Setting socket non-blocking on HPUX */
            #ifdef HPUX
            fcntl(logr->sock, O_NONBLOCK);
            #endif

            #ifdef WIN32
            int bmode = 1;
            
            /* Setting socket to non-blocking */
            ioctlsocket(logr->sock, FIONBIO, (u_long FAR*) &bmode);
            #endif

            logr->rip_id = rc;
            return(1);
        }
    }

    return(0);
}



/* start_agent: Sends the synchronization message to
 * the server and waits for the ack.
 */
void start_agent(int is_startup)
{
    int recv_b = 0, attempts = 0, g_attempts = 1;

    char *tmp_msg;
    char msg[OS_MAXSTR +2];
    char buffer[OS_MAXSTR +1];
    char cleartext[OS_MAXSTR +1];
    char fmsg[OS_MAXSTR +1];
    

    memset(msg, '\0', OS_MAXSTR +2);
    memset(buffer, '\0', OS_MAXSTR +1);
    memset(cleartext, '\0', OS_MAXSTR +1);
    memset(fmsg, '\0', OS_MAXSTR +1);
    snprintf(msg, OS_MAXSTR, "%s%s", CONTROL_HEADER, HC_STARTUP);
    
    
    /* Sending start message and waiting for the ack */	
    while(1)
    {
        /* Sending start up message */
        send_msg(0, msg);
        attempts = 0;
        

        /* Read until our reply comes back */
        while(((recv_b = recv(logr->sock, buffer, OS_MAXSTR,
                              MSG_DONTWAIT)) >= 0) || (attempts <= 5))
        {
            if(recv_b <= 0)
            {
                /* Sleep five seconds before trying to get the reply from
                 * the server again.
                 */
                attempts++;
                sleep(attempts);

                /* Sending message again (after three attempts) */
                if(attempts >= 3)
                {
                    send_msg(0, msg);
                }
                
                continue;
            }
            
            /* Id of zero -- only one key allowed */
            tmp_msg = ReadSecMSG(&keys, buffer, cleartext, 0, recv_b -1);
            if(tmp_msg == NULL)
            {
                merror(MSG_ERROR, ARGV0, logr->rip[logr->rip_id]);
                continue;
            }


            /* Check for commands */
            if(IsValidHeader(tmp_msg))
            {
                /* If it is an ack reply */
                if(strcmp(tmp_msg, HC_ACK) == 0)
                {
                    available_server = time(0);
                    if(is_startup)
                    {
                        verbose(AG_CONNECTED, ARGV0);

                        /* Send log message about start up */
                        snprintf(msg, OS_MAXSTR, OS_AG_STARTED, 
                                keys.keyentries[0]->name,
                                keys.keyentries[0]->ip->ip);
                        snprintf(fmsg, OS_MAXSTR, "%c:%s:%s", LOCALFILE_MQ, 
                                                  "ossec", msg);
                        send_msg(0, fmsg);
                    }
                    return;
                }
            }
        }

        /* Waiting for servers reply */
        merror(AG_WAIT_SERVER, ARGV0, logr->rip[logr->rip_id]);


        /* If we have more than one server, try all. */
        if(logr->rip[1])
        {
            int curr_rip = logr->rip_id;
            merror("%s: INFO: Trying next server ip in the line: '%s'.", ARGV0,
                   logr->rip[logr->rip_id + 1] != NULL?logr->rip[logr->rip_id + 1]:
                                                       logr->rip[0]);
            connect_server(logr->rip_id +1);

            if(logr->rip_id == curr_rip)
            {
                sleep(g_attempts);
                g_attempts+=(attempts * 3);
            }
            else
            {
                g_attempts = 1;
            }
        }
    }
    
    return;
}



/* EOF */
