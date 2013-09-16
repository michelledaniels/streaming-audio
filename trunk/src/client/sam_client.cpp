/**
 * @file sam_client.cpp
 * Implementation for applications connecting to the Streaming Audio Manager
 * @author Michelle Daniels
 * @date January 2012
 * @copyright UCSD 2012
 */

#include <QCoreApplication>
#include <QDebug>
#include <QTcpSocket>
#include <QTimer>

#include "sam_client.h"

namespace sam
{

static const int MAX_CLIENT_NAME = 64;
static const quint32 REPORT_INTERVAL_MILLIS = 1000;

StreamingAudioClient::StreamingAudioClient() :
    QObject(),
    m_channels(1),
    m_bufferSize(0),
    m_sampleRate(0),
    m_type(TYPE_BASIC),
    m_preset(0),
    m_port(-1),
    m_name(NULL),
    m_samIP(NULL),
    m_samPort(0),
    m_payloadType(PAYLOAD_PCM_16),
    m_packetQueueSize(-1),
    m_replyIP(NULL),
    m_replyPort(0),
    m_responseReceived(false),
    m_oscReader(NULL),
    m_driveExternally(false),
    m_interface(NULL),
    m_sender(NULL),
    m_audioCallback(NULL),
    m_audioCallbackArg(NULL),
    m_audioOut(NULL),
    m_muteCallback(NULL),
    m_muteCallbackArg(NULL),
    m_soloCallback(NULL),
    m_soloCallbackArg(NULL),
    m_disconnectCallback(NULL),
    m_disconnectCallbackArg(NULL)
{
}

StreamingAudioClient::~StreamingAudioClient()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState)
    {
        // unregister with SAM
        if (m_port >= 0)
        {
            OscMessage msg;
            msg.init("/sam/app/unregister", "i", m_port);
            if (!OscClient::sendFromSocket(&msg, &m_socket))
            {
                qWarning("StreamingAudioClient::~StreamingAudioClient() Couldn't send OSC message");
            }
            m_port = -1;
        }

        m_socket.disconnect();
    }
    else
    {
        qWarning("StreamingAudioClient::~StreamingAudioClient() couldn't unregister from SAM because socket was already disconnected");
    }

    m_socket.close();

    if (m_oscReader)
    {
        delete m_oscReader;
        m_oscReader = NULL;
    }

    if (m_interface) // need to stop interface before deleting RtpSender
    {
        m_interface->stop();
        delete m_interface;
        m_interface = NULL;
    }
    
    if (m_sender)
    {
        delete m_sender;
        m_sender = NULL;
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
    
    if (m_samIP)
    {
        delete[] m_samIP;
        m_samIP = NULL;
    }
    
    if (m_name)
    {
        delete[] m_name;
        m_name = NULL;
    }

    if (m_replyIP)
    {
        delete[] m_replyIP;
        m_replyIP = NULL;
    }
}

int StreamingAudioClient::init(const SacParams& params)
{
    // init params that are not initialized in original init()
    m_preset = params.preset;
    m_packetQueueSize = params.packetQueueSize;

    // copy reply IP address
    if (params.replyIP)
    {
        if (m_replyIP)
        {
            delete[] m_replyIP;
            m_replyIP = NULL;
        }
        size_t len = strlen(params.replyIP);
        m_replyIP = new char[len + 1];
        strncpy(m_replyIP, params.replyIP, len + 1);
    }

    // init everything else with original init()
    return init(params.numChannels,
                params.type,
                params.name,
                params.samIP,
                params.samPort,
                params.replyPort,
                params.payloadType,
                params.driveExternally);
}

