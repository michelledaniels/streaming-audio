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

#include "sam_client.h"

namespace sam
{

static const int MAX_CLIENT_NAME = 64;
static const quint32 REPORT_INTERVAL_MILLIS = 1000;

StreamingAudioClient::StreamingAudioClient() :
    QObject(),
    m_channels(1),
    m_type(TYPE_BASIC),
    m_port(-1),
    m_name(NULL),
    m_samIP(NULL),
    m_samPort(0),
    m_replyPort(0),
    m_interface(NULL),
    m_sender(NULL),
    m_rtpBasePort(0),
    m_audioCallback(NULL),
    m_audioCallbackArg(NULL),
    m_audioOut(NULL)
{
}

StreamingAudioClient::StreamingAudioClient(unsigned int numChannels, StreamingAudioType type, const char* name, const char* samIP, quint16 samPort, quint16 replyPort) :
    QObject(),
    m_channels(numChannels),
    m_type(type),
    m_port(-1),
    m_name(NULL),
    m_samIP(NULL),
    m_samPort(samPort),
    m_replyPort(replyPort),
    m_interface(NULL),
    m_sender(NULL),
    m_audioCallback(NULL),
    m_audioCallbackArg(NULL),
    m_audioOut(NULL)
{ 
    init(numChannels, type, name, samIP, samPort, replyPort);
}

StreamingAudioClient::~StreamingAudioClient()
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
    
    qDebug("End of StreamingAudioClient destructor");
}

