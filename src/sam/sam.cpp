/**
 * @file sam.cpp
 * Implementation of Streaming Audio Manager (SAM) functionality
 * @author Michelle Daniels
 * @date September 2011
 * @copyright UCSD 2011-2013
 */

#include <errno.h>
#include <signal.h> // needed for kill() on OS X but not in OpenSUSE for some reason
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <QDebug>

#include "sam.h"
#include "sam_app.h"
#include "jack_util.h"
#include "osc.h"

static const int MAX_PORT_NAME = 32;
static const int MAX_CMD_LEN = 64;

static const int OUTPUT_ENABLED_BASIC = -1;
static const int OUTPUT_ENABLED_DISCRETE = -2;
static const int OUTPUT_DISABLED = -3;

using sam::StreamingAudioType;

StreamingAudioManager::StreamingAudioManager(const SamParams& params) :
    QObject(),
    m_sampleRate(params.sampleRate),
    m_bufferSize(params.bufferSize),
    m_numBasicChannels(params.numBasicChannels),
    m_maxOutputChannels(params.maxOutputChannels),
    m_jackDriver(NULL),
    m_jackPID(-1),
    m_client(NULL),
    m_maxClients(params.maxClients),
    m_apps(NULL),
    m_appState(NULL),
    m_isRunning(false),
    m_numPhysicalPortsOut(0),
    m_outputUsed(NULL),
    m_rtpPort(params.rtpPort),
    m_outputPortOffset(params.outputPortOffset),
    m_outputJackClientName(NULL),
    m_outputJackPortBase(NULL),
    m_packetQueueSize(params.packetQueueSize),
    m_renderer(NULL),
    m_meterInterval(0),
    m_nextMeterNotify(0),
    m_samplesElapsed(0),
    m_volumeCurrent(params.volume),
    m_volumeNext(params.volume),
    m_muteCurrent(false),
    m_muteNext(false),
    m_soloCurrent(false),
    m_delayCurrent(0),
    m_delayNext(0),
    m_delayMax(0),
    m_oscServerPort(params.oscPort),
    m_udpSocket(NULL),
    m_tcpServer(NULL),
    m_samThread(NULL)
{
    m_apps = new StreamingAudioApp*[m_maxClients];
    m_appState = new SamAppState[m_maxClients];
    for (int i = 0; i < m_maxClients; i++)
    {
        m_apps[i] = NULL;
        m_appState[i] = AVAILABLE;
    }
    
    int len = strlen(params.jackDriver);
    m_jackDriver = new char[len + 1];
    strncpy(m_jackDriver, params.jackDriver, len + 1);

    len = strlen(params.outputJackClientName);
    m_outputJackClientName = new char[len + 1];
    strncpy(m_outputJackClientName, params.outputJackClientName, len + 1);

    len = strlen(params.outputJackPortBase);
    m_outputJackPortBase = new char[len + 1];
    strncpy(m_outputJackPortBase, params.outputJackPortBase, len + 1);

    m_delayMax = int(m_sampleRate * (params.maxDelayMillis / 2000.0f)); // allocate half of max available delay for apps, half for global delay
    setDelay(params.delayMillis);

    m_meterInterval = int(m_sampleRate * (params.meterIntervalMillis / 1000.0f));

    connect(this, SIGNAL(meterTick()), this, SLOT(notifyMeter()));

    if (params.renderHost && strlen(params.renderHost) > 0 && params.renderPort > 0)
    {
        // initialize renderer
        OscAddress* address = new OscAddress;
        address->host.setAddress(params.renderHost);
        address->port = params.renderPort;
        m_renderer = address;
        qWarning("Auto-registered renderer at host %s, port %u", params.renderHost, params.renderPort);
    }

    m_basicChannels.append(params.basicChannels);
    m_discreteChannels.append(params.discreteChannels);

    m_samThread = new QThread();
    moveToThread(m_samThread);
    connect(m_samThread, SIGNAL(started()), this, SLOT(run()));
}

StreamingAudioManager::~StreamingAudioManager()
{
    qDebug("StreamingAudioManager destructor called");

    stop();
    
    if (m_outputUsed)
    {
        delete[] m_outputUsed;
        m_outputUsed = NULL;
    }
    
    if (m_jackDriver)
    {
        delete[] m_jackDriver;
        m_jackDriver = NULL;
    }

    if (m_apps)
    {
        delete[] m_apps;
        m_apps = NULL;
    }

    if (m_appState)
    {
        delete[] m_appState;
        m_appState = NULL;
    }
    m_samThread->deleteLater();
}

void StreamingAudioManager::start()
{
    m_samThread->start();
}

void StreamingAudioManager::run()
{
    if (m_isRunning)
    {
        qWarning("StreamingAudioManager::run() SAM is already running");
        return;
    }

    m_udpSocket = new QUdpSocket(this);
    m_tcpServer = new QTcpServer(this);
    connect(m_udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(handleTcpConnection()));

    // bind OSC sockets
    if (!m_tcpServer->listen(QHostAddress::Any, m_oscServerPort))
    {
        qWarning("StreamingAudioManager::run() TCP server couldn't listen on port %d", m_oscServerPort);
        emit startupError();
        return;
    }
    qDebug("StreamingAudioManager::run() TCP server listening on port %d for OSC messages", m_oscServerPort);
    
    if (!m_udpSocket->bind(m_oscServerPort))
    {
        qWarning("StreamingAudioManager::run() UDP socket couldn't bind to port %d: %s", m_oscServerPort, m_udpSocket->errorString().toAscii().data());
        emit startupError();
        return;
    }
    qDebug("StreamingAudioManager::run() UDP socket binded successfully to port %d.  Now listening for OSC messages.", m_oscServerPort);
    
    // TODO: verify valid sample rate and buffer size
    
    // check for an already-running JACK server
    if (JackServerIsRunning())
    {
        qWarning("*An instance of the JACK server (jackd or jackdmp) is already running.  SAM will try to use it.");
    }

    else
    {
        // start jackd
        if (!start_jack(m_sampleRate, m_bufferSize, m_outputPortOffset + m_maxOutputChannels, m_jackDriver))
        {
            qWarning("Couldn't start JACK server process");
            emit startupError();
            return;
        }
    }

    // open jack client
    if (!open_jack_client())
    {
        emit startupError();
        return;
    }

    // verify that JACK is running at correct sample rate and buffer size
    jack_nframes_t bufferSize = jack_get_buffer_size(m_client);
    jack_nframes_t sampleRate = jack_get_sample_rate(m_client);
    if (bufferSize != (unsigned int)m_bufferSize)
    {
        qWarning("Expected JACK running with buffer size %d, but actual buffer size is %d", m_bufferSize, bufferSize);
        emit startupError();
        return;
    }
    if (sampleRate != (unsigned int)m_sampleRate)
    {
        qWarning("Expected JACK running with sample rate %d, but actual sample rate is %d", m_sampleRate, sampleRate);
        emit startupError();
        return;
    }

    // register jack callbacks
    jack_set_buffer_size_callback(m_client, StreamingAudioManager::jackBufferSizeChanged, this);
    jack_set_process_callback(m_client, StreamingAudioManager::jackProcess, this);
    jack_set_sample_rate_callback(m_client, StreamingAudioManager::jackSampleRateChanged, this);
    jack_set_xrun_callback(m_client, StreamingAudioManager::jackXrun, this);
    jack_on_shutdown(m_client, StreamingAudioManager::jackShutdown, this);

    // activate client (starts processing)
    if (jack_activate(m_client) != 0)
    {
        qWarning("Couldn't activate JACK client");
        emit startupError();
        return;
    }

    if (!init_physical_ports())
    {
        emit startupError();
        return;
    }
    
    printf("\nSAM is now running.  Send OSC messages to port %d.\n", m_oscServerPort);

    m_isRunning = true;
}

