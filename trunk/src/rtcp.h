/**
 * @file rtcp.h
 * RTCP functionality
 * @author Michelle Daniels
 * @date June 2012
 * @copyright UCSD 2012
 */

#ifndef RTCP_H
#define RTCP_H

#include <QObject>
#include <QHostAddress>

static const quint8 RTCP_SR_PACKET_TYPE = 200; // sender report packet type
static const quint8 RTCP_RR_PACKET_TYPE = 201; // receiver report packet type

/**
 * @struct RtcpHandlerReport
 * An RtcpHandlerReport encapsulates an RTCP receiver report.
 */
struct RtcpHandlerReport // TODO: should be RtcpReceiverReport??
{
    quint32 reporterSsrc;           ///< SSRC of reporter (receiver)
    quint32 reporteeSsrc;           ///< SSRC of reportee (sender)
    quint8 lossFraction;            ///< fraction of packets lost during reporting interval
    qint32 packetsLost;             ///< cumulative number of packets lost
    quint32 maxExtendedSeqNum;      ///< highest extended sequence number received
    quint32 jitter;                 ///< interarrival jitter estimate
    quint32 lastSenderTimestamp;    ///< timestamp of last received sender report
    quint32 lastSenderDelay;        ///< delay since last received sender report
};

/**
 * @struct RtcpSenderReport
 * An RtcpSenderReport encapsulates an RTCP sender report.
 */
struct RtcpSenderReport
{
    quint32 reporterSsrc;   ///< SSRC of reporter (sender)
    quint32 ntpSeconds;     ///< timestamp seconds (in NTP format)
    quint32 ntpMillis;      ///< timestamp milliseconds (in NTP format)
    quint32 rtpTimestamp;   ///< RTP timestamp
    quint32 packetsSent;    ///< cumulative number of packets sent by this sender
    quint32 octetsSent;     ///< cumulative number of bytes sent by this sender
};

class QUdpSocket;

/**
 * @class RtcpHandler
 * @author Michelle Daniels
 * @date 2012
 *
 * An RtcpHandler handles RTCP sender and receiver reports.
 */
class RtcpHandler : public QObject
{
    Q_OBJECT
public:

    /**
     * Constructor.
     */
    RtcpHandler(quint16 localPort, quint32 ssrc, const QString& remoteAddress, quint16 remotePort, QObject* parent = 0);

    /**
     * Destructor.
     */
    virtual ~RtcpHandler();

    /**
     * Copy constructor (not used).
     */
    RtcpHandler(const RtcpHandler&);

    /**
     * Assignment operator (not used).
     */
    RtcpHandler& operator=(const RtcpHandler);

    /**
     * Start this RTCP handler.
     * @return true on success, false on failure
     */
    bool start();
    
    /**
     * Set the remote host
     * @param host the remote host address
     */
    void setRemoteHost(QHostAddress& host) { m_remoteHost = host; }
    
signals:
    /**
     * Signal when a receiver report is received.
     */
    void receiverReportReceived();

    /**
     * Signal when a sender report is received.
     */
    void senderReportReceived(quint32 lastSenderTimestamp);
    
protected slots:

    /**
     * Read pending datagrams.
     */
    void readPendingDatagrams();

    /**
     * Send a sender report.
     * @param currentTimeMillis millisecond part of current time
     * @param currentTimeSecs second part of current time
     * @param timestamp current RTP timestamp
     * @param packetsSent number of RTP packets sent since sender started sending
     * @param octetsSent number of bytes sent since sender started sending
     */
    void sendSenderReport(qint64 currentTimeMillis, quint32 currentTimeSecs, quint32 timestamp, quint32 packetsSent, quint32 octetsSent);
    
    /**
     * Send a receiver report.
     * @param senderSsrc the SSRC of the source this receiver is receiving from
     * @param firstSeqNumThisInt sequence number of first packet received during this reporting interval
     * @param maxSeqNumThisInt maximum sequence number received during this reporting interval
     * @param packetsThisInt total number of packets received during this reporting interval
     * @param firstSeqNum sequence number of first packet received from this receiver's source
     * @param maxExtendedSeqNum the maximum extended sequence number received from this receiver's source
     * @param packets total number of packets received from this receiver's source
     * @param jitter current jitter measure for this receiver's source
     * @param lastSenderTimestamp timestamp of last packet received from this receiver's source
     * @param delayMillis number of milliseconds since last sender report was received from this receiver's source
     */
    void sendReceiverReport(quint32 senderSsrc, qint64 firstSeqNumThisInt, qint64 maxSeqNumThisInt, quint64 packetsThisInt, quint32 firstSeqNum, quint64 maxExtendedSeqNum, quint64 packets, quint32 jitter, quint32 lastSenderTimestamp, qint64 delayMillis);

protected:
    
    /**
     * Read a receiver report.
     * @param stream the data stream to read the report from
     */
    void read_receiver_report(QDataStream& stream);

    /**
     * Read a sender report.
     * @param stream the data stream to read the report from
     */
    void read_sender_report(QDataStream& stream);
    
    QUdpSocket* m_socket;               ///< UDP socket used to send and receive RTCP packets
    quint16 m_localPort;                ///< port number on which to listen for RTCP packets
    quint32 m_ssrc;                     ///< SSRC of this sender or receiver
    QHostAddress m_remoteHost;          ///< address of remote host to send reports to
    quint16 m_remotePort;               ///< port of remote host to send reports to
    
    RtcpHandlerReport m_receiverReport; ///< current RTCP receiver report
    RtcpSenderReport m_senderReport;    ///< current RTCP sender report
    
};

#endif // RTCP_H
