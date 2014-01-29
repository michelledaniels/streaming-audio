/**
 * @file sam_app.cpp
 * Implementation of functionality related to related to audio for a single client application
 * @author Michelle Daniels
 * @date September 2011
 * @copyright UCSD 2011-2012
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <QDebug>

#include "jack/jack.h"

#include "sam.h"
#include "sam_app.h"

namespace sam
{

static const int MAX_IP_LEN = 32;
static const int MAX_CMD_LEN = 32;
static const int MAX_PORT_NAME = 128;

static const quint32 REPORT_INTERVAL = 1000;

StreamingAudioApp::StreamingAudioApp(const char* name, 
                                     int port, 
                                     int channels, 
                                     const SamAppPosition& pos, 
                                     StreamingAudioType type, 
                                     int preset,
                                     jack_client_t* client, 
                                     QTcpSocket* socket, 
                                     quint16 rtpBasePort, 
                                     int maxDelay, 
                                     quint32 packetQueueSize, 
                                     qint32 clockSkewThreshold,
                                     StreamingAudioManager* sam, 
                                     QObject* parent) :
    QObject(parent),
    m_name(NULL),
    m_port(port),
    m_channels(channels),
    m_channelsUsed(channels),
    m_sampleRate(0),
    m_position(pos),
    m_type(type),
    m_preset(preset),
    m_deleteMe(false),
    m_sam(sam),
    m_channelAssign(NULL),
    m_jackClient(client),
    m_outputPorts(NULL),
    m_volumeCurrent(1.0f),
    m_volumeNext(1.0f),
    m_isMutedCurrent(false),
    m_isMutedNext(false),
    m_isSoloCurrent(false),
    m_isSoloNext(false),
    m_delayCurrent(0),
    m_delayNext(0),
    m_delayMax(maxDelay),
    m_delayBuffer(NULL),
    m_delayRead(NULL),
    m_delayWrite(NULL),
    m_rmsOut(NULL),
    m_peakOut(NULL),
    m_rmsIn(NULL),
    m_peakIn(NULL),
    m_receiver(NULL),
    m_rtpBasePort(rtpBasePort),
    m_packetQueueSize(packetQueueSize),
    m_clockSkewThreshold(clockSkewThreshold),
    m_socket(socket)
{
    qDebug("StreamingAudioApp::StreamingAudioApp app port = %d", m_port);
    int len = strlen(name);
    m_name = new char[len + 1];
    strncpy(m_name, name, len + 1);

    // allocate array of output port pointers
    m_outputPorts = new jack_port_t*[m_channels];

    // allocate arrays of RMS levels
    m_rmsOut = new float[m_channels];
    m_rmsIn = new float[m_channels];
    
    // allocate arrays of peak levels
    m_peakOut = new float[m_channels];
    m_peakIn = new float[m_channels];
    
    // allocate array for channel assignments
    m_channelAssign = new int[m_channels];

    // init arrays
    for (int ch = 0; ch < m_channels; ch++)
    {
        m_outputPorts[ch] = NULL;
        m_rmsOut[ch] = 0.0f;
        m_peakOut[ch] = 0.0f;
        m_rmsIn[ch] = 0.0f;
        m_peakIn[ch] = 0.0f;
        m_channelAssign[ch] = -1;
    }

    connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnectApp()));
}

StreamingAudioApp::~StreamingAudioApp()
{
    qDebug("StreamingAudioApp destructor called for app = %d", m_port);
    
    // store params for emitting appClosed later
    int port = m_port;
    int type = m_type;

    m_port = -1; // this will prevent SAM from trying to unregister us a second time from disconnectApp

    // disconnect this signal/slot: it was only for when the socket disconnected before the app was being deleted
    disconnect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnectApp()));

    // free array of ports
    if (m_outputPorts)
    {
        if (m_jackClient)
        {
            // unregister ports (this automatically disconnects ports)
            for (int i = 0; i < m_channels; i++)
            {
                if (m_outputPorts[i])
                {
                    jack_port_unregister(m_jackClient, m_outputPorts[i]);
                    m_outputPorts[i] = NULL;
                }
            }
        }
    
        delete[] m_outputPorts;
        m_outputPorts = NULL;
    }

    if (m_channelAssign)
    {
        delete[] m_channelAssign;
        m_channelAssign = NULL;
    }

    if (m_rmsOut)
    {
        delete[] m_rmsOut;
        m_rmsOut = NULL;
    }

    if (m_peakOut)
    {
        delete[] m_peakOut;
        m_peakOut = NULL;
    }

    if (m_rmsIn)
    {
        delete[] m_rmsIn;
        m_rmsIn = NULL;
    }

    if (m_peakIn)
    {
        delete[] m_peakIn;
        m_peakIn = NULL;
    }
    
    if (m_receiver)
    {
        delete m_receiver;
        m_receiver = NULL;
    }
    
    if (m_name)
    {
        delete[] m_name;
        m_name = NULL;
    }

    for (int i = 0; i < m_volumeSubscribers.size(); i++)
    {
        if (m_volumeSubscribers[i])
        {
            delete m_volumeSubscribers[i];
            m_volumeSubscribers[i] = NULL;
        }
    }
    m_volumeSubscribers.clear();
    for (int i = 0; i < m_muteSubscribers.size(); i++)
    {
        if (m_muteSubscribers[i])
        {
            delete m_muteSubscribers[i];
            m_muteSubscribers[i] = NULL;
        }
    }
    m_muteSubscribers.clear();
    for (int i = 0; i < m_delaySubscribers.size(); i++)
    {
        if (m_delaySubscribers[i])
        {
            delete m_delaySubscribers[i];
            m_delaySubscribers[i] = NULL;
        }
    }
    m_delaySubscribers.clear();
    for (int i = 0; i < m_positionSubscribers.size(); i++)
    {
        if (m_positionSubscribers[i])
        {
            delete m_positionSubscribers[i];
            m_positionSubscribers[i] = NULL;
        }
    }
    m_positionSubscribers.clear();
    for (int i = 0; i < m_typeSubscribers.size(); i++)
    {
        if (m_typeSubscribers[i])
        {
            delete m_typeSubscribers[i];
            m_typeSubscribers[i] = NULL;
        }
    }
    m_typeSubscribers.clear();
    
    if (m_audioData)
    {
        for (int ch = 0; ch < m_channels; ch++)
        {
            if (m_audioData[ch])
            {
                delete[] m_audioData[ch];
                m_audioData[ch] = NULL;
            }
        }
        delete[] m_audioData;
        m_audioData = NULL;
    }

    if (m_delayBuffer)
    {
        for (int ch = 0; ch < m_channels; ch++)
        {
            if (m_delayBuffer[ch])
            {
                delete[] m_delayBuffer[ch];
                m_delayBuffer[ch] = NULL;
            }
        }
        delete[] m_delayBuffer;
        m_delayBuffer = NULL;
    }

    if (m_delayRead)
    {
        delete[] m_delayRead;
        m_delayRead = NULL;
    }

    if (m_delayWrite)
    {
        delete[] m_delayWrite;
        m_delayWrite = NULL;
    }
    
    if (m_socket)
    {
        m_socket->close();
        m_socket->deleteLater(); // TODO: need this, or can delete on this thread??
        m_socket = NULL;
    }

    emit appClosed(port, type);
}

bool StreamingAudioApp::init()
{
    qDebug("StreamingAudioApp::init port = %d", m_port);

    // TODO: any error checking? Making sure that ports haven't already been registered somehow??

    if (!m_jackClient)
    {
        qWarning("StreamingAudioApp::init JACK client was NULL, port = %d", m_port);
        return false;
    }

    m_sampleRate = jack_get_sample_rate(m_jackClient);

    // register JACK output ports
    char portName[MAX_PORT_NAME];
    for (int i = 0; i < m_channels; i++)
    {
        snprintf(portName, MAX_PORT_NAME, "app%d-output_%d", m_port, i + 1);
        m_outputPorts[i] = jack_port_register(m_jackClient, portName, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (!m_outputPorts[i])
        {
            qWarning("StreamingAudioApp::init port = %d, ERROR: couldn't register output port for channel %d!", m_port, i + 1);
            return false;
        }
        qDebug("StreamingAudioApp::init port = %d registered output port %d", m_port, i);
    }
    
    // allocate audio buffer and delay line
    m_audioData = new float*[m_channels];
    m_delayBuffer = new float*[m_channels];
    m_delayRead = new int[m_channels];
    m_delayWrite = new int[m_channels];
    for (int ch = 0; ch < m_channels; ch++)
    {
        m_audioData[ch] = new float[jack_get_buffer_size(m_jackClient)];
        m_delayBuffer[ch] = new float[m_delayMax];
        m_delayRead[ch] = 0;
        m_delayWrite[ch] = 0;
    }

    // start receiver
    quint16 portOffset = m_port * 4;
    m_receiver = new RtpReceiver(portOffset + m_rtpBasePort, portOffset + m_rtpBasePort + 1, portOffset + m_rtpBasePort + 3, REPORT_INTERVAL, 1000 + m_port, jack_get_sample_rate(m_jackClient), jack_get_buffer_size(m_jackClient), m_packetQueueSize, m_clockSkewThreshold, m_jackClient, NULL);

    connect(m_sam, SIGNAL(xrun()), m_receiver, SLOT(handleXrun()));

    if (!m_receiver->start())
    {
        qWarning("StreamingAudioApp::init port = %d, ERROR: couldn't start RTP receiver!", m_port);
        return false;
    }
    
    return true;
}

void StreamingAudioApp::setVolume(float volume)
{
    m_volumeNext = volume >= 0.0 ? volume : 0.0;
    m_volumeNext = volume <= 1.0 ? volume : 1.0;

    // notify subscribers
    OscMessage replyMsg;
    replyMsg.init("/sam/val/volume", "if", m_port, m_volumeNext);
    QVector<OscAddress*>::iterator it;
    for (it = m_volumeSubscribers.begin(); it != m_volumeSubscribers.end(); it++)
    {
        if (!OscClient::sendUdp(&replyMsg, (OscAddress*)*it))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

void StreamingAudioApp::setMute(bool isMuted)
{
    m_isMutedNext = isMuted;

    // notify subscribers
    OscMessage replyMsg;
    replyMsg.init("/sam/val/mute", "ii", m_port, m_isMutedNext);
    QVector<OscAddress*>::iterator it;
    for (it = m_muteSubscribers.begin(); it != m_muteSubscribers.end(); it++)
    {
        if (!OscClient::sendUdp(&replyMsg, (OscAddress*)*it))
        {
            qWarning("Couldn't send OSC message");
        }
    }

    // notify the client
    if (!OscClient::sendFromSocket(&replyMsg, m_socket))
    {
        qWarning("Couldn't send OSC message");
    }
}

void StreamingAudioApp::setSolo(bool isSolo)
{
    m_isSoloNext = isSolo;

    // notify subscribers
    OscMessage replyMsg;
    replyMsg.init("/sam/val/solo", "ii", m_port, m_isSoloNext);
    QVector<OscAddress*>::iterator it;
    for (it = m_soloSubscribers.begin(); it != m_soloSubscribers.end(); it++)
    {
        if (!OscClient::sendUdp(&replyMsg, (OscAddress*)*it))
        {
            qWarning("Couldn't send OSC message");
        }
    }

    // notify the client
    if (!OscClient::sendFromSocket(&replyMsg, m_socket))
    {
        qWarning("Couldn't send OSC message");
    }
}

void StreamingAudioApp::setDelay(float delay)
{
    m_delayNext = m_sampleRate * (delay / 1000.0f);
    qDebug("StreamingAudioApp::setDelay requested delay = %d samples", m_delayNext);
    m_delayNext = (m_delayNext < 0) ? 0 : m_delayNext;
    // TODO: why is this max-1 and not max?
    m_delayNext = (m_delayNext >= m_delayMax) ? m_delayMax - 1 : m_delayNext;

    float delaySet = ((m_delayNext * 1000.0f) / (float)m_sampleRate); // actual delay set, in millis

    // notify subscribers
    OscMessage replyMsg;
    replyMsg.init("/sam/val/delay", "if", m_port, delaySet);
    QVector<OscAddress*>::iterator it;
    for (it = m_delaySubscribers.begin(); it != m_delaySubscribers.end(); it++)
    {
        if (!OscClient::sendUdp(&replyMsg, (OscAddress*)*it))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

void StreamingAudioApp::setType(StreamingAudioType type, int preset)
{
    qDebug("StreamingAudioApp::SetType type = %d, preset = %d", type, preset);
    m_type = type;
    m_preset = preset;

    // notify subscribers
    OscMessage replyMsg;
    replyMsg.init("/sam/val/type", "iii", m_port, m_type, m_preset);
    QVector<OscAddress*>::iterator it;
    for (it = m_typeSubscribers.begin(); it != m_typeSubscribers.end(); it++)
    {
        if (!OscClient::sendUdp(&replyMsg, (OscAddress*)*it))
        {
            qWarning("Couldn't send OSC message");
        }
    }

    // notify the client
    if (!OscClient::sendFromSocket(&replyMsg, m_socket))
    {
        qWarning("Couldn't send OSC message");
    }
}

void StreamingAudioApp::setPosition(const SamAppPosition& pos)
{
    m_position = pos;

    // notify subscribers
    OscMessage replyMsg;
    replyMsg.init("/sam/val/position", "iiiiii", m_port, m_position.x, m_position.y, m_position.width, m_position.height, m_position.depth);
    QVector<OscAddress*>::iterator it;
    for (it = m_positionSubscribers.begin(); it != m_positionSubscribers.end(); it++)
    {
        if (!OscClient::sendUdp(&replyMsg, (OscAddress*)*it))
        {
            qWarning("Couldn't send OSC message");
        }
    }
}

void StreamingAudioApp::setChannelAssignment(int appChannel, int assignChannel)
{
    if (appChannel < 0 || appChannel >= m_channels) return;
    m_channelAssign[appChannel] = assignChannel;
}

bool StreamingAudioApp::subscribe(const char* host, quint16 port, int param)
{
    OscMessage replyMsg;
    OscAddress replyAddress;
    replyAddress.host.setAddress(host);
    replyAddress.port = port;

    switch (param)
    {
    case SUBSCRIPTION_VOLUME:
        qDebug("StreamingAudioApp::subscribe to Volume id = %d", m_port);
        if (!subscribe_helper(m_volumeSubscribers, host, port)) return false;
        replyMsg.init("/sam/val/volume", "if", m_port, m_volumeNext);
        break;

    case SUBSCRIPTION_MUTE:
        qDebug("StreamingAudioApp::subscribe to Mute id = %d", m_port);
        if (!subscribe_helper(m_muteSubscribers, host, port)) return false;
        replyMsg.init("/sam/val/mute", "ii", m_port, m_isMutedNext);
        break;

    case SUBSCRIPTION_SOLO:
        qDebug("StreamingAudioApp::subscribe to Solo id = %d", m_port);
        if (!subscribe_helper(m_soloSubscribers, host, port)) return false;
        replyMsg.init("/sam/val/solo", "ii", m_port, m_isSoloNext);
        break;

    case SUBSCRIPTION_DELAY:
        qDebug("StreamingAudioApp::subscribe to Delay id = %d", m_port);
        if (!subscribe_helper(m_delaySubscribers, host, port)) return false;
        replyMsg.init("/sam/val/delay", "if", m_port, getDelay());
        break;

    case SUBSCRIPTION_POSITION:
        qDebug("StreamingAudioApp::subscribe to Position id = %d", m_port);
        if (!subscribe_helper(m_positionSubscribers, host, port)) return false;
        replyMsg.init("/sam/val/position", "iiiiii", m_port, m_position.x, m_position.y, m_position.width, m_position.height, m_position.depth);
    break;

    case SUBSCRIPTION_TYPE:
        qDebug("StreamingAudioApp::subscribe to Type id = %d", m_port);
        if (!subscribe_helper(m_typeSubscribers, host, port)) return false;
        replyMsg.init("/sam/val/type", "iii", m_port, m_type, m_preset);
        break;

    case SUBSCRIPTION_METER:
        qDebug("StreamingAudioApp::unsubscribe from Meter id = %d", m_port);
        if (!subscribe_helper(m_meterSubscribers, host, port)) return false;
        replyMsg.init("/sam/val/meter", "ii", m_port, m_channels);
        for (int ch = 0; ch < m_channels; ch++)
        {
            replyMsg.addFloatArg(m_rmsIn[ch]);
            replyMsg.addFloatArg(sqrt(m_peakIn[ch])); // only take square root when peak is sent, not each time it changes, for efficiency
            replyMsg.addFloatArg(m_rmsOut[ch]);
            replyMsg.addFloatArg(sqrt(m_peakOut[ch])); // only take square root when peak is sent, not each time it changes, for efficiency
            m_peakIn[ch] = 0.0f; // reset peak levels for next interval
            m_peakOut[ch] = 0.0f; // reset peak levels for next interval
        }
        break;

    default:
        qWarning("StreamingAudioApp::subscribe unknown parameter %d", param);
        return false;
    }

    if (!OscClient::sendUdp(&replyMsg, &replyAddress))
    {
        qWarning("Couldn't send OSC message");
        return false;
    }
    return true;
}

bool StreamingAudioApp::subscribeTcp(QTcpSocket* socket, int param)
{
    if (!socket)
    {
        qWarning("StreamingAudioApp::subscribeTcp can't subscribe a NULL socket!");
        return false;
    }

    OscMessage replyMsg;

    switch (param)
    {
    case SUBSCRIPTION_VOLUME:
        qDebug("StreamingAudioApp::subscribe to Volume id = %d", m_port);
        if (!subscribe_tcp_helper(m_volumeSubscribersTcp, socket)) return false;
        replyMsg.init("/sam/val/volume", "if", m_port, m_volumeNext);
        break;

    case SUBSCRIPTION_MUTE:
        qDebug("StreamingAudioApp::subscribe to Mute id = %d", m_port);
        if (!subscribe_tcp_helper(m_muteSubscribersTcp, socket)) return false;
        replyMsg.init("/sam/val/mute", "ii", m_port, m_isMutedNext);
        break;

    case SUBSCRIPTION_SOLO:
        qDebug("StreamingAudioApp::subscribe to Solo id = %d", m_port);
        if (!subscribe_tcp_helper(m_soloSubscribersTcp, socket)) return false;
        replyMsg.init("/sam/val/solo", "ii", m_port, m_isSoloNext);
        break;

    case SUBSCRIPTION_DELAY:
        qDebug("StreamingAudioApp::subscribe to Delay id = %d", m_port);
        if (!subscribe_tcp_helper(m_delaySubscribersTcp, socket)) return false;
        replyMsg.init("/sam/val/delay", "if", m_port, getDelay());
        break;

    case SUBSCRIPTION_POSITION:
        qDebug("StreamingAudioApp::subscribe to Position id = %d", m_port);
        if (!subscribe_tcp_helper(m_positionSubscribersTcp, socket)) return false;
        replyMsg.init("/sam/val/position", "iiiiii", m_port, m_position.x, m_position.y, m_position.width, m_position.height, m_position.depth);
        break;

    case SUBSCRIPTION_TYPE:
        qDebug("StreamingAudioApp::subscribe to Type id = %d", m_port);
        if (!subscribe_tcp_helper(m_typeSubscribersTcp, socket)) return false;
        replyMsg.init("/sam/val/type", "iii", m_port, m_type, m_preset);
        break;

    case SUBSCRIPTION_METER:
        qDebug("StreamingAudioApp::unsubscribe from Meter id = %d", m_port);
        if (!subscribe_tcp_helper(m_meterSubscribersTcp, socket)) return false;
        replyMsg.init("/sam/val/meter", "ii", m_port, m_channels);
        for (int ch = 0; ch < m_channels; ch++)
        {
            replyMsg.addFloatArg(m_rmsIn[ch]);
            replyMsg.addFloatArg(sqrt(m_peakIn[ch])); // only take square root when peak is sent, not each time it changes, for efficiency
            replyMsg.addFloatArg(m_rmsOut[ch]);
            replyMsg.addFloatArg(sqrt(m_peakOut[ch])); // only take square root when peak is sent, not each time it changes, for efficiency
            m_peakIn[ch] = 0.0f; // reset peak levels for next interval
            m_peakOut[ch] = 0.0f; // reset peak levels for next interval
        }
        break;

    default:
        qWarning("StreamingAudioApp::subscribeTcp unknown parameter %d", param);
        return false;
    }

    if (!OscClient::sendFromSocket(&replyMsg, socket))
    {
        qWarning("Couldn't send OSC message");
    }
    return true;
}

bool StreamingAudioApp::unsubscribe(const char* host, quint16 port, int param)
{
    switch (param)
    {
    case SUBSCRIPTION_VOLUME:
        qDebug("StreamingAudioApp::unsubscribe from Volume id = %d", m_port);
        return unsubscribe_helper(m_volumeSubscribers, host, port);

    case SUBSCRIPTION_MUTE:
        qDebug("StreamingAudioApp::unsubscribe from Mute id = %d", m_port);
        return unsubscribe_helper(m_muteSubscribers, host, port);

    case SUBSCRIPTION_SOLO:
        qDebug("StreamingAudioApp::unsubscribe from Solo id = %d", m_port);
        return unsubscribe_helper(m_soloSubscribers, host, port);

    case SUBSCRIPTION_DELAY:
        qDebug("StreamingAudioApp::unsubscribe from Delay id = %d", m_port);
        return unsubscribe_helper(m_delaySubscribers,host, port);

    case SUBSCRIPTION_POSITION:
        qDebug("StreamingAudioApp::unsubscribe from Position id = %d", m_port);
        return unsubscribe_helper(m_positionSubscribers, host, port);

    case SUBSCRIPTION_TYPE:
        qDebug("StreamingAudioApp::unsubscribe from Type id = %d", m_port);
        return unsubscribe_helper(m_typeSubscribers, host, port);

    case SUBSCRIPTION_METER:
        qDebug("StreamingAudioApp::unsubscribe from Meter id = %d", m_port);
        return unsubscribe_helper(m_meterSubscribers, host, port);

    default:
        qWarning("StreamingAudioApp::unsubscribe unknown parameter %d", param);
        return false;
    }
    return false;
}

bool StreamingAudioApp::unsubscribeTcp(QTcpSocket* socket, int param)
{
    switch (param)
    {
    case SUBSCRIPTION_VOLUME:
        qDebug("StreamingAudioApp::unsubscribeTcp from Volume id = %d", m_port);
        return unsubscribe_tcp_helper(m_volumeSubscribersTcp, socket);

    case SUBSCRIPTION_MUTE:
        qDebug("StreamingAudioApp::unsubscribeTcp from Mute id = %d", m_port);
        return unsubscribe_tcp_helper(m_muteSubscribersTcp, socket);

    case SUBSCRIPTION_SOLO:
        qDebug("StreamingAudioApp::unsubscribeTcp from Solo id = %d", m_port);
        return unsubscribe_tcp_helper(m_soloSubscribersTcp, socket);

    case SUBSCRIPTION_DELAY:
        qDebug("StreamingAudioApp::unsubscribeTcp from Delay id = %d", m_port);
        return unsubscribe_tcp_helper(m_delaySubscribersTcp, socket);

    case SUBSCRIPTION_POSITION:
        qDebug("StreamingAudioApp::unsubscribeTcp from Position id = %d", m_port);
        return unsubscribe_tcp_helper(m_positionSubscribersTcp, socket);

    case SUBSCRIPTION_TYPE:
        qDebug("StreamingAudioApp::unsubscribeTcp from Type id = %d", m_port);
        return unsubscribe_tcp_helper(m_typeSubscribersTcp, socket);

    case SUBSCRIPTION_METER:
        qDebug("StreamingAudioApp::unsubscribeTcp from Meter id = %d", m_port);
        return unsubscribe_tcp_helper(m_meterSubscribersTcp, socket);

    default:
        qWarning("StreamingAudioApp::unsubscribeTcp unknown parameter %d", param);
        return false;
    }
    return false;
}

bool StreamingAudioApp::subscribeAll(const char* host, quint16 port)
{
    for (int i = 0; i < NUM_SUBSCRIPTIONS; i++)
    {
        if (!subscribe(host, port, i)) return false;
    }

    return true;
}

bool StreamingAudioApp::unsubscribeAll(const char* host, quint16 port)
{
    for (int i = 0; i < NUM_SUBSCRIPTIONS; i++)
    {
        if (!unsubscribe(host, port, i)) return false;
    }

    return true;
}

bool StreamingAudioApp::subscribeAllTcp(QTcpSocket* socket)
{
    for (int i = 0; i < NUM_SUBSCRIPTIONS; i++)
    {
        if (!subscribeTcp(socket, i)) return false;
    }

    return true;
}

bool StreamingAudioApp::unsubscribeAllTcp(QTcpSocket* socket)
{
    for (int i = 0; i < NUM_SUBSCRIPTIONS; i++)
    {
        if (!unsubscribeTcp(socket, i)) return false;
    }

    return true;
}

bool StreamingAudioApp::notifyMeter()
{
    if (!m_rmsOut || !m_peakOut) return false;

    if (m_meterSubscribers.isEmpty()) return true;

    OscMessage replyMsg;
    replyMsg.init("/sam/val/meter", "ii", m_port, m_channels);
    for (int ch = 0; ch < m_channels; ch++)
    {
        replyMsg.addFloatArg(m_rmsIn[ch]);
        replyMsg.addFloatArg(sqrt(m_peakIn[ch])); // only take square root when peak is sent, not each time it changes, for efficiency
        replyMsg.addFloatArg(m_rmsOut[ch]);
        replyMsg.addFloatArg(sqrt(m_peakOut[ch])); // only take square root when peak is sent, not each time it changes, for efficiency
        m_peakIn[ch] = 0.0f; // reset peak levels for next interval
        m_peakOut[ch] = 0.0f; // reset peak levels for next interval
    }

    // send the current meter levels to all subscribers
    QVector<OscAddress*>::iterator it;
    for (it = m_meterSubscribers.begin(); it != m_meterSubscribers.end(); it++)
    {
        if (!OscClient::sendUdp(&replyMsg, (OscAddress*)*it))
        {
            qWarning("Couldn't send OSC message");
            return false;
        }
    }

    return true;
}

int StreamingAudioApp::process(jack_nframes_t nframes, float volumeCurrent, float volumeNext, bool muteCurrent, bool muteNext, bool soloCurrent, bool soloNext, int delayCurrent, int delayNext)
{
    float volumeStart = (muteCurrent || m_isMutedCurrent) ? 0.0f : volumeCurrent * m_volumeCurrent;
    volumeStart = (soloCurrent && !m_isSoloCurrent) ? 0.0f : volumeStart;
    float volumeEnd = (muteNext || m_isMutedNext) ? 0.0f : volumeNext * m_volumeNext;
    volumeEnd = (soloNext && !m_isSoloNext) ? 0.0f : volumeEnd;
    float volumeInc = (volumeEnd - volumeStart) / nframes;

    // init delay line
    int delayStart = delayCurrent + m_delayCurrent;
    int delayEnd = delayNext + m_delayNext;
    delayEnd = (delayEnd >= m_delayMax) ? m_delayMax - 1 : delayEnd;
    float delayInc = (delayEnd - delayStart) / (float)nframes;

    // get audio from the network
    m_receiver->receiveAudio(m_audioData, m_channels, nframes);

    // process audio only for channels that are actually used (connected to an output)
    for (int ch = 0; ch < m_channelsUsed; ch++)
    {
        if (!m_outputPorts)
        {
            qWarning("StreamingAudioApp::process for app %d: output ports are NULL!!", m_port);
            return -1;
        }
        jack_port_t* outPort = m_outputPorts[ch];

        if (outPort)
        {
            jack_default_audio_sample_t* out = (jack_default_audio_sample_t*)jack_port_get_buffer(outPort, nframes);
            if (!out)
            {
                qWarning("StreamingAudioApp::process for app %d couldn't get output buffer from JACK", m_port);
                return -1;
            }
            float rmsOut = 0.0f;
            float rmsIn = 0.0f;
            float volume = volumeStart + volumeInc;
            float delay = delayStart + delayInc;

            if (delayStart == delayEnd) // constant delay
            {
                m_delayRead[ch] = m_delayWrite[ch] - delayStart;
                while (m_delayRead[ch] < 0) m_delayRead[ch] += m_delayMax;

                for (unsigned int n = 0; n < nframes; n++)
                {
                    // write new sample to delay line
                    m_delayBuffer[ch][m_delayWrite[ch]++] = m_audioData[ch][n];
                    if (m_delayWrite[ch] >= m_delayMax) m_delayWrite[ch] = 0;

                    // read next sample from delay line
                    float delayOut = m_delayBuffer[ch][m_delayRead[ch]++];
                    if (m_delayRead[ch] >= m_delayMax) m_delayRead[ch] = 0;

                    out[n] = volume * delayOut;
                    float outSquared = out[n] * out[n];
                    rmsOut += outSquared;
                    if (outSquared > m_peakOut[ch])
                    {
                        m_peakOut[ch] = outSquared;
                    }
                    float inSquared = m_audioData[ch][n] * m_audioData[ch][n];
                    if (inSquared > m_peakIn[ch])
                    {
                        m_peakIn[ch] = inSquared;
                    }
                    rmsIn += inSquared;

                    volume += volumeInc;
                    delay += delayInc;
                }
            }
            else // delay is changing
            {
                m_delayRead[ch] = m_delayWrite[ch] - delayStart;
                while (m_delayRead[ch] < 0) m_delayRead[ch] += m_delayMax;

                for (unsigned int n = 0; n < nframes; n++)
                {
                    // write new sample to delay line
                    m_delayBuffer[ch][m_delayWrite[ch]++] = m_audioData[ch][n];
                    if (m_delayWrite[ch] >= m_delayMax) m_delayWrite[ch] = 0;

                    // read next sample from delay line
                    float delayOut = m_delayBuffer[ch][m_delayRead[ch]++];
                    if (m_delayRead[ch] >= m_delayMax) m_delayRead[ch] = 0;

                    // TODO: implement interpolation for varying delay

                    out[n] = volume * delayOut;
                    float outSquared = out[n] * out[n];
                    rmsOut += outSquared;
                    if (outSquared > m_peakOut[ch])
                    {
                        m_peakOut[ch] = outSquared;
                    }
                    float inSquared = m_audioData[ch][n] * m_audioData[ch][n];
                    if (inSquared > m_peakIn[ch])
                    {
                        m_peakIn[ch] = inSquared;
                    }
                    rmsIn += inSquared;

                    volume += volumeInc;
                    delay += delayInc;
                }
            }

            m_rmsOut[ch] = sqrt(rmsOut / nframes);
            m_rmsIn[ch] = sqrt(rmsIn / nframes);
            //m_peak[ch] = sqrt(peak);
            // TODO: convert RMS values to dB?
            // TODO: allow measurement over different lengths than just frame size
            // TODO: consider a longer decay time so peaks remain visible longer? (should be a UI thing?)
        }
        else
        {
            m_rmsOut[ch] = 0.0f;

            // compute input RMS and peak levels
            float rmsIn = 0.0f;
            for (unsigned int n = 0; n < nframes; n++)
            {
                float inSquared = m_audioData[ch][n] * m_audioData[ch][n];
                if (inSquared > m_peakIn[ch])
                {
                    m_peakIn[ch] = inSquared;
                }
                rmsIn += inSquared;
            }
            m_rmsIn[ch] = sqrt(rmsIn / nframes);
        }
        
        //qDebug("StreamingAudioApp:process RMS level for app %d channel %d = %0.4f", m_port, ch, m_rms[ch]);
    }

    // report zero levels for meters on channels that aren't connected to an output
    for (int ch = m_channelsUsed; ch < m_channels; ch++)
    {
        m_rmsIn[ch] = 0.0f;
        m_peakIn[ch] = 0.0f;
        m_rmsOut[ch] = 0.0f;
        m_peakOut[ch] = 0.0f;
    }
    
    m_volumeCurrent = m_volumeNext;
    m_isMutedCurrent = m_isMutedNext;
    m_isSoloCurrent = m_isSoloNext;
    m_delayCurrent = m_delayNext;
    
    return 0;
}

const char* StreamingAudioApp::getOutputPortName(unsigned int index)
{
    if ((int)index >= m_channels || !m_outputPorts || !m_outputPorts[index]) return NULL;
    
    return jack_port_name(m_outputPorts[index]);
}

bool StreamingAudioApp::getMeters(int ch, float& rmsIn, float& peakIn, float& rmsOut, float& peakOut)
{
    if (ch < 0 || ch >= m_channels) return false;

    rmsIn = m_rmsIn[ch];
    peakIn = m_peakIn[ch];
    rmsOut = m_rmsOut[ch];
    peakOut = m_peakOut[ch];
    return true;
}

bool StreamingAudioApp::subscribe_helper(QVector<OscAddress*> &subscribers, const char* hostRef, quint16 portRef)
{
    for (int i = 0; i < subscribers.size(); i++)
    {
        QString hostStr =  subscribers[i]->host.toString();
        QByteArray hostBytes = hostStr.toLocal8Bit();
        const char* host = hostBytes.constData();
        if (strcmp(host, hostRef) == 0 && (subscribers[i]->port == portRef))
        {
            // duplicate found - don't add again
            qWarning("StreamingAudioApp::subscribe tried to add duplicate address: hostname = %s, port = %d", hostRef, portRef);
            return true;
        }
    }

    // add subscriber
    OscAddress* address = new OscAddress();
    address->host.setAddress(hostRef);
    address->port = portRef;
    subscribers.push_back(address);
    return true;
}

bool StreamingAudioApp::unsubscribe_helper(QVector<OscAddress*> &subscribers, const char* hostRef, quint16 portRef)
{
    QVector<OscAddress*>::iterator it;
    for (it = subscribers.begin(); it != subscribers.end(); it++)
    {
        OscAddress* addr = *it;
        QString hostStr = (*it)->host.toString();
        QByteArray hostBytes = hostStr.toLocal8Bit();
        const char* host = hostBytes.constData();
        quint16 port = (*it)->port;
        if (strcmp(host, hostRef) == 0 && (port == portRef))
        {
            subscribers.erase(it);
            delete addr;
            return true;
        }
    }

    qWarning("StreamingAudioApp::unsubscribe address tried to unsubscribe address that was not already subscribed: hostname = %s, port = %d", hostRef, portRef);
    return false;
}

bool StreamingAudioApp::subscribe_tcp_helper(QVector<QTcpSocket*> &subscribers, QTcpSocket* socket)
{
    for (int i = 0; i < subscribers.size(); i++)
    {
        if (subscribers[i] == socket)
        {
            // duplicate found - don't add again
            qWarning("StreamingAudioApp::subscribeTcp tried to add duplicate socket");
            return true;
        }
    }

    // add subscriber
    subscribers.push_back(socket);
    return true;
}

bool StreamingAudioApp::unsubscribe_tcp_helper(QVector<QTcpSocket*> &subscribers, QTcpSocket* socket)
{
    QVector<QTcpSocket*>::iterator it;
    for (it = subscribers.begin(); it != subscribers.end(); it++)
    {
        QTcpSocket* itSocket = *it;
        if (itSocket == socket)
        {
            subscribers.erase(it);
            return true;
        }
    }

    qWarning("StreamingAudioApp::unsubscribeTcp address tried to unsubscribe socket that was not already subscribed");
    return false;
}

void StreamingAudioApp::disconnectApp()
{
    qDebug("StreamingAudioApp::disconnectApp %d, m_deleteMe = %d", m_port, m_deleteMe);
    if (!m_deleteMe) emit appDisconnected(m_port); // if the app is already flagged for deletion, we don't need to emit this
}

} // end of namespace SAM
