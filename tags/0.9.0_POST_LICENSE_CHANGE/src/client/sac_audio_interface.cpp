/**
 * @file sac_audio_interface.cpp
 * Implementation of different audio interfaces for SacAudioInterfaces
 * @author Michelle Daniels
 * @date September 2012
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

#include <unistd.h>

#include <QCoreApplication>
#include <QDebug>

#include "sac_audio_interface.h"

namespace sam
{

static const int MAX_PORT_NAME = 64;
static const int MAX_CMD_LEN = 64;

static const int SLEEP_BUFFER_MILLIS = 2;
static const int SHORT_SLEEP_MICROS = 500;


// ------------------- SacAudioInterface implementation -------------------

SacAudioInterface::SacAudioInterface(unsigned int channels, unsigned int bufferSamples, unsigned int sampleRate) :
        m_channels(channels),
        m_bufferSamples(bufferSamples),
        m_sampleRate(sampleRate),
        m_audioCallback(NULL),
        m_audioCallbackArg(NULL),
        m_audioIn(NULL),
        m_audioOut(NULL)
{
    // allocate audio buffers
    m_audioIn = new float*[m_channels];
    m_audioOut = new float*[m_channels];
    for (unsigned int ch = 0; ch < m_channels; ch++)
    {
        m_audioIn[ch] = new float[m_bufferSamples];
        m_audioOut[ch] = new float[m_bufferSamples];
    }
}

SacAudioInterface::~SacAudioInterface()
{
    if (m_audioIn)
    {
        for (unsigned int ch = 0; ch < m_channels; ch++)
        {
            if (m_audioIn[ch])
            {
                delete[] m_audioIn[ch];
                m_audioIn[ch] = NULL;
            }
        }
        delete[] m_audioIn;
        m_audioIn = NULL;
    }
    
    if (m_audioOut)
    {
        for (unsigned int ch = 0; ch < m_channels; ch++)
        {
            if (m_audioOut[ch])
            {
                delete[] m_audioOut[ch];
                m_audioOut[ch] = NULL;
            }
        }
        delete[] m_audioOut;
        m_audioOut = NULL;
    }
}

bool SacAudioInterface::setAudioCallback(AudioInterfaceCallback callback, void* arg)
{ 
    m_audioCallback = callback; 
    m_audioCallbackArg = arg; 
    
    return true;
}

// ------------------- VirtualAudioInterface implementation -------------------

VirtualAudioInterface::VirtualAudioInterface(unsigned int channels, unsigned int bufferSamples, unsigned int sampleRate) :
    SacAudioInterface(channels, bufferSamples, sampleRate),
    m_audioInterval(0.0),
    m_nextAudioTick(0),
    m_shouldQuit(false)
{
    m_audioInterval = m_bufferSamples * 1000.0 / (double)m_sampleRate;
    qDebug("VirtualAudioInterface::VirtualAudioInterface audio interval = %f", m_audioInterval);
}

VirtualAudioInterface::~VirtualAudioInterface()
{
    stop();
}

void VirtualAudioInterface::run()
{
    m_timer.start();
    m_shouldQuit = false;
    while (!m_shouldQuit)
    {
        // call registered callback with NULL input data
        if (!m_audioCallback(m_channels, m_bufferSamples, NULL, NULL, m_audioCallbackArg))
        {
            qWarning("VirtualAudioInterface::run() error occurred calling registered audio callback");
        }
        
        m_nextAudioTick += m_bufferSamples; // TODO: could this ever wrap around??

        // sleep for almost the full tick time
        if (m_audioInterval > SLEEP_BUFFER_MILLIS) msleep(m_audioInterval - SLEEP_BUFFER_MILLIS);

        // sleep in shorter intervals until tick time
        while (true)
        {
            qint64 elapsedMillis = m_timer.elapsed();
            quint64 elapsedSamples = (elapsedMillis * m_sampleRate) / 1000;
            if (elapsedSamples >= m_nextAudioTick) break;
            usleep(SHORT_SLEEP_MICROS);
        }
    }

    qDebug("VirtualAudioInterface::run finished");
}

bool VirtualAudioInterface::go()
{
    start();
    return true;
}

bool VirtualAudioInterface::stop()
{
    if (m_shouldQuit) return true; // already stopped?

    m_shouldQuit = true;
    qDebug("VirtualAudioInterface::stop requested stop");
    wait();
    qDebug("VirtualAudioInterface::stop thread finished executing");
    
    return true;
}

#ifndef SAC_NO_JACK
// ------------------- JackAudioInterface implementation -------------------

JackAudioInterface::JackAudioInterface(unsigned int channels, unsigned int bufferSamples, unsigned int sampleRate, const char* clientName) :
        SacAudioInterface(channels, bufferSamples, sampleRate),
        m_client(NULL),
        m_inputPorts(NULL)
{
    int len = strlen(clientName) + 1;
    m_clientName = new char[len];
    strncpy(m_clientName, clientName, len); 
}

JackAudioInterface::~JackAudioInterface()
{
   
    stop();
    if (m_clientName)
    {
        delete[] m_clientName;
        m_clientName = NULL;
    }
}

bool JackAudioInterface::go()
{
    // start JACK
    if (jack_server_is_running())
    {
        qDebug() << "JackAudioInterface::start Warning: JACK was already running" << endl;
    }
    else
    {
        qDebug() << "JackAudioInterface::start Starting JACK:" << endl;
        pid_t jackPID = start_jack(m_sampleRate, m_bufferSamples);
        qDebug() << "JackAudioInterface::start Finished starting JACK" << endl;
        if (jackPID >= 0)
        {
            qDebug() << "JackAudioInterface::start: Successfully started the JACK server with PID " << QString::number(jackPID) << endl;
        }
        else
        {
            qWarning() << "JackAudioInterface::start: Couldn't start JACK server" << endl;
            return false;
        }
    }
    
    // create a jack client
    if (!open_jack_client())
    {
        qWarning() << "JackAudioInterface::start Couldn't open JACK client" << endl;
        return false;
    }

    qDebug() << "JackAudioInterface::start opened JACK client" << endl;

    // check that buffer size and sample rate are correct
    if (jack_get_buffer_size(m_client) != m_bufferSamples || jack_get_sample_rate(m_client) != m_sampleRate)
    {
        qWarning() << "JackAudioInterface::start ERROR: JACK is running with incorrect parameters: sample rate = " << jack_get_sample_rate(m_client) << ", buffer size = " << jack_get_buffer_size(m_client) << endl;
        // TODO: how to communicate this error back to the app?
        return false;
    }
    
    qDebug() << "JackAudioInterface::start checked JACK parameters" << endl;

    // register jack callbacks
    jack_set_buffer_size_callback(m_client, JackAudioInterface::jack_buffer_size_changed, this);
    jack_set_process_callback(m_client, JackAudioInterface::jack_process, this);
    jack_set_sample_rate_callback(m_client, JackAudioInterface::jack_sample_rate_changed, this);
    jack_on_shutdown(m_client, JackAudioInterface::jack_shutdown, this);
    jack_set_xrun_callback(m_client, JackAudioInterface::jack_xrun, this);

    qDebug() << "JackAudioInterface::start registered JACK callbacks" << endl;

     // activate client (starts processing)
    if (jack_activate(m_client) != 0)
    {
        qDebug() << "JackAudioInterface::start Couldn't activate JACK client" << endl;
        return false;
    }

    qDebug() << "JackAudioInterface::start activated JACK client" << endl;
    return true;
}

bool JackAudioInterface::stop()
{
    if (m_client)
    {
        int result = jack_deactivate(m_client);
        if (result != 0)
        {
            qDebug("JackAudioInterface::~JackAudioInterface() couldn't deactivate jack client");
        }
    }
    
     // free input ports
    if (m_inputPorts)
    {
        for (unsigned int ch = 0; ch < m_channels; ch++)
        {
            if (m_inputPorts[ch])
            {
                jack_port_unregister(m_client, m_inputPorts[ch]);
                m_inputPorts[ch] = NULL;
            }
        }
        delete[] m_inputPorts;
        m_inputPorts = NULL;
    }
    
    if (!close_jack_client())
    {
        qDebug() << "JackAudioInterface::~JackAudioInterface() couldn't close JACK client" << endl;
    }
    
    return true;
}

bool JackAudioInterface::setPhysicalInputs(unsigned int* inputChannels)
{
    // get all jack ports that correspond to physical inputs
    if (!m_client) return false;
    const char** inputPorts = jack_get_ports(m_client, NULL, NULL, JackPortIsOutput | JackPortIsPhysical);

    // count the number of physical input ports
    const char** currentPort = inputPorts;
    unsigned int numPhysicalPortsIn = 0;
    while (*currentPort != NULL)
    {
        numPhysicalPortsIn++;
        qDebug("JackAudioInterface::SetPhysicalInputs() counted port %s", *currentPort);
        currentPort++;
    }
    qDebug("JackAudioInterface::SetPhysicalInputs() counted %d physical inputs", numPhysicalPortsIn);
    jack_free(inputPorts);
    
    if (m_channels > numPhysicalPortsIn)
    {
        return false; // requested more input ports than actually exist
    }
    
    // allocate array of input port pointers
    m_inputPorts = new jack_port_t*[m_channels];
    for (unsigned int ch = 0; ch < m_channels; ch++)
    {
        m_inputPorts[ch] = NULL;
    }
    
    // register JACK input ports and connect physical inputs to client inputs
    char portName[MAX_PORT_NAME];
    for (unsigned int i = 0; i < m_channels; i++)
    {
        // register input port
        snprintf(portName, MAX_PORT_NAME, "input_%d", i + 1);
        m_inputPorts[i] = jack_port_register(m_client, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        if (!m_inputPorts[i])
        {
            qDebug() << "JackAudioInterface::SetPhysicalInputs ERROR: couldn't register input port for channel " << i + 1 << endl;
            return false;
        }
        qDebug() << "JackAudioInterface::SetPhysicalInputs registered input port " << i << endl;
        
        // connect the specified physical input to the newly-created input port
        char systemIn[MAX_PORT_NAME];
        snprintf(systemIn, MAX_PORT_NAME, "system:capture_%d", inputChannels[i]);
        // TODO: handle case where system ports have non-standard names (is this possible?)?
        const char* inputPortName = jack_port_name(m_inputPorts[i]); // TODO: handle case where input port requested doesn't exist?
        int result = jack_connect(m_client, systemIn, inputPortName);
        if (result == EEXIST)
        {
            qWarning("JackAudioInterface::SetPhysicalInputs WARNING: %s and %s were already connected", systemIn, inputPortName);
            // TODO: decide how to handle this case
        }
        else if (result != 0)
        {
            qWarning("JackAudioInterface::SetPhysicalInputs ERROR: couldn't connect %s to %s", systemIn, inputPortName);

            return false;
        }
        else
        {
            qDebug("JackAudioInterface::SetPhysicalInputs connected %s to %s", systemIn, inputPortName);
        }
    }

    qDebug() << "JackAudioInterface::SetPhysicalInputs registered input ports" << endl;
    
    return true;
}

int JackAudioInterface::process_audio(jack_nframes_t nframes)
{ 
    // TODO: need error-checking to make sure nframes is not > buffer size?
    
    
    if (m_inputPorts)
    {
        // grab input audio if physical inputs enabled
        for (unsigned int ch = 0; ch < m_channels; ch++)
        {
            jack_port_t* inPort = m_inputPorts[ch];
            if (inPort)
            {
                jack_default_audio_sample_t* in = (jack_default_audio_sample_t*) jack_port_get_buffer(inPort, nframes);
                for (unsigned int n = 0; n < nframes; n++)
                {
                    m_audioIn[ch][n] = in[n];
                }
            }
            else
            {
                qDebug("JackAudioInterface::process_audio input buffer was null!");
                // use zeros if an error occurred
                // TODO: handle this error differently??
                for (unsigned int ch = 0; ch < m_channels; ch++)
                {
                    memset(m_audioIn[ch], 0, nframes * sizeof(float));
                }
            }   
        }
        
        // call registered callback with input data
        if (!m_audioCallback(m_channels, nframes, m_audioIn, NULL, m_audioCallbackArg))
        {
            qDebug("JackAudioInterface::process_audio error occurred calling registered audio callback with input data");
        }
    }
    else
    {
        // call registered callback with NULL input data
        if (!m_audioCallback(m_channels, nframes, NULL, NULL, m_audioCallbackArg))
        {
            qDebug("JackAudioInterface::process_audio error occurred calling registered audio callback");
        }
    }
    
    return 0;
}

bool JackAudioInterface::open_jack_client()
{
    // open a client connection to the JACK server
    jack_status_t status;
    m_client = jack_client_open(m_clientName, JackUseExactName, &status);
    if (m_client == NULL)
    {
        qWarning("JackAudioInterface::open_jack_client(): jack_client_open() failed, status = 0x%2.0x", status);
        if (status & JackServerFailed)
        {
            qWarning("JackAudioInterface::open_jack_client(): Unable to connect to JACK server");
        }
        return false;
    }
    if (status & JackServerStarted)
    {
        qDebug("JackAudioInterface::open_jack_client(): Started new JACK server");
    }
    if (status & JackNameNotUnique)
    {
        qWarning("JackAudioInterface::open_jack_client(): unique client name `%s' assigned", jack_get_client_name(m_client));
        // TODO: this should actually be an error: since our client name includes our port #, we should already be unique
    }
    
    return true;
}

bool JackAudioInterface::close_jack_client()
{
    if (m_client != NULL)
    {
        // close this client
        int result = jack_client_close(m_client);
        if (result == 0)
        {
            m_client = NULL;
        }
        else
        {
            qDebug("JackAudioInterface::close_jack_client() couldn't close jack client");
            return false;
        }
    }
    return true;
}

bool JackAudioInterface::jack_server_is_running()
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

pid_t JackAudioInterface::start_jack(int sampleRate, int bufferSize)
{
    // start jackd
    pid_t jackPID = fork();
    if (jackPID == -1)
    {
        perror("JackAudioInterface::start_jack() Couldn't fork to start jackd");
        return false;
    }
    else if (jackPID == 0)
    {
#if defined __APPLE__
        const char* jackCmd = "/usr/local/bin/jackd";
        const char* driver = "coreaudio";
#else
        const char* jackCmd = "jackd";
        const char* driver = "dummy";
#endif
        char cmdDriver[MAX_CMD_LEN];
        snprintf(cmdDriver, MAX_CMD_LEN, "-d%s", driver);

        char cmdFs[MAX_CMD_LEN];
        snprintf(cmdFs, MAX_CMD_LEN, "-r%d", sampleRate);

        char cmdBuf[MAX_CMD_LEN];
        snprintf(cmdBuf, MAX_CMD_LEN, "-p%d", bufferSize);

        // note: all of these (char*) casts were needed to avoid compiler warnings because
        // string literals are char[]'s and not char*'s  (weird!)
        char* cmd[] = {(char*)"jackd", cmdDriver, cmdFs, cmdBuf, (char*)0};
        int status = execvp(jackCmd, cmd);
        if (status < 0)
        {
            perror("JackAudioInterface::start_jack() execvp failed");
        }
    }
    else
    {
        //printf("JackAudioInterface::start_jack() Successfully forked to start jackd with pid %d\n", m_jackPID);
        //printf("JackAudioInterface::start_jack() Sleeping to give jack time to actually start...\n");
        sleep(2);
        //printf("JackAudioInterface::start_jack() ...done sleeping.\n");
    }
    
    return jackPID;
}

// ----- JACK CALLBACKS -----
int JackAudioInterface::jack_buffer_size_changed(jack_nframes_t nframes, void*)
{
    qWarning() << "JackAudioInterface::jack_buffer_size_changed() WARNING: JACK buffer size changed to " << nframes << "/sec" << endl;
    // TODO: reallocate m_audioOut buffers?
    return 0;
}

int JackAudioInterface::jack_process(jack_nframes_t nframes, void* jackAI)
{
    ((JackAudioInterface*)jackAI)->process_audio(nframes);
    return 0;
}

int JackAudioInterface::jack_sample_rate_changed(jack_nframes_t nframes, void*)
{
    qWarning() << "JackAudioInterface::jack_sample_rate_changed() WARNING: JACK sample rate changed to " << nframes << "/sec" << endl;
    return 0;
}

void JackAudioInterface::jack_shutdown(void*)
{
    qWarning() << "JackAudioInterface::jack_shutdown() JACK server shut down..." << endl;
    // TODO: pass this error along to parent app?
}

int JackAudioInterface::jack_xrun(void*)
{
    qWarning() << "JackAudioInterface::jack_xrun()" << endl;
    // TODO: pass this error along to parent app?
    return 0;
}

#endif // SAC_NO_JACK

} // end of namespace sam
