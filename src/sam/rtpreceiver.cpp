/**
 * @file rtpreceiver.cpp
 * Implementation of RTP receiver functionality
 * @author Michelle Daniels
 * @date June 2012
 * @copyright UCSD 2012
 */

#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QtEndian>

#include "rtpreceiver.h"

namespace sam
{

static const quint16 MAX_DROPOUT = 3000;
static const quint16 MAX_MISORDER = 100;
static const quint16 MAX_LATE = 200;

static const int MAX_PORT_NAME = 64;
static const int JITTER_ADJUST_FACTOR = 3;

RtpReceiver::RtpReceiver(quint16 portRtp, 
                         quint16 portRtcpLocal, 
                         quint16 portRtcpRemote, 
                         quint32 reportInterval, 
                         quint32 ssrc, 
                         qint32 sampleRate, 
                         qint32 bufferSize, 
                         quint32 packetQueueSize, 
                         qint32 clockSkewThreshold,
                         jack_client_t* jackClient, 
                         QObject *parent) :
    QObject(parent),
    m_socketRtp(NULL),
    m_portRtp(portRtp),
    m_remotePortRtcp(portRtcpRemote),
    m_ssrc(ssrc),
    m_senderSsrc(0),
    m_playtime(0),
    m_timestampOffset(0),
    m_sampleRate(sampleRate),
    m_firstPacket(true),
    m_sequenceMax(0),
    m_sequenceWrapCount(0),
    m_badSequence(1),
    m_numLate(0),
    m_numMissed(0),
    m_packetQueue(NULL),
    m_bufferSamples(bufferSize),
    m_packetQueueSize(packetQueueSize),
    m_clockFirstTime(true),
    m_clockDelayEstimate(0),
    m_clockActiveDelay(0),
    m_clockSkewThreshold(clockSkewThreshold),
    m_jitterFirstTime(true),
    m_transitTimePrev(0),
    m_jitter(0),
    m_maxExtendedSeqNum(0),
    m_maxSeqNumThisInt(0),
    m_firstSeqNum(0),
    m_firstSeqNumThisInt(0),
    m_packetsReceived(0),
    m_packetsReceivedThisInt(0),
    m_reportInterval(reportInterval),
    m_lastSenderTimestamp(0),
    m_rtcpHandler(NULL),
    m_jackClient(jackClient),
    m_zeros(NULL)
{
    // init RTP socket
    m_socketRtp = new QUdpSocket(this);

    // init RTCP receiver
    QString host;
    m_rtcpHandler = new RtcpHandler(portRtcpLocal, m_ssrc, host, portRtcpRemote, this);

    // init buffer of zeros
    m_zeros = new float[m_bufferSamples];
    for (int i = 0; i < m_bufferSamples; i++)
    {
        m_zeros[i] = 0.0f;
    }
}

RtpReceiver::~RtpReceiver()
{
    // sockets will be destroyed by parent

    if (m_zeros)
    {
        delete[] m_zeros;
        m_zeros = NULL;
    }

    delete m_rtcpHandler;
    m_rtcpHandler = NULL;
    
    m_socketRtp->close();
    
    while (m_packetQueue)
    {
        RtpPacket* packet = m_packetQueue;
        m_packetQueue = m_packetQueue->m_next;
        delete packet;
    }
}

bool RtpReceiver::start()
{
    if (!m_socketRtp->bind(m_portRtp))
    {
        qWarning("RtpReceiver::start() RTP socket couldn't bind to port %d: %s", m_portRtp, m_socketRtp->errorString().toAscii().data());
        return false;
    }
    else
    {
        m_portRtp = m_socketRtp->localPort(); // TODO: is this necessary/desired?
        qDebug("RtpReceiver::start() RTP socket binded to port %d", m_portRtp);
    }
    
    // start timer for RTCP packet transmission
    QTimer* rtcpTimer = new QTimer(this);
    connect(rtcpTimer, SIGNAL(timeout()), this, SLOT(sendRtcpReport()));
    rtcpTimer->start(m_reportInterval);
    connect(this, SIGNAL(receiverReportReady(quint32, qint64, qint64, quint64, quint32, quint64, quint64, quint32, quint32, qint64)), 
                         m_rtcpHandler, SLOT(sendReceiverReport(quint32, qint64, qint64, quint64, quint32, quint64, quint64, quint32, quint32, qint64)));
    
    // start receiving packets
    connect(m_socketRtp, SIGNAL(readyRead()), this, SLOT(readPendingDatagramsRtp()));
    connect(m_rtcpHandler, SIGNAL(senderReportReceived(quint32)), this, SLOT(handleSenderReport(quint32)));
    return m_rtcpHandler->start();
}

// ---------- SLOTS ----------
void RtpReceiver::readPendingDatagramsRtp()
{
    while (m_socketRtp->hasPendingDatagrams())
    {
        RtpPacket* packet = read_datagram();
        if (!packet)
        {
            qWarning("RtpReceiver::readPendingDatagramsRtp received invalid RTP packet, ssrc = %u, RTP port = %d", m_ssrc, m_portRtp);
            delete packet;
            break;
        }
        
        m_senderSsrc = packet->m_ssrc; // TODO: check that this sender SSRC doesn't change (not receiving from multiple senders?)

        // update timestamp offset
        quint32 currentOffset = update_timestamp_offset(packet);

        // set extended sequence number
        if (!set_extended_seq_num(packet, currentOffset))
        {
            qWarning("RtpReceiver::readPendingDatagramsRtp couldn't set extended sequence number, ssrc = %u, RTP port = %d", m_ssrc, m_portRtp);
            delete packet;
            break;
        }

        // set packet playtime
        quint32 basePlayoutTime = packet->m_timestamp + m_timestampOffset;
        qint32 clockOffset = adjust_for_clock_skew(packet);
        qint32 jitterOffset = adjust_for_jitter(packet);
        packet->m_playoutTime = basePlayoutTime + clockOffset + jitterOffset; // TODO: make sure this can't try to go negative??
        //qWarning("RtpReceiver::readPendingDatagramsRtp received packet with sequence number %u, timestamp %u, arrival time %u, set playtime to %u, clockOffset = %d, jitterOffset = %d, current playtime = %u", packet->m_sequenceNum, packet->m_timestamp, packet->m_arrivalTime, packet->m_playoutTime, clockOffset, jitterOffset, m_playtime);

        
        // filter out late packets (take into account wrapping of playtime
        if (((qint32)(packet->m_playoutTime) - (qint32)m_playtime) < 0) //if (packet->m_playoutTime < m_playtime)
        {
            qWarning("RtpReceiver::readPendingDatagramsRtp LATE packet received: sequence number = %u, packet m_playoutTime = %u, current playtime = %u, ssrc = %u, RTP port = %d", packet->m_sequenceNum, packet->m_playoutTime, m_playtime, m_ssrc, m_portRtp);
            m_numLate++;
            if (m_numLate > MAX_LATE)
            {
                m_firstPacket = true; // force a reset with the next packet
                QDateTime currentTime = QDateTime::currentDateTime();
                QString currentTimeString = currentTime.toString();
                QByteArray currentTimeAscii = currentTimeString.toAscii();
                qWarning("[%s] RtpReceiver::readPendingDatagramsRtp TOO MANY LATE PACKETS received, forcing reset: ssrc = %u, RTP port = %d", currentTimeAscii.data(), m_ssrc, m_portRtp);
            }
            delete packet;
            break;
        }
        else
        {
            m_numLate = 0;
        }
		
		if (clockOffset >= 0)
		{
			// insert in queue based on sequence number
			insert_packet_in_queue(packet);
		}
		else 
		{
			qWarning("RtpReceiver::readPendingDatagramsRtp skipping inserting packet in queue after clock skew compensation");
		}

    }
}

void RtpReceiver::sendRtcpReport()
{
    qint64 delayMillis = m_reportTimer.elapsed();
    emit receiverReportReady(m_senderSsrc, 
                             m_firstSeqNumThisInt, 
                             m_maxSeqNumThisInt, 
                             m_packetsReceivedThisInt, 
                             m_firstSeqNum, 
                             m_maxExtendedSeqNum, 
                             m_packetsReceived, 
                             m_jitter, 
                             m_lastSenderTimestamp, 
                             delayMillis);
   
    // reset interval-specific stats
    m_firstSeqNumThisInt = 0; // TODO: this (and max) should be the sequence number of the next packet received
    m_maxSeqNumThisInt = 0;
    m_packetsReceivedThisInt = 0;
}

void RtpReceiver::handleSenderReport(quint32 lastSenderTimestamp)
{
    m_lastSenderTimestamp = lastSenderTimestamp;
    
    // restart timer for last sender delay
    // TODO: move this into RtcpHandler??
    m_reportTimer.restart();   
}

// -------------- HELPERS ---------------
RtpPacket* RtpReceiver::read_datagram()
{
    QByteArray datagram;
    datagram.resize(m_socketRtp->pendingDatagramSize());
    quint16 senderPort;
    m_socketRtp->readDatagram(datagram.data(), datagram.size(), &m_sender, &senderPort);

    // Get current timestamp from JACK
    if (!m_jackClient) return NULL;
    jack_nframes_t elapsedSamples = jack_frame_time(m_jackClient);

    //qDebug() << "\nDATAGRAM RECEIVED on RTP port: time elapsed = " << timeElapsed << "ms, " << elapsedSamples << "samples";
    
    // handle RTP packet
    RtpPacket* packet = new RtpPacket();
    packet->read(datagram, elapsedSamples); // TODO: this should return false if not valid RTP packet
    m_packetsReceived++;
    m_packetsReceivedThisInt++; // includes late or duplicated packets
    
    // TODO: check that this is a valid packet, if not valid, return NULL
    
    return packet;
}

quint32 RtpReceiver::update_timestamp_offset(RtpPacket* packet)
{
    quint32 currentOffset = packet->m_arrivalTime - packet->m_timestamp;
    quint32 offsetDiff = currentOffset - m_timestampOffset;
    if ((offsetDiff & 0x80000000) != 0) // is offset < m_timestampOffset with unsigned comparison
    {
        qDebug("RtpReceiver::update_timestamp_offset: timestamp offset UPDATED: previous offset = %u, new offset = %u, ssrc = %u, RTP port = %d", m_timestampOffset, currentOffset, m_ssrc, m_portRtp);
        m_timestampOffset = currentOffset;
        //qWarning("RtpReceiver::update_timestamp_offset: previous playtime = %u, new playtime = %u, offset difference = %u, ssrc = %u, RTP port = %d", m_playtime, m_playtime + offsetDiff, offsetDiff, m_ssrc, m_portRtp);
        //m_playtime += offsetDiff; // also update playtime
        //qDebug("RtpReceiver::update_timestamp_offset: timestamp offset BYPASSING UPDATE: offset = %u, new offset should be = %u", m_timestampOffset, currentOffset);
    }
    
    return currentOffset;
}

void RtpReceiver::init_stats(RtpPacket* packet, quint32 currentOffset)
{
    m_clockFirstTime = true;
    m_clockDelayEstimate = 0;
    m_clockActiveDelay = 0;
    m_jitterFirstTime = true;
    m_transitTimePrev = 0;
    m_jitter = 0;
    //m_playtime = packet->m_arrivalTime;
    m_timestampOffset = currentOffset;
    m_sequenceMax = packet->m_sequenceNum;
    m_sequenceWrapCount = 0;
    m_maxExtendedSeqNum = m_sequenceMax;
    m_firstSeqNum = m_sequenceMax;
    m_firstSeqNumThisInt = m_sequenceMax;
    m_maxSeqNumThisInt = m_sequenceMax;
    m_packetsReceived = 1;
    m_packetsReceivedThisInt = 1;
}

bool RtpReceiver::set_extended_seq_num(RtpPacket* packet, quint32 currentOffset)
{
    // set extended sequence number
    // NOTE: wrap-around code based on RTP book page 76
    quint16 udelta = packet->m_sequenceNum - m_sequenceMax;

    //qDebug("RtpReceiver::set_extended_seq_num udelta = %d", udelta);
            
    if (m_firstPacket)
    {
        init_stats(packet, currentOffset);
        m_firstPacket = false;
        m_rtcpHandler->setRemoteHost(m_sender);
        qDebug("RECEIVED FIRST PACKET");
    }
    else if (udelta < MAX_DROPOUT)
    {
        if (packet->m_sequenceNum < m_sequenceMax)
        {
            m_sequenceWrapCount++;
        }
        m_sequenceMax = packet->m_sequenceNum;
    }
    else if (udelta <= 65535 - MAX_MISORDER)
    {
        // the sequence number made a very large jump
        if (packet->m_sequenceNum == m_badSequence)
        {
            // two sequential packets received, assume the other side restarted without telling us
            // TODO: how to handle this scenario?
            qWarning("RtpReceiver::set_extended_seq_num RESETTING: sequence number made large jump");
            qint64 packetsExpected = m_maxExtendedSeqNum - m_firstSeqNum + 1;
            qDebug() << "RtpReceiver::set_extended_seq_num previous sequence session packets expected = " << packetsExpected << ", packets received = " << (m_packetsReceived - 2);
            init_stats(packet, currentOffset);
            
            // TODO: need to remove anything in queue?
            // TODO: is it ok that we ignored the first of the two sequential packets, or should we always save it?
        }
        else
        {
            m_badSequence = packet->m_sequenceNum + 1;
            qWarning("RtpReceiver::set_extended_seq_num received BADLY MISORDERED packet: sequence num = %u, ssrc = %u, RTP port = %d", packet->m_sequenceNum, m_ssrc, m_portRtp);
            return false;
        }
    }
    else
    {
        // duplicate or misordered packet
        qWarning("RtpReceiver::set_extended_seq_num DUPLICATE OR MISORDERED packet received: sequence number = %u, ssrc = %u, RTP port = %d", packet->m_sequenceNum, m_ssrc, m_portRtp);
    }
    packet->m_extendedSeqNum = packet->m_sequenceNum + (65536 * m_sequenceWrapCount);
    m_maxExtendedSeqNum = (packet->m_extendedSeqNum > m_maxExtendedSeqNum) ? packet->m_extendedSeqNum : m_maxExtendedSeqNum;

    return true;
}

void RtpReceiver::insert_packet_in_queue(RtpPacket* packet)
{
    // ============== START LOCKED SECTION ===============
    QMutexLocker locker(&m_queueMutex);
    // remove any used packets from the beginning of the queue
    while (m_packetQueue && m_packetQueue->m_used)
    {
        // TODO: could recycle the used packet to use for next incoming packet
        RtpPacket* used = m_packetQueue;
        m_packetQueue = m_packetQueue->m_next;
        //qWarning("RtpReceiver::insert_packet_in_queue REMOVED PACKET from queue: sequence number = %u, playtime = %u", used->m_sequenceNum, used->m_playoutTime);
        delete used;
    }
    locker.unlock();
    // =============== END LOCKED SECTION ================   
    
    if (!m_packetQueue)
    {
        // queue is empty - start new one
        m_packetQueue = packet;
        //qWarning("RtpReceiver::insert_packet_in_queue INSERTED PACKET in empty queue: sequence number = %u, playtime = %u", packet->m_sequenceNum, packet->m_playoutTime);
        return;
    }
    
    // insert the new packet into the queue
    RtpPacket* currentPacket = m_packetQueue;
    RtpPacket* previousPacket = NULL;
    while (currentPacket)
    {
        if (packet->m_extendedSeqNum == currentPacket->m_extendedSeqNum)
        {
            // duplicate packet: do nothing
            qWarning("RtpReceiver::insert_packet_in_queue IGNORING DUPLICATE PACKET: sequence number = %u", packet->m_sequenceNum);
            delete packet;
            break;
        }
        else if (packet->m_extendedSeqNum < currentPacket->m_extendedSeqNum) // TODO: this comparison could be a problem if m_extendedSeqNum ever wrapped around, but since it's 64 bits, that's very unlikely
        {
            // insert packet before current packet
            packet->m_next = currentPacket;
            if (previousPacket) previousPacket->m_next = packet;
            //qWarning("RtpReceiver::insert_packet_in_queue INSERTED PACKET before current packet: sequence number = %u, playtime = %u", packet->m_sequenceNum, packet->m_playoutTime);
            break;
        }
        else
        {
            if (currentPacket->m_next)
            {
                // advance to next packet in queue if there is one
                previousPacket = currentPacket;
                currentPacket = currentPacket->m_next;
            }
            else
            {
                // add the packet to the end of the queue
                packet->m_next = NULL;
                currentPacket->m_next = packet;
                //qWarning("RtpReceiver::insert_packet_in_queue INSERTED PACKET at end of queue: sequence number = %u, playtime = %u", packet->m_sequenceNum, packet->m_playoutTime);
                break;
            }
        }
    }
}

qint32 RtpReceiver::adjust_for_clock_skew(RtpPacket* packet)
{
    // based on RTP book p.178
    quint32 delay = packet->m_arrivalTime - packet->m_timestamp;
    //qWarning("RtpReceiver::adjust_for_clock_skew: packet timestamp = %u, arrival time = %u, delay = %u, old m_clockDelayEstimate = %u", packet->m_timestamp, packet->m_arrivalTime, delay, m_clockDelayEstimate);

    // TODO: do any of these need to be unsigned comparisons/operations??

    if (m_clockFirstTime)
    {
        m_clockFirstTime = false;
        m_clockDelayEstimate = delay;
        m_clockActiveDelay = delay;
        return 0;
    }
    else
    {
        m_clockDelayEstimate = (31/32.0) * m_clockDelayEstimate + (delay/32.0); // TODO: change these to multiplications
    }

    qint32 delayDiff = m_clockActiveDelay - m_clockDelayEstimate;
    //qWarning("RtpReceiver::adjust_for_clock_skew: m_clockActiveDelay = %u, m_clockDelayEstimate = %u, delayDiff = %d, ssrc = %u, RTP port = %u, packet queue length = %d", m_clockActiveDelay, m_clockDelayEstimate, delayDiff, m_ssrc, m_portRtp, packet_queue_length());
    if (delayDiff >= m_clockSkewThreshold)
    {
        // sender is fast compared to receiver
        QDateTime currentTime = QDateTime::currentDateTime();
        QString currentTimeString = currentTime.toString();
        QByteArray currentTimeAscii = currentTimeString.toAscii();
        qWarning("[%s] Receiver is slower than sender: compensating for clock skew! ssrc = %u, RTP port = %u, system playtime = %u, packet queue length = %d", currentTimeAscii.data(), m_ssrc, m_portRtp, m_playtime, packet_queue_length());
        m_timestampOffset -= m_clockSkewThreshold;
        m_clockActiveDelay = m_clockDelayEstimate;
        return -m_clockSkewThreshold;
    }
    else if (delayDiff <= -m_clockSkewThreshold)
    {
        // sender is slow compared to receiver
        QDateTime currentTime = QDateTime::currentDateTime();
        QString currentTimeString = currentTime.toString();
        QByteArray currentTimeAscii = currentTimeString.toAscii();
        qWarning("[%s] Receiver is faster than sender: compensating for clock skew! ssrc = %u, RTP port = %u, system playtime = %u, packet queue length = %d", currentTimeAscii.data(), m_ssrc, m_portRtp, m_playtime, packet_queue_length());
        m_timestampOffset += m_clockSkewThreshold;
        m_clockActiveDelay = m_clockDelayEstimate;
        return m_clockSkewThreshold;
    }

    return 0;
}

qint32 RtpReceiver::packet_queue_length()
{
    RtpPacket* currentPacket = m_packetQueue;
    // skip any used packets from the beginning of the queue
    while (currentPacket && currentPacket->m_used)
    {
        currentPacket = currentPacket->m_next;
    }

    qint32 length = 0;
    while (currentPacket)
    {
        length++;
        currentPacket = currentPacket->m_next;

    }

    return length;
}

qint32 RtpReceiver::adjust_for_jitter(RtpPacket* packet)
{
    quint32 transitTime = packet->m_arrivalTime - packet->m_timestamp;
    if (m_jitterFirstTime)
    {
        m_transitTimePrev = transitTime;
        m_jitterFirstTime = false;
    }
    int diff = m_transitTimePrev - transitTime;
    int diff2 = abs(diff) - m_jitter;
    int diff3 = diff2 / 16;
    m_jitter = m_jitter + diff3;
    m_transitTimePrev = transitTime;

    //qDebug("RtpReceiver::adjust_for_jitter: transit time = %u, diff = %d, diff2 = %d, diff3 = %d, jitter = %u", transitTime, diff, diff2, diff3, m_jitter);

    quint32 adjustment = m_packetQueueSize * m_bufferSamples;
    //quint32 tempDiff1 = adjustment - (m_jitter * JITTER_ADJUST_FACTOR);
    //quint32 tempDiff2 = tempDiff1 & 0x80000000;
    //qDebug("RtpReceiver::adjust_for_jitter: tempDiff1 = %u, tempDiff2 = %u", tempDiff1, tempDiff2);
    /*if (((adjustment - (m_jitter * JITTER_ADJUST_FACTOR)) & 0x80000000) != 0) // is adjustment < (m_jitter * JITTER_ADJUST_FACTOR) with unsigned comparison
    {
        adjustment = m_jitter * JITTER_ADJUST_FACTOR;
        qWarning("TOO MUCH JITTER: adjusting playtime by %d", adjustment);
    }*/

    return adjustment;
}

int RtpReceiver::receiveAudio(float** audio, int channels, int frames)
{
    m_playtime = jack_last_frame_time(m_jackClient);
    
    //qWarning("\nRtpReceiver::receiveAudio ssrc = %u, RTP port = %u, system playtime = %u, packet queue length = %d", m_ssrc, m_portRtp, m_playtime, packet_queue_length());
    
    // TODO: check that audio is not NULL?
    
    // ============== START LOCKED SECTION ===============
    QMutexLocker locker(&m_queueMutex);
    RtpPacket* packetQueue = m_packetQueue;
    // skip any used packets from the beginning of the queue
    while (packetQueue && packetQueue->m_used)
    {
        qDebug("RtpReceiver::receiveAudio SKIPPING USED PACKET left in queue: packet playtime = %u, ssrc = %u, RTP port = %d", packetQueue->m_playoutTime, m_ssrc, m_portRtp);
        packetQueue = packetQueue->m_next;
    }
    locker.unlock();
    // =============== END LOCKED SECTION ================

    // get next audio packet

    // TODO: handle case where number of samples in a packet is not the same as JACK buffer size
    if (!packetQueue || (((qint32)m_playtime - (qint32)(packetQueue->m_playoutTime) < 0)))
    {
        m_numMissed++;
        if (!packetQueue && !m_firstPacket)
        {
            qWarning("RtpReceiver::receiveAudio NO AVAILABLE PACKETS: playing silence: playtime = %u, ssrc = %u, RTP port = %d", m_playtime, m_ssrc, m_portRtp);
        }
        else if (packetQueue && m_packetsReceived > m_packetQueueSize) // TODO: how to handle warning if the first couple of packets arrive but not enough to fill queue, then stop??
        {
            if (m_numMissed < 10)
            {
                qWarning("RtpReceiver::receiveAudio %lld MISSING PACKET(S): playing silence: playtime = %u, next packet playtime = %u, ssrc = %u, RTP port = %d", m_numMissed, m_playtime, packetQueue->m_playoutTime, m_ssrc, m_portRtp);
            }
        }

        if ((m_numMissed % 200) == 0)
        {
            qWarning("RtpReceiver::receiveAudio MISSED %lld PACKETS: playing silence: playtime = %u, ssrc = %u, RTP port = %d", m_numMissed, m_playtime, m_ssrc, m_portRtp);
        }

        // output silence
        // TODO: confirm that frames and m_bufferSamples are the same size?
        for (int ch = 0; ch < channels; ch++)
        {
            memcpy(audio[ch], m_zeros, frames * sizeof(float));
        }
    }
    else
    {
        // find last playable packet before playtime, discard any being skipped
        RtpPacket* packet = packetQueue;
        RtpPacket* next = packet->m_next;
        while (next && (((qint32)(next->m_playoutTime) - (qint32)m_playtime) < 0)) //next->m_playoutTime < m_playtime)
        {
            // the next packet in the queue is also playable: skip this one and remove from queue
            // TODO: could do some kind of interpolation to compensate for skipping packets??
            qWarning("RtpReceiver::receiveAudio SKIPPING PACKET: system playtime = %u, skipped packet with playtime = %u, next packet playtime = %u, ssrc = %u, RTP port = %d", m_playtime, packet->m_playoutTime, next->m_playoutTime, m_ssrc, m_portRtp);

            // flag skipped packet for removal from queue (to be done by network thread to avoid conflicts)
            packet->m_used = true;

            // advance to next packet in queue
            packet = next;  
            next = packet->m_next;            
        }

        if (packet && !packet->m_used)
        {
            //qWarning("RtpReceiver::receiveAudio playing packet: system playtime = %u, packet playtime = %u, ssrc = %u, RTP port = %d", m_playtime, packet->m_playoutTime, m_ssrc, m_portRtp);
            m_numMissed = 0;

            // grab audio data from next playable packet
            packet->getPayload(channels, frames, audio);

            // flag used packet for removal from queue (to be done by network thread to avoid conflicts)
            packet->m_used = true;
        }
        else
        {
            qWarning("RtpReceiver::receiveAudio NO UNUSED PACKETS ready to play: playing silence: playtime = %u, ssrc = %u, RTP port = %d", m_playtime, m_ssrc, m_portRtp);
            // output silence
            // TODO: confirm that frames and m_bufferSamples are the same size?
            for (int ch = 0; ch < channels; ch++)
            {
                memcpy(audio[ch], m_zeros, frames * sizeof(float));
            }
        }
    }

    return 0;
}

void RtpReceiver::handleXrun()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString currentTimeString = currentTime.toString();
    QByteArray currentTimeAscii = currentTimeString.toAscii();
    qWarning("[%s] RtpReceiver::handleXrun: ssrc = %u, RTP port = %d", currentTimeAscii.data(), m_ssrc, m_portRtp);

    // TODO: decide how to handle xruns
}

} // end of namespace SAM
