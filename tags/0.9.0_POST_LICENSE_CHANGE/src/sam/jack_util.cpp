/**
 * @file jack_util.cpp
 * Shared JACK-related functionality
 * @author Michelle Daniels
 * @date November 2011
 * @license 
 * This software is Copyright 2011-2014 The Regents of the University of California. 
 * All Rights Reserved.
 *
 * Permission to copy, modify, and distribute this software and its documentation for 
 * educational, research and non-profit purposes by non-profit entities, without fee, 
 * and without a written agreement is hereby granted, provided that the above copyright
 * notice, this paragraph and the following three paragraphs appear in all copies.
 *
 * Permission to make commercial use of this software may be obtained by contacting:
 * Technology Transfer Office
 * 9500 Gilman Drive, Mail Code 0910
 * University of California
 * La Jolla, CA 92093-0910
 * (858) 534-5815
 * invent@ucsd.edu
 *
 * This software program and documentation are copyrighted by The Regents of the 
 * University of California. The software program and documentation are supplied 
 * "as is", without any accompanying services from The Regents. The Regents does 
 * not warrant that the operation of the program will be uninterrupted or error-free. 
 * The end-user understands that the program was developed for research purposes and 
 * is advised not to rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 * OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. THE UNIVERSITY OF
 * CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, 
 * AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 */

#include <errno.h>
#include <signal.h> // needed for kill() on OS X but not in OpenSUSE for some reason
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "jack_util.h"

namespace sam
{

static const int MAX_PORT_NAME = 32;
static const int MAX_CMD_LEN = 64;

bool JackServerIsRunning()
{
    printf("\nTesting if JACK server is running.\n*Please ignore any JACK failure messages printed to the console.*\n");
    // try to open a client connection to the JACK server
    // explicitly tell JACK to not start a server if one wasn't already running (JackNoStartServer)
    jack_status_t status;
    jack_client_t* client = jack_client_open("test", JackNoStartServer, &status);
    printf("Finished testing if JACK server is running.\n*You can pay attention to JACK console messages again.*\n\n");
    if (client == NULL)
    {
        // no client, so server must not be running (possible there was some other kind of error)
        return false;
    }

    // successfully opened the test client, so there was a server running already
    jack_client_close(client);
    return true;
}

pid_t StartJack(int sampleRate, int bufferSize, int outChannels, const char* driver)
{
    // start jackd
    pid_t jackPID = fork();
    if (jackPID == -1)
    {
        perror("StartJack() Couldn't fork to start jackd");
        return false;
    }
    else if (jackPID == 0)
    {
#if defined __APPLE__
        const char* jackCmd = "/usr/local/bin/jackd";
#else
        const char* jackCmd = "jackd";
#endif
        char cmdDriver[MAX_CMD_LEN];
        snprintf(cmdDriver, MAX_CMD_LEN, "-d%s", driver);

        char cmdFs[MAX_CMD_LEN];
        snprintf(cmdFs, MAX_CMD_LEN, "-r%d", sampleRate);

        char cmdBuf[MAX_CMD_LEN];
        snprintf(cmdBuf, MAX_CMD_LEN, "-p%d", bufferSize);
        
        char cmdOut[MAX_CMD_LEN];
        snprintf(cmdOut, MAX_CMD_LEN, "-o%d", outChannels);

        // note: all of these (char*) casts were needed to avoid compiler warnings because
        // string literals are char[]'s and not char*'s  (weird!)
        char* cmd[] = {(char*)"jackd", cmdDriver, cmdFs, cmdBuf, cmdOut, (char*)0};
        int status = execvp(jackCmd, cmd);
        if (status < 0)
        {
            perror("StartJack() execvp failed");
        }
    }
    else
    {
        //printf("SAMTest::start_jack() Successfully forked to start jackd with pid %d\n", m_jackPID);
        //printf("SAMTest::start_jack() Sleeping to give jack time to actually start...\n");
        sleep(2);
        //printf("SAMTest::start_jack() ...done sleeping.\n");
    }
    
    return jackPID;
}

bool StopJack(pid_t jackPID)
{
    if (jackPID > 0)
    {
        // try to kill the jackd process
        int result = kill(jackPID, SIGQUIT);
        if (result == 0)
        {
            //printf("Successfully called kill on jackd process\n");
            //printf("Sleeping to give jackd a chance to properly exit...\n");
            sleep(1);
            //printf("...done sleeping.\n");
        }
        else
        {
            perror("StopJack() Could not kill jackd process\n");
            return false;
        }
        jackPID = 0;
    }
    return true;
}

bool PortIsInput(const jack_port_t* port)
{
    int portFlags = jack_port_flags(port);
    return ((portFlags & JackPortIsInput) == JackPortIsInput);
}

bool PortIsOutput(const jack_port_t* port)
{
    int portFlags = jack_port_flags(port);
    return ((portFlags & JackPortIsOutput) == JackPortIsOutput);
}

} // end of namespace SAM
