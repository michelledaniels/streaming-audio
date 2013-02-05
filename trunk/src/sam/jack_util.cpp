/**
 * jack_util.cpp
 * Shared JACK-related functionality
 * Michelle Daniels  November 2011
 * Copyright UCSD 2011
 */

#include <errno.h>
#include <signal.h> // needed for kill() on OS X but not in OpenSUSE for some reason
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "jack_util.h"

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