bool StreamingAudioManager::stop()
{
    qDebug("StreamingAudioManager::Stop");
    qDebug() << "Thread = " << thread() << ", SAM thread = " << m_samThread;

    if (!m_isRunning) return false;

    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i])
        {
            delete m_apps[i];
            m_apps[i] = NULL;
        }
    }

    unregisterRenderer();
    
    // unregister UIs
    for (int i = 0; i < m_uiSubscribers.size(); i++)
    {
        delete m_uiSubscribers[i];
    }
    m_uiSubscribers.clear();
    
    // stop jack
    bool success = close_jack_client();
    success &= stop_jack();
    
    // stop OSC server
    m_tcpServer->close();
    m_udpSocket->close();

    if (m_udpSocket)
    {
        delete m_udpSocket;
        m_udpSocket = NULL;
    }

    if (m_tcpServer)
    {
        delete m_tcpServer;
        m_tcpServer = NULL;
    }

    m_samThread->quit();
    if (!m_samThread->wait(5000))
    {
        qWarning("Timed out waiting for SAM thread to quit");
    }

    if (success)
    {
        m_isRunning = false;
        qWarning("\nSAM stopped.\n");
    }

    return success;
}

bool StreamingAudioManager::idIsValid(int id)
{
    // check for out of range id
    if (id < 0 || id >= m_maxClients)
    {
        qDebug("StreamingAudioManager::idIsValid received out of range ID: %d", id);
        return false;
    }

    // make sure id is in use/initialized
    if (!m_apps[id])
    {
        qDebug("StreamingAudioManager::idIsValid received invalid ID: %d", id);
        return false;
    }

    return true;
}

int StreamingAudioManager::getNumApps()
{
    int count = 0;
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i] != NULL) count++;
    }
    return count;
}       

int StreamingAudioManager::registerApp(const char* name, int channels, int x, int y, int width, int height, int depth, StreamingAudioType type, QTcpSocket* socket)
{
    // TODO: check for duplicates (an app already at the same IP/port)?
    
    // find an available port
    int port = -1;
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_appState[i] == AVAILABLE)
        {
            port = i;
            break;
        }
    }
    
    if (port < 0)
    {
        // error: all ports in use
        qWarning("StreamingAudioManager::registerApp error: max clients already in use!");
        return -1;
    }
    
    // create a new app
    m_appState[port] = INITIALIZING;
    SamAppPosition pos;
    pos.x = x;
    pos.y = y;
    pos.width = width;
    pos.height = height;
    pos.depth = depth;
    m_apps[port] = new StreamingAudioApp(name, port, channels, pos, type, m_client, socket, m_rtpPort, m_delayMax, m_packetQueueSize, this);
    connect(m_apps[port], SIGNAL(appClosed(int,int)), this, SLOT(cleanupApp(int,int)));
    connect(m_apps[port], SIGNAL(appDisconnected(int)), this, SLOT(closeApp(int)));
    if (!m_apps[port]->init())
    {
        qWarning("StreamingAudioManager::registerApp error: could not initialize app!");
        m_apps[port]->flagForDelete();
        return -1;
    }

    // identify output ports for this app to connect to
    if (!allocate_output_ports(port, channels, type))
    {
        m_apps[port]->flagForDelete();
        return -1;
    }
    
    if (!connect_app_ports(port, m_apps[port]->getChannelAssignments()))
    {
        m_apps[port]->flagForDelete();
        return -1;
    }
    qDebug("StreamingAudioManager::registerApp finished connecting app ports");

    m_appState[port] = ACTIVE;

    // notify UI subscribers of app finished registering
    for (int i = 0; i < m_uiSubscribers.size(); i++)
    {
        OscAddress* replyAddr = m_uiSubscribers.at(i);
        OscMessage replyMsg;
        replyMsg.init("/sam/app/registered", "isiiiiiii", port,
                                                         m_apps[port]->getName(),
                                                         m_apps[port]->getNumChannels(),
                                                         pos.x,
                                                         pos.y,
                                                         pos.width,
                                                         pos.height,
                                                         pos.depth,
                                                         m_apps[port]->getType());
        if (!OscClient::sendUdp(&replyMsg, replyAddr))
        {
            qWarning("Couldn't send OSC message");
        }
    }
    
    // notify renderer
    if (m_renderer)
    {
        send_stream_added(m_apps[port], m_renderer);
    }
    
    return port;
}

bool StreamingAudioManager::unregisterApp(int port)
{
    if (port == -1)
    {
        // unregister all apps
        bool success = true;
        for (int i = 0; i < m_maxClients; i++)
        {
            if (m_apps[i])
            {
                success = success && unregisterApp(i);
            }
        }
        return success;
    }
    
    if (!m_apps[port] || (m_appState[port] != ACTIVE)) return false;

    // TODO: could this still be a race condition?  What if app is deleted/app status set to closing between check above and call below??
    // flag app for deletion - we can't delete it now because the JACK processing thread could be using it
    m_apps[port]->flagForDelete();

    return true;
}

bool StreamingAudioManager::registerUI(const char* host, quint16 port)
{
    // subscribe UI address to be notified when app registers/unregisters
    if (!StreamingAudioApp::subscribe(m_uiSubscribers, host, port))
    {
        return false;
    }
    
    // send ui/regconfirm message to UI
    OscAddress address;
    address.host.setAddress(host);
    address.port = port;
    OscMessage msg;
    msg.init("/sam/ui/regconfirm", "iif", getNumApps(), m_muteNext, m_volumeNext);
    if (!OscClient::sendUdp(&msg, &address))
    {
        qWarning("Couldn't send OSC message");
    }
    
    // send app/registered messages to UI for all currently-registered apps
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i])
        {
            SamAppPosition pos = m_apps[i]->getPosition();
            msg.clear();
            msg.init("/sam/app/registered", "isiiiiiii", m_apps[i]->getPort(),
                                                        m_apps[i]->getName(),
                                                        m_apps[i]->getNumChannels(),
                                                        pos.x,
                                                        pos.y,
                                                        pos.width,
                                                        pos.height,
                                                        pos.depth,
                                                        m_apps[i]->getType());
            if (!OscClient::sendUdp(&msg, &address))
            {
                qWarning("Couldn't send OSC message");
            }
        }       
    }
    
    return true;
}

bool StreamingAudioManager::unregisterUI(const char* host, quint16 port)
{
    // unsubscribe the UI from all apps
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i])
        {
            m_apps[i]->unsubscribeAll(host, port);
        }
    }
    
    // unsubscribe UI address from being notified when app registers/unregisters
    return StreamingAudioApp::unsubscribe(m_uiSubscribers, host, port);
}

bool StreamingAudioManager::registerRenderer(const char* hostname, quint16 port)
{
    if (m_renderer)
    {
        return false; // a renderer is already registered
    }
    
    OscAddress* address = new OscAddress;
    address->host.setAddress(hostname);
    address->port = port;
    m_renderer = address;
    
    // send regconfirm message to renderer
    OscMessage msg;
    msg.init("/sam/render/regconfirm", "");
    if (!OscClient::sendUdp(&msg, m_renderer))
    {
        qWarning("Couldn't send OSC message");
    }
    
    // send /sam/stream/add messages to renderer for all currently-registered apps
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i])
        {
            send_stream_added(m_apps[i], m_renderer);
        }       
    }
    
    return true;
}