int StreamingAudioClient::init(unsigned int numChannels, StreamingAudioType type, const char* name, const char* samIP, quint16 samPort, quint16 replyPort)
{
    if (m_port >= 0) return SAC_ERROR; // already initialized and registered

    m_channels = numChannels;
    m_type = type;
    m_samPort = samPort;
    m_replyPort = replyPort;

    // TODO: handle case where name or samIP are NULL??

    // copy name
    if (m_name)
    {
        delete[] m_name;
        m_name = NULL;
    }
    int len = strlen(name);
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
    printf("Sage Audio Client version %s\n", VERSION_STRING);
    printf("Built on %s at %s\n", __DATE__, __TIME__);
    printf("Copyright UCSD 2011-2013\n");
    printf("Connecting to SAM at IP %s, port %d\n", m_samIP, m_samPort);
    printf("--------------------------------------\n\n");
    
    QString samIPStr(m_samIP);
    m_socket.connectToHost(samIPStr, m_samPort);
    if (!m_socket.waitForConnected(timeout))
    {
        qWarning("StreamingAudioClient::start() couldn't connect to SAM");
        return SAC_TIMEOUT;
    }
    
    m_replyPort = m_socket.localPort();
    qDebug("StreamingAudioClient::start() replyPort set to %d\n", m_replyPort);
    
    
    qDebug("StreamingAudioClient::start() Sending register message to SAM: name = %s, channels = %d, position = [%d, %d, %d, %d, %d], type = %d", m_name, m_channels, x, y, width, height, depth, m_type);

    OscMessage msg;
    msg.init("/sam/app/register", "siiiiiiii", m_name, m_channels,
                                                       x,
                                                       y,
                                                       width,
                                                       height,
                                                       depth,
                                                       m_type,
                                                       m_replyPort);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("StreamingAudioClient::start() Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    
    qDebug("StreamingAudioClient::start() Finished sending register message to SAM, now waiting for response...");
    if (!m_socket.waitForReadyRead(timeout))
    {
        qWarning("StreamingAudioClient::start() timed out waiting for response to register request");
        return SAC_TIMEOUT;
    }
    
    qDebug("StreamingAudioClient::start() Received response from SAM");
    if (!read_from_socket()) // TODO: what if response doesn't arrive all at once? Is that possible?
    {
        qWarning("StreamingAudioClient::start() couldn't read OSC message from SAM");
        return SAC_OSC_ERROR;
    }
    
    return ((m_port >= 0) ? SAC_SUCCESS : SAC_REQUEST_DENIED);
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
    if (m_port < 0) return SAC_NOT_REGISTERED; // not yet registered
    if (m_type == type) return SAC_SUCCESS; // type already set
    
    OscMessage msg;
    msg.init("/sam/set/type", "iii", m_port, type, m_replyPort);
    if (!OscClient::sendFromSocket(&msg, &m_socket))
    {
        qWarning("StreamingAudioClient::setType() Couldn't send OSC message");
        return SAC_OSC_ERROR;
    }
    
    qDebug("StreamingAudioClient::setType() Finished sending set/type message to SAM, now waiting for response...");
    if (!m_socket.waitForReadyRead(timeout))
    {
        qWarning("StreamingAudioClient::setType() timed out waiting for response to set/type request");
        return SAC_TIMEOUT;
    }
    
    qDebug("StreamingAudioClient::setType() Received response from SAM");
    if (!read_from_socket())
    {
        qWarning("StreamingAudioClient::setType() couldn't read OSC message from SAM");
        return SAC_OSC_ERROR;
    }
    
    return ((m_type == type) ? SAC_SUCCESS : SAC_REQUEST_DENIED);
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
    m_audioCallback = callback; 
    m_audioCallbackArg = arg; 
    
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

void StreamingAudioClient::handleOscMessage(OscMessage* msg, const char* sender)
{
    qDebug("StreamingAudioClient::handleOscMessage sender = %s", sender);
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
                qDebug() << "Received regconfirm from SAM, id = " << port << ", sample rate = " << sampleRate << ", buffer size = " << bufferSize << ", base RTP port = " << rtpPort << endl;
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
                qDebug() << "SAM registration DENIED: error code = " << errorCode << endl;
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
            if (msg->typeMatches("i"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int type= arg.val.i;
                qDebug() << "Received typeconfirm from SAM, type = " << type << endl;
                handle_typeconfirm(type);
            }
            else
            {
                printf("Unknown OSC message:\n");
                msg->print();
            }
        }
        else if (qstrcmp(address + prefixLen + 4, "/deny") == 0) // /sam/type/deny
        {
            if (msg->typeMatches("i"))
            {
                OscArg arg;
                msg->getArg(0, arg);
                int errorCode = arg.val.i;
                qDebug() << "Type change DENIED, error code = " << errorCode << endl;
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
    else
    {
        printf("Unknown OSC message:");
        msg->print();
    }
    delete msg;
}

void StreamingAudioClient::handle_regconfirm(int port, unsigned int sampleRate, unsigned int bufferSize, quint16 rtpBasePort)
{
    printf("StreamingAudioClient registration confirmed: unique id = %d", port);
    
    // init RTP
    quint16 portOffset = port * 4;
    m_sender = new RtpSender(m_samIP, portOffset + rtpBasePort, portOffset + rtpBasePort + 3, portOffset + rtpBasePort + 1, REPORT_INTERVAL_MILLIS, sampleRate, m_channels, bufferSize, port, PAYLOAD_PCM_16);
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
    
    qDebug() << "StreamingAudioClient::handle_regconfirm started RtpSender" << endl;
    
    
    // init audio interface
#ifdef SAC_NO_JACK
    m_interface = new VirtualAudioInterface(m_channels, bufferSize, sampleRate);
#else
    // write the JACK client name
    char clientName[MAX_CLIENT_NAME];
    snprintf(clientName, MAX_CLIENT_NAME, "SAC-client%d", port);
    
    m_interface = new JackAudioInterface(m_channels, bufferSize, sampleRate, clientName);
#endif
    m_interface->setAudioCallback(StreamingAudioClient::interface_callback, this);
    
    // allocate audio buffer
    m_audioOut = new float*[m_channels];
    for (unsigned int ch = 0; ch < m_channels; ch++)
    {
        m_audioOut[ch] = new float[bufferSize];
    }

    // start interface (start sending audio)
    if (!m_interface->go())
    {
        qWarning() << "StreamingAudioClient::handle_regconfirm ERROR: couldn't start audio interface : unregistering with SAM" << endl;
        // unregister with SAM
        OscMessage msg;
        msg.init("/sam/app/unregister", "i", port);
        if (!OscClient::sendFromSocket(&msg, &m_socket))
        {
            qWarning("Couldn't send OSC message");
        }
        return;
    }

    qDebug() << "StreamingAudioClient::handle_regconfirm started audio interface" << endl;
    
    m_port = port;
}

void StreamingAudioClient::handle_regdeny(int errorCode)
{
    qDebug() << "SAM registration DENIED: error = " << QString::number(errorCode) << endl;
   
    // check to see if we timed out
    // TODO: checking and setting m_regTimedOut should be atomic/locked!
    // TODO: what if we tried re-registering after a time-out, and then received a regconfirm/regdeny??
    
    // TODO: do something with the error code?
}

void StreamingAudioClient::handle_typeconfirm(int type)
{
    qDebug() << "StreamingAudioClient::handle_typeconfirm, type = " << type << endl;
    
    m_type = (StreamingAudioType)type;
}

void StreamingAudioClient::handle_typedeny(int errorCode)
{
    qDebug() << "SAM type change DENIED: error = " << QString::number(errorCode) << endl;
   
    // TODO: do something with the error code?
}

bool StreamingAudioClient::send_audio(unsigned int nchannels, unsigned int nframes, float** in)
{ 
    // TODO: need error-checking to make sure nframes is not > buffer size?
    
    if (m_audioCallback)
    {
        //qDebug("StreamingAudioClient::send_audio calling registered audio callback");
        // call audio callback function if registered
        if (!m_audioCallback(m_channels, nframes, m_audioOut, m_audioCallbackArg))
        {
            qWarning("StreamingAudioClient::send_audio error occurred calling registered audio callback");
            // output zeros if an error occurred
            // TODO: handle this error differently??
            for (unsigned int ch = 0; ch < m_channels; ch++)
            {
                memset(m_audioOut[ch], 0, nframes * sizeof(float));
            }
        }
    }
    else if (in)
    {
        // otherwise grab input audio if physical inputs enabled
        for (unsigned int ch = 0; ch < m_channels; ch++)
        {
            // TODO: check that in[ch] is non-NULL?
            for (unsigned int n = 0; n < nframes; n++)
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
            memset(m_audioOut[ch], 0, nframes * sizeof (float));
        }
    }

    // send audio data
    m_sender->sendAudio(m_channels, nframes, m_audioOut);
    return true;
}

bool StreamingAudioClient::interface_callback(unsigned int nchannels, unsigned int nframes, float** in, float** out, void* sac)
{
    return ((StreamingAudioClient*)sac)->send_audio(nchannels, nframes, in);
}

bool StreamingAudioClient::read_from_socket()
{
    int available = m_socket.bytesAvailable();
    qDebug() << "StreamingAudioClient::readFromSocket() reading from TCP socket " << m_socket.socketDescriptor() << ", " << available << " bytes available";

    QByteArray block = m_socket.readAll();
    int start = block.startsWith(SLIP_END) ? 1 : 0;
    if (m_data.isEmpty() && (start == 0))
    {
        qWarning("StreamingAudioClient::readFromSocket() Invalid message fragment received: %s", block.constData());
        return false; // valid OSC messages must start with SLIP_END
    }

    if (start > 0)
    {
        m_data.clear();
    }

    int len = block.length();
    while (start < len)
    {
        int end = block.indexOf(SLIP_END, start);
        qDebug() << "start = " << start << ", end = " << end;
        if (start == end)
        {
            start++;
            continue;
        }
        if (end < 0)
        {
            // partial OSC message
            QByteArray msg = block.right(len - start);
            OscMessage::slipDecode(msg);
            m_data.append(msg);
            qDebug() << "StreamingAudioClient::readFromSocket() TCP partial message received = " << QString(m_data);
            break;
        }
        else
        {
            // end of an OSC message
            QByteArray msg = block.mid(start, end - start);
            OscMessage::slipDecode(msg);
            m_data.append(msg);
            // TODO: process OSC message
            qDebug() << "StreamingAudioClient::readFromSocket() TCP message: " << QString(m_data);
            qDebug() << "StreamingAudioClient::readFromSocket() Message length = " << m_data.length();

            OscMessage* oscMsg = new OscMessage();
            bool success = oscMsg->read(m_data);
            if (!success)
            {
                qDebug("StreamingAudioClient::readFromSocket() Couldn't read OSC message");
                delete oscMsg;
                return false;
            }
            else
            {
                handleOscMessage(oscMsg, m_socket.peerAddress().toString().toAscii().data());
            }

            start = end + 1;
            m_data.clear(); // prepare for new message
        }
    }
    return true;
}

} // end of namespace sam