int StreamingAudioClient::init(unsigned int numChannels, StreamingAudioType type, const char* name, const char* samIP, quint16 samPort, quint16 replyPort, quint8 payloadType, bool driveExternally)
{
    if (m_port >= 0) return SAC_ERROR; // already initialized and registered

    if (!name || !samIP)
    {
        qWarning("StreamingAudioClient::init both name and samIP must be specified");
        return SAC_ERROR; // name and samIP must be specified
    }
    
    m_channels = numChannels;
    m_type = type;
    m_samPort = samPort;
    m_replyPort = replyPort;
    m_payloadType = payloadType;
    m_driveExternally = driveExternally;

    // copy name
    if (m_name)
    {
        delete[] m_name;
        m_name = NULL;
    }
    size_t len = strlen(name);
    m_name = new char[len + 1];
    strncpy(m_name, name, len + 1);

    // copy SAM IP address
    if (m_samIP)
    {
        delete[] m_samIP;
        m_samIP = NULL;
    }
    len = strlen(samIP);
    m_samIP = new char[len + 1];
    strncpy(m_samIP, samIP, len + 1);

    // need an instance of QCoreApplication for the event loop to run - check if one already exists
    if (!qApp)
    {
        int argc = 0;
        char ** argv = {0};
        (void) new QCoreApplication(argc, argv);
        qDebug("Instantiated QCoreApplication for event loop");
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::start(int x, int y, int width, int height, int depth, unsigned int timeout)
{
    if (m_port >= 0) return SAC_ERROR; // already registered
    if (!m_samIP) return SAC_ERROR; // not initialized yet
    
    printf("\n");
    printf("--------------------------------------\n");
    printf("Streaming Audio Client version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("Built on %s at %s\n", __DATE__, __TIME__);
    printf("Copyright UCSD 2011-2013\n");
    printf("Connecting to SAM at IP %s, port %d\n", m_samIP, m_samPort);
    printf("--------------------------------------\n\n");
    
    // bind to local IP and port (Qt 5+ only)
    if (m_replyIP || m_replyPort > 0)
    {
#if QT_VERSION >= 0x050000
        if (m_replyIP)
        {
            QString replyIPStr(m_replyIP);
            QHostAddress replyIPAddr(replyIPStr);
            m_socket.bind(replyIPAddr, m_replyPort);
        }
        else
        {
            m_socket.bind(QHostAddress::Any, m_replyPort);
        }
#else
        qWarning("Couldn't bind to specified local IP and port.  Qt version 5.0+ required.");
        return SAC_ERROR;
#endif
    }

    QString samIPStr(m_samIP);
    m_socket.connectToHost(samIPStr, m_samPort);
    if (!m_socket.waitForConnected(timeout))
    {
        qWarning("StreamingAudioClient::start() couldn't connect to SAM");
        return SAC_TIMEOUT;
    }
    
    m_replyPort = m_socket.localPort();

    // set up listening for OSC messages
    m_oscReader = new OscTcpSocketReader(&m_socket);
    connect(&m_socket, SIGNAL(readyRead()), m_oscReader, SLOT(readFromSocket()));
    connect(m_oscReader, SIGNAL(messageReady(OscMessage*, const char*, QAbstractSocket*)), this, SLOT(handleOscMessage(OscMessage*, const char*, QAbstractSocket*)));
    connect(&m_socket, SIGNAL(disconnected()), this, SLOT(samDisconnected()));
    
    OscMessage msg;
    msg.init("/sam/app/register", "siiiiiiiiiiiiii", m_name, m_channels,
                                                       x,
                                                       y,
                                                       width,
                                                       height,
                                                       depth,
                                                       m_type,
                                                       m_preset,
                                                       0, // placeholder for packet size/samples per packet requested
                                                       m_packetQueueSize,
                                                       VERSION_MAJOR,
                                                       VERSION_MINOR,
                                                       VERSION_PATCH,
                                                       m_replyPort);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("StreamingAudioClient::start() Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }

    // wait on response from SAM
    QEventLoop loop;
    connect(this, SIGNAL(responseReceived()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));
    loop.exec();

    if (!m_responseReceived)
    {
        qWarning("StreamingAudioClient::start() timed out waiting for response to register request");
        return SAC_TIMEOUT;
    }
    else if (m_port < 0)
    {
       return SAC_REQUEST_DENIED;
    }

    return SAC_SUCCESS;
}

int StreamingAudioClient::setMute(bool isMuted)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    OscMessage msg;
    msg.init("/sam/set/mute", "ii", m_port, isMuted);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::setSolo(bool isSolo)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    OscMessage msg;
    msg.init("/sam/set/solo", "ii", m_port, isSolo);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::setGlobalMute(bool isMuted)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    OscMessage msg;
    msg.init("/sam/set/mute", "ii", -1, isMuted);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::setPosition(int x, int y, int width, int height, int depth)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    qDebug("Sending position message to SAM: position = [%d, %d, %d, %d, %d]", x, y, width, height, depth);
    OscMessage msg;
    msg.init("/sam/set/position", "iiiiii", m_port, x,
                                                    y,
                                                    width,
                                                    height,
                                                    depth);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::setType(StreamingAudioType type, unsigned int timeout)
{
    return setTypeWithPreset(type, m_preset, timeout);
}

int StreamingAudioClient::setTypeWithPreset(StreamingAudioType type, int preset, unsigned int timeout)
{
    if (m_port < 0) return SAC_NOT_REGISTERED; // not yet registered
    if (m_type == type && m_preset == preset) return SAC_SUCCESS; // type already set

    m_responseReceived = false;
    OscMessage msg;
    msg.init("/sam/set/type", "iiii", m_port, type, preset, m_replyPort);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("StreamingAudioClient::setType() Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }

    // wait on response from SAM
    QEventLoop loop;
    connect(this, SIGNAL(responseReceived()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));
    loop.exec();

    if (!m_responseReceived)
    {
        qWarning("StreamingAudioClient::setType() timed out waiting for response to set/type request");
        return SAC_TIMEOUT;
    }

    return ((m_type == type && m_preset == preset) ? SAC_SUCCESS : SAC_REQUEST_DENIED);
}

int StreamingAudioClient::setVolume(float volume)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    OscMessage msg;
    msg.init("/sam/set/volume", "if", m_port, volume);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::setGlobalVolume(float volume)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    OscMessage msg;
    msg.init("/sam/set/volume", "if", -1, volume);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::setDelay(float delay)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    OscMessage msg;
    msg.init("/sam/set/delay", "if", m_port, delay);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    return SAC_SUCCESS;
}

int StreamingAudioClient::setAudioCallback(SACAudioCallback callback, void* arg)
{ 
    if (m_audioCallback) return SAC_ERROR; // callback already set
    m_audioCallback = callback; 
    m_audioCallbackArg = arg; 
    
    return SAC_SUCCESS;
}

int StreamingAudioClient::setMuteCallback(SACMuteCallback callback, void* arg)
{
    if (m_muteCallback) return SAC_ERROR; // callback already set
    m_muteCallback = callback;
    m_muteCallbackArg = arg;

    return SAC_SUCCESS;
}

int StreamingAudioClient::setSoloCallback(SACSoloCallback callback, void* arg)
{
    if (m_soloCallback) return SAC_ERROR; // callback already set
    m_soloCallback = callback;
    m_soloCallbackArg = arg;

    return SAC_SUCCESS;
}

int StreamingAudioClient::setDisconnectCallback(SACDisconnectCallback callback, void* arg)
{
    if (m_disconnectCallback) return SAC_ERROR; // callback already set
    m_disconnectCallback = callback;
    m_disconnectCallbackArg = arg;

    return SAC_SUCCESS;
}

int StreamingAudioClient::setPhysicalInputs(unsigned int* inputChannels)
{
    if (m_port < 0) return SAC_NOT_REGISTERED;
    
    if (!m_interface) return SAC_ERROR;
    
#ifdef SAC_NO_JACK
    return SAC_ERROR; // physical inputs not supported
#else
    // TODO: handle this casting better, or eliminate it??
    if (!dynamic_cast<JackAudioInterface*>(m_interface)->setPhysicalInputs(inputChannels)) return SAC_ERROR;
    return SAC_SUCCESS;
#endif
}

void StreamingAudioClient::samDisconnected()
{
    qWarning("StreamingAudioClient SAM was disconnected");
    if (m_disconnectCallback)
    {
        m_disconnectCallback(m_disconnectCallbackArg);
    }
}

void StreamingAudioClient::handleOscMessage(OscMessage* msg, const char* sender, QAbstractSocket* socket)
{
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
    if (qstrncmp(address + prefixLen, "app", 3) == 0)
    {
        // check third level of address
        if (qstrcmp(address + prefixLen + 3, "/regconfirm") == 0) // /sam/app/regconfirm
        {
            if (msg->typeMatches("iiii"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int port = arg.val.i;
                msg->getArg(1, arg);
                int sampleRate = arg.val.i;
                msg->getArg(2, arg);
                int bufferSize = arg.val.i;
                msg->getArg(3, arg);
                int rtpPort  = arg.val.i;
                qDebug("Received regconfirm from SAM, id = %d, sample rate = %d, buffer size = %d, base RTP port = %d", port, sampleRate, bufferSize, rtpPort);
                handle_regconfirm(port, sampleRate, bufferSize, (quint16)rtpPort);
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 3, "/regdeny") == 0) // /sam/app/regdeny)
        {
            if (msg->typeMatches("i"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int errorCode = arg.val.i;
                qWarning("SAM registration DENIED: error code = %d", errorCode);
                handle_regdeny(errorCode);
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
    else if (qstrncmp(address + prefixLen, "type", 4) == 0)
    {
        // check third level of address
        if (qstrcmp(address + prefixLen + 4, "/confirm") == 0) // /sam/type/confirm
        {
            if (msg->typeMatches("iii"))
            {
                OscArg arg;
                msg->getArg(1, arg);
                int type = arg.val.i; // for now ignore arg 0 (id)
                msg->getArg(2, arg);
                int preset = arg.val.i;
                qDebug("Received typeconfirm from SAM, type = %d, preset = %d", type, preset);
                handle_typeconfirm(type, preset);
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 4, "/deny") == 0) // /sam/type/deny
        {
            if (msg->typeMatches("iiii"))
            {
                OscArg arg;
                msg->getArg(3, arg);
                int errorCode = arg.val.i; // for now ignore args 0 and 1 (id, type, and preset)
                qDebug("Type change DENIED, error code = %d", errorCode);
                handle_typedeny(errorCode);
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
    else if (qstrncmp(address + prefixLen, "val", 3) == 0)
    {
        // check third level of address
        if (qstrcmp(address + prefixLen + 3, "/mute") == 0) // /sam/val/mute
        {
            if (msg->typeMatches("ii"))
            {
                OscArg arg;
                msg->getArg(1, arg);
                int mute = arg.val.i; // for now ignore arg 0 (id)
                qDebug("Received message from SAM that client mute status is %d", mute);
                // call mute callback
                if (m_muteCallback)
                {
                    m_muteCallback(mute, m_muteCallbackArg);
                }
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 3, "/solo") == 0) // /sam/val/solo
        {
            if (msg->typeMatches("ii"))
            {
                OscArg arg;
                msg->getArg(1, arg);
                int solo = arg.val.i; // for now ignore arg 0 (id)
                qDebug("Received message from SAM that client solo status is %d", solo);
                // call solo callback
                if (m_soloCallback)
                {
                    m_soloCallback(solo, m_soloCallbackArg);
                }
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 3, "/type") == 0) // /sam/val/type
        {
            if (msg->typeMatches("iii"))
            {
                OscArg arg;
                msg->getArg(1, arg);
                int type = arg.val.i; // for now ignore arg 0 (id)
                msg->getArg(2, arg);
                int preset = arg.val.i;
                qDebug("Received message from SAM that client type is %d, preset is %d", type, preset);
                // TODO: call type callback
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
    else
    {
        printf("Unknown OSC message:\n");
        msg->print();
    }
    delete msg;
}

void StreamingAudioClient::handle_regconfirm(int port, unsigned int sampleRate, unsigned int bufferSize, quint16 rtpBasePort)
{
    printf("StreamingAudioClient registration confirmed: unique id = %d, rtpBasePort = %d", port, rtpBasePort);

    m_bufferSize = bufferSize;
    m_sampleRate = sampleRate;

    // init RTP
    quint16 portOffset = port * 4;
    m_sender = new RtpSender(m_samIP, portOffset + rtpBasePort, portOffset + rtpBasePort + 3, portOffset + rtpBasePort + 1, REPORT_INTERVAL_MILLIS, sampleRate, m_channels, bufferSize, port, m_payloadType);
    if (!m_sender->init())
    {
        qWarning("StreamingAudioClient::handle_regconfirm couldn't initialize RtpSender: unregistering with SAM");
        // unregister with SAM
        OscMessage msg;
        msg.init("/sam/app/unregister", "i", port);
        if (!OscClient::sendFromSocket(&msg, &m_socket))
        {
            qWarning("Couldn't send OSC message");
        }
        return;
    }
    
    // allocate audio buffer
    m_audioOut = new float*[m_channels];
    for (unsigned int ch = 0; ch < m_channels; ch++)
    {
        m_audioOut[ch] = new float[bufferSize];
    }

    // initialize and start interface if not driving SAC sending externally
    if (!m_driveExternally)
    {
#ifdef SAC_NO_JACK
        m_interface = new VirtualAudioInterface(m_channels, bufferSize, sampleRate);
#else
        // write the JACK client name
        char clientName[MAX_CLIENT_NAME];
        snprintf(clientName, MAX_CLIENT_NAME, "SAC-client%d-%d", rtpBasePort, port);
    
        m_interface = new JackAudioInterface(m_channels, bufferSize, sampleRate, clientName);
#endif
        m_interface->setAudioCallback(StreamingAudioClient::interface_callback, this);

        // start interface (start sending audio)
        if (!m_interface->go())
        {
            qWarning("StreamingAudioClient::handle_regconfirm ERROR: couldn't start audio interface : unregistering with SAM");
            // unregister with SAM
            OscMessage msg;
            msg.init("/sam/app/unregister", "i", port);
            if (!OscClient::sendFromSocket(&msg, &m_socket))
            {
                qWarning("Couldn't send OSC message");
            }
            return;
        }
    }

    m_port = port;
    m_responseReceived = true;
    emit responseReceived();
}

void StreamingAudioClient::handle_regdeny(int errorCode)
{
    qWarning("SAM registration DENIED: error = %d", errorCode);
   
    // check to see if we timed out
    // TODO: what if we tried re-registering after a time-out, and then received a regconfirm/regdeny??
    
    // TODO: do something with the error code?
    m_responseReceived = true;
    emit responseReceived();
}

void StreamingAudioClient::handle_typeconfirm(int type, int preset)
{
    m_type = (StreamingAudioType)type;
    m_preset = preset;
    m_responseReceived = true;
    emit responseReceived();
}

void StreamingAudioClient::handle_typedeny(int errorCode)
{
    qWarning("SAM type change DENIED: error code = %d", errorCode);
    // TODO: do something with the error code?
    m_responseReceived = true;
    emit responseReceived();
}

int StreamingAudioClient::sendAudio(float** in)
{ 
    if (m_audioCallback)
    {
        // call audio callback function if registered
        if (!m_audioCallback(m_channels, m_bufferSize, m_audioOut, m_audioCallbackArg))
        {
            qWarning("StreamingAudioClient::sendAudio error occurred calling registered audio callback");
            // output zeros if an error occurred
            // TODO: handle this error differently??
            for (unsigned int ch = 0; ch < m_channels; ch++)
            {
                memset(m_audioOut[ch], 0, m_bufferSize * sizeof(float));
            }
        }
    }
    else if (in)
    {
        // otherwise grab input audio if physical inputs enabled (or SAC is driven externally)
        for (unsigned int ch = 0; ch < m_channels; ch++)
        {
            // TODO: check that in[ch] is non-NULL?
            for (unsigned int n = 0; n < m_bufferSize; n++)
            {
                m_audioOut[ch][n] = in[ch][n];
            }
        }
    }
    else
    {
        // output zeros if no callback is registered and no input data exists
        for (unsigned int ch = 0; ch < m_channels; ch++)
        {
            memset(m_audioOut[ch], 0, m_bufferSize * sizeof (float));
        }
    }

    // send audio data
    if (m_sender->sendAudio(m_channels, m_bufferSize, m_audioOut))
    {
        return SAC_SUCCESS;
    }
    else
    {
        return SAC_ERROR;
    }
}

bool StreamingAudioClient::interface_callback(unsigned int nchannels, unsigned int nframes, float** in, float** out, void* sac)
{
    // TODO: nchannels should match m_channels and nframes should match m_bufferSize
    return ((StreamingAudioClient*)sac)->sendAudio(in);
}

} // end of namespace sam
