/**
 * @file rtpsender.h
 * RTP sender interface
 * @author Michelle Daniels
 * @copyright UCSD 2012
 */

#ifndef RTPSENDER_H
#define RTPSENDER_H

#include "rtcp.h"
#include "rtp.h"

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
