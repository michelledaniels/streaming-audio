/**
 * @file rtpsender.h
 * RTP sender interface
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

#ifndef RTPSENDER_H
#define RTPSENDER_H

#include "../rtcp.h"
#include "../rtp.h"

class QUdpSocket;

namespace sam
{

/**
 * @class RtpSender
 * @author Michelle Daniels
 * @date 2012
 *
 * An RtpSender sends RTP packets to an RtpReceiver
 */
class RtpSender : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    RtpSender(const QString& host, quint16 portRtp, quint16 portRtcpLocal, quint16 portRtcpRemote, int reportInterval, int sampleRate, int channels, int bufferSize, quint32 ssrc, quint8 payloadType, QObject *parent = 0);

    /**
     * Destructor.
     */
    virtual ~RtpSender();

    /**
     * Copy constructor (not used).
     */
    RtpSender(const RtpSender&);

    /**
     * Assignment operator (not used).
     */
    RtpSender& operator=(const RtpSender);

    /**
     * Initialize this sender.
     * @return true on success, false on failure
     */
    bool init();
    
    /**
     * Force next packet sent to have the given timestamp (debugging tool).
     * @param timestamp timestamp to force
     */
    void forceTimestamp(quint32 timestamp) { m_timestamp = timestamp; }


    /**
     * Force next packet sent to have the given sequence number (debugging tool).
     * @param n sequence number to force
     */
    void forceSequenceNum(quint16 n) { m_sequenceNum = n; }
    
    /**
     * Send the given audio buffer.
     * @param numChannels number of audio channels to send
     * @param numSamples number of samples to send (per channel)
     * @param data audio sample data in the form data[channel][sample]
     * @return true on success, false on failure
     */
    bool sendAudio(int numChannels, int numSamples, float** data);

signals:
    /**
     * Signal that a RTCP sender report is ready to be sent.
     * @param currentTimeMillis millisecond part of current time
     * @param currentTimeSecs second part of current time
     * @param timestamp current RTP timestamp
     * @param packetsSent number of RTP packets sent since sender started sending
     * @param octetsSent number of bytes sent since sender started sending
     */
    void sendRtcpTick(qint64 currentTimeMillis, quint32 currentTimeSecs, quint32 timestamp, quint32 packetsSent, quint32 octetsSent);
    
protected:

    QUdpSocket* m_socketRtp;    ///< UDP socket used to send packets
    QHostAddress m_remoteHost;  ///< address of remote host (receiver)
    quint16 m_remotePortRtp;    ///< RTP port for receiver
    quint16 m_remotePortRtcp;   ///< RTCP port for receiver
    qint32 m_sampleRate;        ///< audio sample rate
    quint32 m_ssrc;             ///< sender SSRC
    quint8 m_payloadType;       ///< audio payload type
    quint32 m_timestamp;        ///< current packet timestamp
    quint16 m_sequenceNum;      ///< current packet sequence number
    QByteArray m_packetData;    ///< bytes of RTP packet to be sent
    RtpPacket m_packet;         ///< RTP packet to be sent

    quint32 m_reportInterval;   ///< milliseconds between RTCP sender reports
    quint32 m_nextReportTick;   ///< timestamp when next sender report should be sent (in milliseconds)
    
    quint32 m_packetsSent;      ///< total number of packets sent
    quint32 m_octetsSent;       ///< total number of bytes sent
    
    RtcpHandler* m_rtcpHandler; ///< RTCP report handler
};

} // end of namespace SAM

#endif // RTPSENDER_H
