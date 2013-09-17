/**
 * @file rtpsender.cpp
 * RTP sender implementation
 * @author Michelle Daniels
 * @copyright UCSD 2012
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <QDebug>
#include <QTime>
#include <QtEndian>
#include <QUdpSocket>

#include "rtpsender.h"

namespace sam
{

RtpSender::RtpSender(const QString& host, quint16 portRtp, quint16 portRtcpLocal, quint16 portRtcpRemote, int reportInterval, int sampleRate, int channels, int bufferSize, quint32 ssrc, quint8 payloadType, QObject *parent) :
    m_socketRtp(NULL),
    m_remotePortRtp(portRtp),
    m_remotePortRtcp(portRtcpRemote),
    m_sampleRate(sampleRate),
    m_ssrc(ssrc),
    m_payloadType(payloadType),
    m_timestamp(0),
    m_sequenceNum(0),
    m_reportInterval(0),
    m_nextReportTick(0),
    m_packetsSent(0),
    m_octetsSent(0),
    m_rtcpHandler(NULL)
{
    m_socketRtp = new QUdpSocket(this);
    m_socketRtp->bind();

    m_remoteHost.setAddress(host);

    // random initialization of timestamp and sequence number
    m_timestamp = qrand() * 4294967296.0f / RAND_MAX; // random unsigned 32-bit int
    m_sequenceNum = qrand() * 65536.0f / RAND_MAX; // random unsigned 16-bit int
    qDebug("Starting timestamp = %u, starting sequence number = %u", m_timestamp, m_sequenceNum);
     
    // init reporting interval (convert from millis to samples)
    m_reportInterval = (m_sampleRate  * reportInterval) / 1000.0;
    m_nextReportTick = m_timestamp + m_reportInterval;
    
    m_rtcpHandler = new RtcpHandler(portRtcpLocal, ssrc, host, portRtcpRemote, this);
    connect(this, SIGNAL(sendRtcpTick(qint64, quint32, quint32, quint32, quint32)), m_rtcpHandler, SLOT(sendSenderReport(qint64, quint32, quint32, quint32, quint32)));
}

RtpSender::~RtpSender()
{
    // socket will be destroyed by parent
    
    delete m_rtcpHandler;
    m_rtcpHandler = NULL;
    
    m_socketRtp->close();
}

bool RtpSender::init()
{
    // start RTCP receiver
    return m_rtcpHandler->start();
}

bool RtpSender::sendAudio(int numChannels, int numSamples, float** data)
{
    // construct RTP packet
    m_packet.init(m_timestamp, m_sequenceNum, m_payloadType, m_ssrc);
    m_packet.setPayload(numChannels, numSamples, data);
    //qDebug() <<  "RtpSender::sendAudio timestamp = " << m_timestamp << " samples, sequence number = " << m_sequenceNum << endl;

    m_timestamp += numSamples;
    m_sequenceNum++;

    // send RTP packet
    m_packetData.clear();
    m_packet.write(m_packetData);
    if (m_socketRtp->writeDatagram(m_packetData, m_remoteHost, m_remotePortRtp) < 0)
    {
        qWarning("RtpSender::sendAudio couldn't write datagram");
        return false;
    }
    
    // check for sending RTCP report
    if (((m_timestamp - m_nextReportTick) & 0x80000000) == 0) // is offset >= m_timestampOffset with unsigned comparison
    {
        QDateTime currentTime = QDateTime::currentDateTimeUtc();
        qint64 currentTimeMillis = currentTime.toMSecsSinceEpoch();
        quint32 currentTimeSecs = currentTimeMillis / 1000;
        emit sendRtcpTick(currentTimeMillis, currentTimeSecs, m_timestamp, m_packetsSent, m_octetsSent);
        m_nextReportTick += m_reportInterval;
    }
    
    m_packetsSent++;
    m_octetsSent += m_packet.m_payload.length();

    return true;
}

} // end of namespace SAM
