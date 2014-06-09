/**
 * @file rtpsender.cpp
 * RTP sender implementation
 * @author Michelle Daniels
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