bool StreamingAudioManager::unregisterRenderer()
{
    // TODO: need error checking to make sure the request is for this renderer??
    if (m_renderer)
    {
        delete m_renderer;
        m_renderer = NULL;
        return true;
    }
    
    return false; // nothing to unregister
}

void StreamingAudioManager::setVolume(float volume)
{
    m_volumeNext = volume >= 0.0 ? volume : 0.0;
    m_volumeNext = volume <= 1.0 ? volume : 1.0;

    // notify subscribers
    for (int i = 0; i < m_uiSubscribers.size(); i++)
    {
        OscAddress* replyAddr = m_uiSubscribers.at(i);
        OscMessage replyMsg;
        replyMsg.init("/sam/val/volume", "if", -1, m_volumeNext);
        if (!OscClient::sendUdp(&replyMsg, replyAddr))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

void StreamingAudioManager::setDelay(float delay)
{
    m_delayNext = m_sampleRate * (delay / 1000.0f);
    qDebug("StreamingAudioManager::setDelay requested delay = %d samples", m_delayNext);
    m_delayNext = (m_delayNext < 0) ? 0 : m_delayNext;
    m_delayNext = (m_delayNext >= m_delayMax) ? m_delayMax - 1 : m_delayNext;

    float delaySet = ((m_delayNext * 1000.0f) / (float)m_sampleRate); // actual delay set, in millis

    // notify subscribers
    for (int i = 0; i < m_uiSubscribers.size(); i++)
    {
        OscAddress* replyAddr = m_uiSubscribers.at(i);
        OscMessage replyMsg;
        replyMsg.init("/sam/val/delay", "if", -1, delaySet);
        if (!OscClient::sendUdp(&replyMsg, replyAddr))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

void StreamingAudioManager::setMute(bool isMuted)
{
    m_muteNext = isMuted;

    // notify subscribers
    for (int i = 0; i < m_uiSubscribers.size(); i++)
    {
        OscAddress* replyAddr = m_uiSubscribers.at(i);
        OscMessage replyMsg;
        replyMsg.init("/sam/val/mute", "ii", -1, m_muteNext);
        if (!OscClient::sendUdp(&replyMsg, replyAddr))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

bool StreamingAudioManager::setAppVolume(int port, float volume)
{
    if (port == -1)
    {
        // global volume
        setVolume(volume);
        return true;
    }
    
    // check for valid port
    if (!idIsValid(port)) return false;

    m_apps[port]->setVolume(volume);
    return true;
}

bool StreamingAudioManager::setAppMute(int port, bool isMuted)
{
    if (port == -1)
    {
        // global mute
        setMute(isMuted);
        return true;
    }
    
    // check for valid port
    if (!idIsValid(port)) return false;

    m_apps[port]->setMute(isMuted);
    return true;
}

bool StreamingAudioManager::setAppSolo(int port, bool isSolo)
{
    // check for valid port
    if (!idIsValid(port)) return false;

    m_apps[port]->setSolo(isSolo);
    return true;
}

bool StreamingAudioManager::setAppDelay(int port, float delay)
{
    if (port == -1)
    {
        // global delay
        setDelay(delay);
        return true;
    }

    // check for valid port
    if (!idIsValid(port)) return false;

    m_apps[port]->setDelay(delay);
    return true;
}

bool StreamingAudioManager::setAppPosition(int port, int x, int y, int width, int height, int depth)
{
    // check for valid port
    if (!idIsValid(port)) return false;

    SamAppPosition pos;
    pos.x = x;
    pos.y = y;
    pos.width = width;
    pos.height = height;
    pos.depth = depth;
    m_apps[port]->setPosition(pos);
    return true;
}

int StreamingAudioManager::setAppType(int port, StreamingAudioType type)
{
    // check for valid port
    if (!idIsValid(port)) return 0; // TODO: define error codes

    int typeOld = m_apps[port]->getType();
    if (typeOld == type) return -1; // nothing to do
    
    // re-assign ports if we are changing from basic to non-basic type and vice-versa
    if (typeOld == sam::TYPE_BASIC)
    {
        qDebug("StreamingAudioManager::setAppType switching app %d from basic type to non-basic type", port);
        
        // allocate new output ports
        if (!allocate_output_ports(port, m_apps[port]->getNumChannels(), type)) return 0; // TODO: define error codes
        
        // disconnect from old output ports
        if (!disconnect_app_ports(port)) return 0; // TODO: define error codes
        
        // connect to new output ports
        if (!connect_app_ports(port, m_apps[port]->getChannelAssignments())) return 0; // TODO: define error codes
    }
    else if (typeOld != sam::TYPE_BASIC && type == sam::TYPE_BASIC)
    {
        qDebug("StreamingAudioManager::SetAppType switching app %d from non-basic type to basic type", port);
        
        // allocate new output ports
        if (!allocate_output_ports(port, m_apps[port]->getNumChannels(), type)) return 0; // TODO: define error codes
        
        // disconnect from old output ports
        if (!disconnect_app_ports(port)) return 0; // TODO: define error codes
        
        // release old output ports used by this app
        // TODO: can use app's channel assigmments array to do this more efficiently ??
        for (int i = 0; i < m_numPhysicalPortsOut; i++)
        {
            if (m_outputUsed[i] == port)
            {
                m_outputUsed[i] = OUTPUT_ENABLED_DISCRETE;
            }
        }
        
        // connect to new output ports
        if (!connect_app_ports(port, m_apps[port]->getChannelAssignments())) return 0; // TODO: define error codes
    }
    else
    {
        qDebug("StreamingAudioManager::setAppType no output port switching required");
    }
    
    m_apps[port]->setType(type);
    
    // notify rendering engine: first remove app with the old type and re-add with the new type
    if (m_renderer)
    {
        qDebug("StreamingAudioManager::setAppType removing app with old type from renderer");
        OscMessage msg;
        msg.init("/sam/stream/remove", "i", port);
        if (!OscClient::sendUdp(&msg, m_renderer))
        {
                qWarning("Couldn't send OSC message");
        }
        qDebug("StreamingAudioManager::SetAppType adding app with new type to renderer");
        send_stream_added(m_apps[port], m_renderer);
    }
    
    qDebug("StreamingAudioManager::setAppType finished successfully");
    return -1;
}

int StreamingAudioManager::jackBufferSizeChanged(jack_nframes_t nframes, void*)
{
    qWarning("WARNING: JACK buffer size changed to %d/sec", nframes);
    return 0;
}

int StreamingAudioManager::jackProcess(jack_nframes_t nframes, void* sam)
{
    ((StreamingAudioManager*)sam)->jack_process(nframes);
    return 0;
}

int StreamingAudioManager::jackSampleRateChanged(jack_nframes_t nframes, void*)
{
    qWarning("WARNING: JACK sample rate changed to %d/sec", nframes);
    return 0;
}

int StreamingAudioManager::jackXrun(void* sam)
{
    qWarning("WARNING: JACK xrun");
    // TODO: notify SAM of xrun and have SAM notify RTP receivers
    ((StreamingAudioManager*)sam)->notifyXrun();
    return 0;
}

void StreamingAudioManager::notifyXrun()
{
    qWarning("StreamingAudioManager::notifyXrun");
    emit xrun();
}

void StreamingAudioManager::jackShutdown(void*)
{
    qWarning("StreamingAudioManager::jackShutdown() Exiting because JACK server shut down...");
    exit(1);
}

void StreamingAudioManager::doBeforeQuit()
{
    qDebug("StreamingAudioManager::doBeforeQuit");
    //stop();
}

void StreamingAudioManager::handleOscMessage(OscMessage* msg, const char* sender, QAbstractSocket* socket)
{
    qDebug("StreamingAudioManager::handleOscMessage");
    // check prefix
    int prefixLen = 5; // prefix is "/sam/"
    const char* address = msg->getAddress();
    qDebug("address = %s", address);
    if (qstrncmp(address, "/sam/", prefixLen) != 0)
    {
        printf("Unknown OSC message:\n");
        msg->print();
        delete msg;
        return;
    }

    // check second level of address
    if (qstrcmp(address + prefixLen, "quit") == 0) // /sam/quit
    {
        if (msg->getNumArgs() > 0)
        {
            printf("Unknown OSC message:\n");
            msg->print();
            delete msg;
            return;
        }
        qDebug("Received /sam/quit message");
        QCoreApplication::quit();
    }
    else if (qstrncmp(address + prefixLen, "debug", 5) == 0)
    {
        // print debug info to console
        qWarning("Received /sam/debug message");
        print_debug();
    }
    else if (qstrncmp(address + prefixLen, "app", 3) == 0)
    {
        handle_app_message(address + prefixLen + 3, msg, sender, socket);
    }
    else if (qstrncmp(address + prefixLen, "ui", 2) == 0)
    {
        handle_ui_message(address + prefixLen + 2, msg, sender);
    }
    else if (qstrncmp(address + prefixLen, "render", 6) == 0)
    {
        handle_render_message(address + prefixLen + 6, msg, sender);
    }
    else if (qstrncmp(address + prefixLen, "set", 3) == 0)
    {
        handle_set_message(address + prefixLen + 3, msg, sender, socket);
    }
    else if (qstrncmp(address + prefixLen, "get", 3) == 0)
    {
        handle_get_message(address + prefixLen + 3, msg, sender);
    }
    else if (qstrncmp(address + prefixLen, "subscribe", 9) == 0)
    {
        handle_subscribe_message(address + prefixLen + 9, msg, sender);
    }
    else if (qstrncmp(address + prefixLen, "unsubscribe", 11) == 0)
    {
        handle_unsubscribe_message(address + prefixLen + 11, msg, sender);
    }
    else
    {
        printf("Unknown OSC message:");
        msg->print();
    }
    delete msg;
}

void StreamingAudioManager::handle_app_message(const char* address, OscMessage* msg, const char* sender, QAbstractSocket* socket)
{
    if (qstrcmp(address, "/register") == 0) // /sam/app/register
    {
        if (socket->socketType() != QAbstractSocket::TcpSocket)
        {
            qWarning("StreamingAudioManager::handle_app_message ERROR: app register message must be sent using TCP!");
            return;
        }
    
        if (msg->typeMatches("siiiiiiiiiii"))
        {
            // register
            osc_register(msg, dynamic_cast<QTcpSocket*>(socket));
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrcmp(address, "/unregister") == 0) // /sam/app/unregister
    {
        if (msg->typeMatches("i"))
        {
            // unregister
            qDebug("SAM received message to unregister app: source host = %s", sender);

            OscArg arg;
            msg->getArg(0, arg);
            int port = arg.val.i;
            printf("Unregistering app %d\n\n", port);
            unregisterApp(port);

            // TODO: error handling?  handle return value from UnregisterApp?
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }
}

void StreamingAudioManager::handle_ui_message(const char* address, OscMessage* msg, const char* sender)
{
    if (!msg->typeMatches("i"))
    {
        printf("Unknown OSC message:\n");
        msg->print();
        return;
    }

    OscArg arg;
    msg->getArg(0, arg);
    quint16 replyPort = arg.val.i;

    qDebug("SAM received UI message: source host = %s", sender);

    // check third level of address
    if (qstrcmp(address, "/register") == 0) // /sam/ui/register
    {
        printf("Registering UI at host %s, port %d\n\n", sender, replyPort);
        bool success = registerUI(sender, replyPort);
        if (!success)
        {
            OscMessage replyMsg;
            replyMsg.init("/sam/ui/regdeny", "i", 0);
            OscAddress replyAddress;
            replyAddress.host.setAddress(sender);
            replyAddress.port = replyPort;
            if (!OscClient::sendUdp(&replyMsg, &replyAddress))
            {
                qWarning("Couldn't send OSC message");
            }
        }
    }
    else if (qstrcmp(address, "/unregister") == 0) // /sam/ui/unregister
    {
        printf("Unregistering UI at host %s, port %d\n\n", sender, replyPort);
        unregisterUI(sender, replyPort);
        // TODO: error handling?  handle return value from UnregisterUI?
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }
}

void StreamingAudioManager::handle_render_message(const char* address, OscMessage* msg, const char* sender)
{
    // check third level of address
    if (qstrcmp(address, "/register") == 0) // /sam/render/register
    {
        if (msg->typeMatches("i"))
        {
            qDebug("SAM received message to register a renderer: source host = %s", sender);

            OscArg arg;
            msg->getArg(0, arg);
            quint16 replyPort = arg.val.i;

            printf("Registering a renderer at host %s, port %d\n\n", sender, replyPort);
            bool success = registerRenderer(sender, replyPort);

            // TODO: move this code into RegisterRenderer??
            if (!success)
            {
                OscMessage replyMsg;
                replyMsg.init("/sam/render/regdeny", "i", 0);
                OscAddress replyAddress;
                replyAddress.host.setAddress(sender);
                replyAddress.port = replyPort;
                if (!OscClient::sendUdp(&replyMsg, &replyAddress))
                {
                    qWarning("Couldn't send OSC message");
                }
            }
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrcmp(address, "/unregister") == 0) // /sam/render/unregister
    {
        if (msg->typeMatches(""))
        {
            qDebug("SAM received message to unregister a renderer: source host = %s", sender);
            printf("Unregistering renderer\n\n");
            unregisterRenderer();
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }
}

void StreamingAudioManager::handle_set_message(const char* address, OscMessage* msg, const char* sender, QAbstractSocket* socket)
{
    // TODO: error handling (invalid port, etc.)

    // check third level of address
    if (qstrcmp(address, "/volume") == 0) // /sam/set/volume
    {
        if (msg->typeMatches("if"))
        {
            osc_set_volume(msg, sender);
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrcmp(address, "/mute") == 0) // /sam/set/mute
    {
        if (msg->typeMatches("ii"))
        {
            osc_set_mute(msg, sender);
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrcmp(address, "/solo") == 0) // /sam/set/solo
    {
        if (msg->typeMatches("ii"))
        {
            osc_set_solo(msg, sender);
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrcmp(address, "/delay") == 0) // /sam/set/delay
    {
        if (msg->typeMatches("if"))
        {
            osc_set_delay(msg, sender);
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrcmp(address, "/position") == 0) // /sam/set/position
    {
        if (msg->typeMatches("iiiiii"))
        {
            osc_set_position(msg, sender);
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else if (qstrcmp(address, "/type") == 0) // /sam/set/type
    {
        if (msg->typeMatches("iii"))
        {
            osc_set_type(msg, sender, socket);
        }
        else
        {
            printf("Unknown OSC message:\n");
            msg->print();
        }
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }
}

void StreamingAudioManager::handle_get_message(const char* address, OscMessage* msg, const char* sender)
{
    if (!msg->typeMatches("ii"))
    {
        printf("Unknown OSC message:\n");
        msg->print();
        return;
    }

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;
    msg->getArg(1, arg);
    quint16 replyPort = arg.val.i;

    qDebug("SAM received get message: source host = %s", sender);

    OscAddress replyAddr;
    replyAddr.host.setAddress(sender);
    replyAddr.port = replyPort;
    OscMessage replyMsg;

    // check for out of range port
    bool validPort = idIsValid(port);

    // check third level of address and init message
    if ((validPort || (port == -1)) && qstrcmp(address, "/volume") == 0) // /sam/get/volume
    {
        float volume = (port < 0) ? m_volumeNext : m_apps[port]->getVolume();
        replyMsg.init("/sam/val/volume", "if", port, volume);
    }
    else if ((validPort || (port == -1)) && qstrcmp(address, "/mute") == 0) // /sam/get/mute
    {
        bool mute = (port < 0) ? m_muteNext : m_apps[port]->getMute();
        replyMsg.init("/sam/val/mute", "ii", port, mute);
    }
    else if ((validPort) && qstrcmp(address, "/solo") == 0) // /sam/get/solo
    {
        bool solo = m_apps[port]->getSolo();
        replyMsg.init("/sam/val/solo", "ii", port, solo);
    }
    else if ((validPort || (port == -1)) && qstrcmp(address, "/delay") == 0) // /sam/get/delay
    {
        int delay = (port < 0) ? m_delayNext : m_apps[port]->getDelay();
        float delayMillis = ((delay * 1000.0f) / (float)m_sampleRate);
        replyMsg.init("/sam/val/delay", "if", port, delayMillis);
    }
    else if (validPort && qstrcmp(address, "/position") == 0) // /sam/get/position
    {
        SamAppPosition pos = m_apps[port]->getPosition();
        replyMsg.init("/sam/val/position", "iiiii", port, pos.x, pos.y, pos.width, pos.height);
    }
    else if (validPort && qstrcmp(address, "/type") == 0) // /sam/get/type
    {
        StreamingAudioType type = m_apps[port]->getType();
        replyMsg.init("/sam/val/type", "ii", port, type);
    }
    else if (validPort && qstrcmp(address, "/meter") == 0) // /sam/get/meter
    {
        qWarning("/sam/get/meter not implemented yet!");
        return;
    }
    else if (!validPort)
    {
        replyMsg.init("/sam/err/idinvalid", "i", port);
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
        return;
    }

    // send the reply
    if (!OscClient::sendUdp(&replyMsg, &replyAddr))
    {
        qWarning("Couldn't send OSC message");
    }
}

void StreamingAudioManager::handle_subscribe_message(const char* address, OscMessage* msg, const char* sender)
{
    if (!msg->typeMatches("ii"))
    {
        printf("Unknown OSC message:\n");
        msg->print();
        return;
    }

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;

    // check for out of range port
    if (!idIsValid(port)) return;

    msg->getArg(1, arg);
    quint16 replyPort = arg.val.i;

    qDebug("SAM received subscribe message: source host = %s", sender);

    // check third level of address
    if (qstrcmp(address, "/volume") == 0) // /sam/subscribe/volume
    {
        printf("Subscribing host %s, port %d to volume for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribeVolume(sender, replyPort);
    }
    else if (qstrcmp(address, "/mute") == 0) // /sam/subscribe/mute
    {
        printf("Subscribing host %s, port %d to mute for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribeMute(sender, replyPort);
    }
    else if (qstrcmp(address, "/solo") == 0) // /sam/subscribe/solo
    {
        printf("Subscribing host %s, port %d to solo for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribeSolo(sender, replyPort);
    }
    else if (qstrcmp(address, "/delay") == 0) // /sam/subscribe/delay
    {
        printf("Subscribing host %s, port %d to delay for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribeDelay(sender, replyPort);
    }
    else if (qstrcmp(address, "/position") == 0) // /sam/subscribe/position
    {
        printf("Subscribing host %s, port %d to position for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribePosition(sender, replyPort);
    }
    else if (qstrcmp(address, "/type") == 0) // /sam/subscribe/type
    {
        printf("Subscribing host %s, port %d to type for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribeType(sender, replyPort);
    }
    else if (qstrcmp(address, "/meter") == 0) // /sam/subscribe/meter
    {
        printf("Subscribing host %s, port %d to meter for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribeMeter(sender, replyPort);
    }
    else if (qstrcmp(address, "/all") == 0) // /sam/subscribe/all
    {
        printf("Subscribing host %s, port %d to all parameters for app %d\n\n", sender, replyPort, port);
        m_apps[port]->subscribeAll(sender, replyPort);
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }

    // TODO: error handling (invalid port, etc.)
}

void StreamingAudioManager::handle_unsubscribe_message(const char* address, OscMessage* msg, const char* sender)
{
    if (!msg->typeMatches("ii"))
    {
        printf("Unknown OSC message:\n");
        msg->print();
        return;
    }

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;

    // check for out of range port
    if (!idIsValid(port)) return;

    msg->getArg(1, arg);
    quint16 replyPort = arg.val.i;

    qDebug("SAM received unsubscribe message: source host = %s", sender);

    // check third level of address
    if (qstrcmp(address, "/volume") == 0) // /sam/subscribe/volume
    {
        printf("Unsubscribing host %s, port %d from volume for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribeVolume(sender, replyPort);
    }
    else if (qstrcmp(address, "/mute") == 0) // /sam/subscribe/mute
    {
        printf("Unsubscribing host %s, port %d from mute for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribeMute(sender, replyPort);
    }
    else if (qstrcmp(address, "/solo") == 0) // /sam/subscribe/solo
    {
        printf("Unsubscribing host %s, port %d from solo for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribeSolo(sender, replyPort);
    }
    else if (qstrcmp(address, "/delay") == 0) // /sam/subscribe/delay
    {
        printf("Unsubscribing host %s, port %d from delay for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribeDelay(sender, replyPort);
    }
    else if (qstrcmp(address, "/position") == 0) // /sam/subscribe/position
    {
        printf("Unsubscribing host %s, port %d from position for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribePosition(sender, replyPort);
    }
    else if (qstrcmp(address, "/type") == 0) // /sam/subscribe/type
    {
        printf("Unsubscribing host %s, port %d from type for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribeType(sender, replyPort);
    }
    else if (qstrcmp(address, "/meter") == 0) // /sam/subscribe/meter
    {
        printf("Unsubscribing host %s, port %d from meter for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribeMeter(sender, replyPort);
    }
    else if (qstrcmp(address, "/all") == 0) // /sam/subscribe/all
    {
        printf("Unsubscribing host %s, port %d from all parameters for app %d\n\n", sender, replyPort, port);
        m_apps[port]->unsubscribeAll(sender, replyPort);
    }
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }

    // TODO: error handling (invalid port, etc.)
}

bool StreamingAudioManager::start_jack(int sampleRate, int bufferSize, int outChannels, const char* driver)
{
    // error checking
    if (m_jackPID >= 0)
    {
        qWarning("StreamingAudioManager::start_jack() error: JACK server already started.  To restart, you must first stop the server.");
        return false;
    }

    // start jackd
    m_jackPID = StartJack(sampleRate, bufferSize, outChannels, driver);
    if (m_jackPID >= 0)
    {
        qDebug("Successfully started the JACK server with PID %d\n", m_jackPID);
        return true;
    }
    else
    {
        return false;
    }
}

bool StreamingAudioManager::stop_jack()
{
    if (m_jackPID < 0) return true; // nothing to stop
    bool success = StopJack(m_jackPID);
    if (success) m_jackPID = 0;
    return success;
}

bool StreamingAudioManager::open_jack_client()
{
    // open a client connection to the JACK server
    jack_status_t status;
    m_client = jack_client_open("StreamingAudioManager", JackNullOption, &status);
    if (m_client == NULL)
    {
        qWarning("StreamingAudioManager::open_jack_client(): jack_client_open() failed, status = 0x%2.0x", status);
        if (status & JackServerFailed)
        {
            qWarning("StreamingAudioManager::open_jack_client(): Unable to connect to JACK server");
        }
        return false;
    }
    if (status & JackServerStarted)
    {
        qWarning("StreamingAudioManager::open_jack_client(): had to start new JACK server");
    }
    if (status & JackNameNotUnique)
    {
        const char *client_name = jack_get_client_name(m_client);
        qWarning("StreamingAudioManager::open_jack_client(): unique client name `%s' assigned", client_name);
    }
    return true;
}

bool StreamingAudioManager::close_jack_client()
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
            return false;
        }
    }
    return true;
}

// ---- OSC message handlers ----
void StreamingAudioManager::osc_set_volume(OscMessage* msg, const char* sender)
{
    qDebug("SAM received message to set volume");
    qDebug("source host = %s", sender);

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;
    msg->getArg(1, arg);
    float volume = arg.val.f;
    printf("Setting volume for app at port %d to %f\n\n", port, volume);
    setAppVolume(port, volume);

    // if the port wasn't valid and SetAppVolume returns false, no error reply is sent because we don't have a reply port for this message.
    // TODO: should all messages have a reply port??
}

void StreamingAudioManager::osc_set_mute(OscMessage* msg, const char* sender)
{
    qDebug("SAM received message to set mute");
    qDebug("source host = %s", sender);

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;
    msg->getArg(1, arg);
    int mute = arg.val.i;
    printf("Setting mute for app %d to %d\n\n", port, mute);
    setAppMute(port, mute);
}

void StreamingAudioManager::osc_set_solo(OscMessage* msg, const char* sender)
{
    qDebug("SAM received message to set solo");
    qDebug("source host = %s", sender);

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;
    msg->getArg(1, arg);
    int solo = arg.val.i;
    printf("Setting solo for app %d to %d\n\n", port, solo);
    setAppSolo(port, solo);
    
    // TODO: error handling (invalid id, etc.)
}

void StreamingAudioManager::osc_set_delay(OscMessage* msg, const char* sender)
{
    qDebug("SAM received message to set delay");
    qDebug("source host = %s", sender);

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;
    msg->getArg(1, arg);
    float delay = arg.val.f;
    printf("Setting delay for app %d to %fms\n\n", port, delay);
    setAppDelay(port, delay);
}

void StreamingAudioManager::osc_set_position(OscMessage* msg, const char* sender)
{
    qDebug("SAM received message to set position");
    qDebug("source host = %s", sender);

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;
    msg->getArg(1, arg);
    int x = arg.val.i;
    msg->getArg(2, arg);
    int y = arg.val.i;
    msg->getArg(3, arg);
    int width = arg.val.i;
    msg->getArg(4, arg);
    int height = arg.val.i;
    msg->getArg(5, arg);
    int depth = arg.val.i;
    printf("Setting position for app %d to [%d %d %d %d %d]\n\n", port, x, y, width, height, depth);
    setAppPosition(port, x, y, width, height, depth);
}

void StreamingAudioManager::osc_set_type(OscMessage* msg, const char* sender, QAbstractSocket* socket)
{
    qDebug("SAM received message to set type");
    qDebug("source host = %s", sender);

    OscArg arg;
    msg->getArg(0, arg);
    int port = arg.val.i;
    msg->getArg(1, arg);
    StreamingAudioType type = (StreamingAudioType)arg.val.i;
    msg->getArg(2, arg);
    quint16 replyPort = arg.val.i;

    printf("Setting type for app %d to %d\n\n", port, type);
    
    // send response
    OscAddress replyAddr;
    replyAddr.host.setAddress(sender);
    replyAddr.port = replyPort;
    int errorCode = setAppType(port, type);
    if (errorCode < 0)
    {
        // send /sam/type/confirm message
        OscMessage msg;
        msg.init("/sam/type/confirm", "i", m_apps[port]->getType());
        
        if (socket->socketType() == QAbstractSocket::TcpSocket)
        {
            if (!OscClient::sendFromSocket(&msg, socket))
            {
                qWarning("Couldn't send OSC message");
            }
        }
        else if (!OscClient::sendUdp(&msg, &replyAddr))
        {
            qWarning("Couldn't send OSC message");
        }
    }
    else
    {
        // send /sam/type/deny message
        // TODO: define error codes
        OscMessage msg;
        msg.init("/sam/type/deny", "i", errorCode);
        if (!OscClient::sendUdp(&msg, &replyAddr))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

void StreamingAudioManager::osc_register(OscMessage* msg, QTcpSocket* socket)
{
    qDebug("SAM received message to register an app");
    qDebug() << "source host = " << socket->peerAddress();
    
    // register app
    OscArg arg;
    msg->getArg(0, arg);
    const char* name = arg.val.s;
    msg->getArg(1, arg);
    int channels = arg.val.i;
    msg->getArg(2, arg);
    int x = arg.val.i;
    msg->getArg(3, arg);
    int y = arg.val.i;
    msg->getArg(4, arg);
    int width = arg.val.i;
    msg->getArg(5, arg);
    int height = arg.val.i;
    msg->getArg(6, arg);
    int depth = arg.val.i;
    msg->getArg(7, arg);
    StreamingAudioType type = (StreamingAudioType)arg.val.i;
    msg->getArg(8, arg);
    int majorVersion = arg.val.i;
    msg->getArg(9, arg);
    int minorVersion = arg.val.i;
    msg->getArg(10, arg);
    int patchVersion = arg.val.i;
    msg->getArg(11, arg);
    quint16 replyPort = arg.val.i;

    int port = -1;
    // register if version matches
    if (majorVersion == sam::VERSION_MAJOR && minorVersion == sam::VERSION_MINOR && patchVersion == sam::VERSION_PATCH)
    {
        printf("Registering app at hostname %s, port %d with name %s, %d channel(s), position [%d %d %d %d %d], type = %d\n\n", socket->peerAddress().toString().toAscii().data(), replyPort, name, channels, x, y, width, height, depth, type);
        port = registerApp(name, channels, x, y, width, height, depth, type, socket);
    }
    else
    {
        qWarning("Denying app registration due to client version mismatch: SAM is version %d.%d.%d, client is %d.%d.%d",
                 sam::VERSION_MAJOR, sam::VERSION_MINOR, sam::VERSION_PATCH, majorVersion, minorVersion, patchVersion);
    }

    // send response
    if (port < 0)
    {
        OscMessage msg;
        msg.init("/sam/app/regdeny", "i", 0);
        if (!OscClient::sendFromSocket(&msg, socket))
        {
            qWarning("Couldn't send OSC message");
        }
    }
    else
    {
        OscMessage msg;
        msg.init("/sam/app/regconfirm", "iiii", port, m_sampleRate, m_bufferSize, m_rtpPort);
        if (!OscClient::sendFromSocket(&msg, socket))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

int StreamingAudioManager::jack_process(jack_nframes_t nframes)
{
    if (m_samplesElapsed > m_nextMeterNotify)
    {
        emit meterTick();
        m_nextMeterNotify += m_meterInterval;
    }

    // check if any app is solo'd (TODO: store this info so we don't have to iterate every time??)
    // also check for any apps that should be deleted
    bool soloNext = false;
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i])
        {
            if (m_apps[i]->shouldDelete())
            {
                qDebug("StreamingAudioManager::jack_process telling app %d to deleteLater, current thread = %p", i, QThread::currentThreadId());
                m_appState[i] = CLOSING;
                m_apps[i]->deleteLater();
            }
            else if (m_apps[i]->getSolo())
            {
                soloNext = true;
            }
        }
    }
    
    // have all apps do their own processing
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i] && m_appState[i] == ACTIVE) // TODO: maybe only need to check app state? but this is safer...
        {
            m_apps[i]->process(nframes, m_volumeCurrent, m_volumeNext, m_muteCurrent, m_muteNext, m_soloCurrent, soloNext, m_delayCurrent, m_delayNext);
           // TODO: need any kind of error handling here?
        }
    }
    
    m_volumeCurrent = m_volumeNext;
    m_muteCurrent = m_muteNext;
    m_soloCurrent = soloNext;
    m_delayCurrent = m_delayNext;
    
    m_samplesElapsed += nframes;

    return 0;
}

bool StreamingAudioManager::init_physical_ports()
{
    // get all jack ports that correspond to physical outputs
    const char** outputPorts = jack_get_ports(m_client, m_outputJackClientName, NULL, JackPortIsInput);

    if (!outputPorts)
    {
        qWarning("JACK client %s has no input ports", m_outputJackClientName);
        return false;
    }

    // count the number of physical output ports
    const char** currentPort = outputPorts;
    while (*currentPort != NULL)
    {
        m_numPhysicalPortsOut++;
        qDebug("StreamingAudioManager::init_physical_ports() counted port %s", *currentPort);
        currentPort++;
    }
    qDebug("StreamingAudioManager::init_physical_ports() counted %d physical outputs", m_numPhysicalPortsOut);
    jack_free(outputPorts);
    
    // -- HACK --
    //m_numPhysicalPortsOut -= m_outputPortOffset;
    // -- END HACK --

    m_outputUsed = new int[m_numPhysicalPortsOut];
    for (int i = 0; i < m_numPhysicalPortsOut; i++)
    {
        m_outputUsed[i] = OUTPUT_DISABLED;
    }

    for (int i = 0; i < m_basicChannels.size(); i++)
    {
        if (m_basicChannels[i] <= m_numPhysicalPortsOut)
        {
            m_outputUsed[m_basicChannels[i]-1] = OUTPUT_ENABLED_BASIC; // outputUsed is 0-indexed, while channel lists are 1-indexed
            qDebug("StreamingAudioManager::init_physical_ports() enabling basic channel: m_outputUsed[%d] = %d", m_basicChannels[i]-1, m_outputUsed[m_basicChannels[i]-1]);
        }
        else
        {
            qWarning("StreamingAudioManager::init_physical_ports() couldn't enable basic channel %u", m_basicChannels[i]);
        }
    }

    for (int i = 0; i < m_discreteChannels.size(); i++)
    {
        if (m_discreteChannels[i] <= m_numPhysicalPortsOut)
        {
            m_outputUsed[m_discreteChannels[i]-1] = OUTPUT_ENABLED_DISCRETE;
            qDebug("StreamingAudioManager::init_physical_ports() enabling discrete channel: m_outputUsed[%d] = %d", m_discreteChannels[i]-1, m_outputUsed[m_discreteChannels[i]-1]);
        }
        else
        {
            qWarning("StreamingAudioManager::init_physical_ports() couldn't enable discrete channel %u", m_discreteChannels[i]);
        }
    }

    return true;
}

bool StreamingAudioManager::send_stream_added(StreamingAudioApp* app, OscAddress* address)
{
    if (!address) return false;
    
    OscMessage msg;
    msg.init("/sam/stream/add", NULL);
    msg.addIntArg(app->getPort());
    msg.addIntArg(app->getType());

    // count the number of channels used
    int channels = app->getNumChannels();
    const int* channelAssign = app->getChannelAssignments();
    int numChannelsUsed = 0;
    if (channelAssign)
    {
        for (int ch = 0; ch < channels; ch++)
        {
            if (channelAssign[ch] >= 0) numChannelsUsed++;
        }
    }
    msg.addIntArg(numChannelsUsed);

    // add the channel assignments to the message
    if (channelAssign)
    {
        for (int ch = 0; ch < channels; ch++)
        {
            if (channelAssign[ch] < 0) continue;
            msg.addIntArg(channelAssign[ch]);
        }
    }

    if (!OscClient::sendUdp(&msg, address))
    {
        qWarning("Couldn't send OSC message");
        return false;
    }
    return true;
}

bool StreamingAudioManager::allocate_output_ports(int port, int channels, StreamingAudioType type)
{
    // identify output ports for this app to connect to
    switch (type)
    {
    case sam::TYPE_BASIC:
    {
        int numChannels = channels > m_basicChannels.size() ? m_basicChannels.size() : channels;
        // assign a basic port for each app output channel
        for (int ch = 0; ch < numChannels; ch++)
        {
            const char* appPortName = m_apps[port]->getOutputPortName(ch);
            if (!appPortName) return false;

            // connect the port to the next available output
            m_apps[port]->setChannelAssignment(ch, m_basicChannels[ch]);
        }
        // any additional app channels won't be connected to anything
        for (int ch = numChannels; ch < channels; ch++)
        {
            m_apps[port]->setChannelAssignment(ch, -1);
        }

        break;
    }
    case sam::TYPE_SPATIAL:
    case sam::TYPE_ARRAY:
    default: // TODO: simplify to iterate through m_discreteChannel list instead
    {
        // find a free output port for each app output channel
        int nextFreeOutput = 0;
        for (int ch = 0; ch < channels; ch++)
        {
            const char* appPortName = m_apps[port]->getOutputPortName(ch);
            if (!appPortName) return false;
            
            // connect the port to the next available output
            bool portFound = false;
            for (int k = nextFreeOutput; k < m_numPhysicalPortsOut; k++)
            {
                qDebug("StreamingAudioManager::allocate_output_ports m_outputUsed[%d] = %d", k, m_outputUsed[k]);
                if (m_outputUsed[k] != OUTPUT_ENABLED_DISCRETE) continue;
                m_apps[port]->setChannelAssignment(ch, k + 1);
                m_outputUsed[k] = port;
                nextFreeOutput = k + 1; // for the next port don't search the slots we already searched
                portFound = true;
                break;
            }
            if (!portFound)
            {
                qWarning("StreamingAudioManager::allocate_output_ports no ports available out of %d physical outputs!", m_numPhysicalPortsOut);
                
                // release output ports already allocated to this app
                // TODO: also undo channel assignment changes !?!
                for (int i = 0; i < m_numPhysicalPortsOut; i++)
                {
                    if (m_outputUsed[i] == port)
                    {
                        m_outputUsed[i] = OUTPUT_ENABLED_DISCRETE;
                    }
                }
                
                return false;
            }
        }
        break;
    }
    }
    return true;
}

bool StreamingAudioManager::connect_app_ports(int port, const int* outputPorts)
{
    qDebug("StreamingAudioManager::connect_app_ports starting");
    
    if (!m_outputJackClientName || !m_outputJackPortBase)
    {
        qWarning("StreamingAudioManager::connect_app_ports JACK client or port to connect to was unspecified");
        return false;
    }

    int channels = m_apps[port]->getNumChannels();
    for (int ch = 0; ch < channels; ch++)
    {
        // check that this channel should be connected
        if (outputPorts[ch] <= 0) continue;

        // connect the app's output port to the specified physical output
        char systemOut[MAX_PORT_NAME];
        snprintf(systemOut, MAX_PORT_NAME, "%s:%s%d", m_outputJackClientName, m_outputJackPortBase, outputPorts[ch] + m_outputPortOffset);
        // TODO: handle case where system ports have non-standard names (is this possible?)?
        const char* appPortName = m_apps[port]->getOutputPortName(ch);
        if (!appPortName) return false;
        int result = jack_connect(m_client, appPortName, systemOut);
        if (result == EEXIST)
        {
            qWarning("StreamingAudioManager::connect_app_ports WARNING: %s and %s were already connected", appPortName, systemOut);
            // TODO: decide how to handle this case
        }
        else if (result != 0)
        {
            qWarning("StreamingAudioManager::connect_app_ports ERROR: couldn't connect %s to %s", appPortName, systemOut);

            // release output ports allocated to this app
            for (int i = 0; i < channels; i++)
            {
                m_outputUsed[i] = -1;
            }

            return false;
        }
        else
        {
            qDebug("StreamingAudioManager::connect_app_ports connected %s to %s", appPortName, systemOut);
        }
    }
    
    qDebug("StreamingAudioManager::connect_app_ports finished");
    
    return true;
}

bool StreamingAudioManager::disconnect_app_ports(int port)
{
    qDebug("StreamingAudioManager::disconnect_app_ports starting");
              
    int channels = m_apps[port]->getNumChannels();
    for (int ch = 0; ch < channels; ch++)
    {
        // get the app output port's name
        const char* appPortName = m_apps[port]->getOutputPortName(ch);
        if (!appPortName) return false;
        
        // get a list of the ports this app's port is connected to
        const char** connections = jack_port_get_connections(jack_port_by_name(m_client, appPortName)); 
       
        if (!connections) continue; // this port isn't connected to anything
        
        const char** conn = connections;
        while (*conn != NULL)
        {
            const char* connName = *conn;
            // disconnect the port
            if (jack_disconnect(m_client, appPortName, connName) != 0)\
            {
                qWarning("StreamingAudioManager::disconnect_app_ports failed to disconnect %s and %s", appPortName, connName);
                return false;
            }
            else
            {
                qDebug("StreamingAudioManager::disconnect_app_ports disconnected %s and %s", appPortName, connName);
            }
            conn++;
        }
    }
    
    qDebug("StreamingAudioManager::disconnect_app_ports finished");
    
    return true;
}

void StreamingAudioManager::handleTcpConnection()
{
    qDebug("StreamingAudioManager::handleTcpConnection");
    QTcpSocket* socket = m_tcpServer->nextPendingConnection();
    int available = socket->bytesAvailable();
    qDebug() << "handling connection for socket " << socket << ", " << available << " bytesAvailable";
    OscTcpSocketReader* reader = new OscTcpSocketReader(socket);
    connect(socket, SIGNAL(readyRead()), reader, SLOT(readFromSocket()));
    connect(socket, SIGNAL(disconnected()), reader, SLOT(deleteLater()));
    connect(reader, SIGNAL(messageReady(OscMessage*, const char*, QAbstractSocket*)), this, SLOT(handleOscMessage(OscMessage*, const char*, QAbstractSocket*)));
}

void StreamingAudioManager::readPendingDatagrams()
{
    while (m_udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        qDebug() << "StreamingAudioManager::readPendingDatagrams UDP message = " << QString(datagram);

        OscMessage* oscMsg = new OscMessage();
        bool success = oscMsg->read(datagram);
        if (!success)
        {
            qDebug("StreamingAudioManager::readPendingDatagrams Couldn't read OSC message");
            delete oscMsg;
        }
        else
        {
            handleOscMessage(oscMsg, sender.toString().toAscii().data(), m_udpSocket);
        }
    }
}

void StreamingAudioManager::closeApp(int port)
{
    qDebug("StreamingAudioManager::closeApp port = %d", port);
    if (port >= 0) unregisterApp(port);
}

void StreamingAudioManager::cleanupApp(int port, int type)
{
    qDebug("StreamingAudioManager::cleanupApp port = %d, type = %d, current thread ID = %p", port, type, QThread::currentThreadId());

    m_apps[port] = NULL;

    if (type > sam::TYPE_BASIC)
    {
        // release output ports used by this app
        // TODO: can use app's channel assigmments array to do this more efficiently
        // TODO: should this been done from App to make sure that ports are only made available when they've been freed in JACK??
        for (int i = 0; i < m_numPhysicalPortsOut; i++)
        {
            if (m_outputUsed[i] == port)
            {
                m_outputUsed[i] = OUTPUT_ENABLED_DISCRETE;
            }
        }
    }

    if (m_appState[port] == ACTIVE || m_appState[port] == CLOSING)
    {
        // notify UI subscribers of app unregistering
        for (int i = 0; i < m_uiSubscribers.size(); i++)
        {
            OscAddress* replyAddr = m_uiSubscribers.at(i);
            OscMessage replyMsg;
            replyMsg.init("/sam/app/unregistered", "i", port);
            if (!OscClient::sendUdp(&replyMsg, replyAddr))
            {
                qWarning("Couldn't send OSC message");
            }
        }
        // notify renderer
        OscMessage replyMsg;
        replyMsg.init("/sam/stream/remove", "i", port);
        if (m_renderer && !OscClient::sendUdp(&replyMsg, m_renderer))
        {
            qWarning("Couldn't send OSC message");
        }

    }

    m_appState[port] = AVAILABLE;
}

void StreamingAudioManager::notifyMeter()
{
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i])
        {
            m_apps[i]->notifyMeter();
        }
    }
}

void StreamingAudioManager::print_debug()
{
    qWarning("\n--PRINTING DEBUG INFO--");

    qWarning("SAM global volume %f, mute %d, delay %d", m_volumeNext, m_muteNext, m_delayNext);

    qWarning("\nJACK port connections:");
    if (!m_client)
    {
        qWarning("JACK client is NULL, no ports to display");
    }

    // get all jack ports that correspond to physical outputs
    const char** portNames = jack_get_ports(m_client, NULL, NULL, 0);

    if (!portNames)
    {
        qWarning("no JACK ports detected");
        return;
    }

    const char** currentName = portNames;
    while (*currentName != NULL)
    {
        // find all ports connected to this one
        jack_port_t* port = jack_port_by_name(m_client, *currentName);
        if (!port)
        {
            qWarning("Couldn't get JACK port by name %s", *currentName);
            currentName++;
            continue;
        }

        const char** connPorts = jack_port_get_connections(port);

        if (!connPorts)
        {
            qWarning("JACK port %s has no connections", *currentName);
            currentName++;
            continue;
        }

        const char** currentPort = connPorts;
        while (*currentPort != NULL)
        {
            qWarning("JACK port %s connected to %s", *currentName, *currentPort);
            currentPort++;
        }

        jack_free(connPorts);
        currentName++;
    }
    jack_free(portNames);

    qWarning("\nSAM clients:");
    for (int i = 0; i < m_maxClients; i++)
    {
        if (m_apps[i])
        {
            SamAppPosition pos = m_apps[i]->getPosition();
            qWarning("Client %d has id %d, name \"%s\", %d channels, position [%d, %d, %d, %d, %d], type %d, volume %f, mute %d, solo %d, delay %f",
                                                        i,
                                                        m_apps[i]->getPort(),
                                                        m_apps[i]->getName(),
                                                        m_apps[i]->getNumChannels(),
                                                        pos.x,
                                                        pos.y,
                                                        pos.width,
                                                        pos.height,
                                                        pos.depth,
                                                        m_apps[i]->getType(),
                                                        m_apps[i]->getVolume(),
                                                        m_apps[i]->getMute(),
                                                        m_apps[i]->getSolo(),
                                                        m_apps[i]->getDelay());
        }
    }
    qWarning("--END PRINTING DEBUG INFO--\n");
}
