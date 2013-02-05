/**
 * @file rtcp.h
 * RTCP functionality
 * Michelle Daniels  June 2012
 * Copyright UCSD 2012
 */

#ifndef RTCP_H
#define RTCP_H

#include <QObject>
#include <QHostAddress>

static const quint8 RTCP_SR_PACKET_TYPE = 200; // sender report packet type
static const quint8 RTCP_RR_PACKET_TYPE = 201; // receiver report packet type

struct RtcpHandlerReport
{
    quint32 reporterSsrc;
    quint32 reporteeSsrc;
    quint8 lossFraction;
    qint32 packetsLost;
    quint32 maxExtendedSeqNum;
    quint32 jitter;
    quint32 lastSenderTimestamp;
    quint32 lastSenderDelay;
};

struct RtcpSenderReport
{
    quint32 reporterSsrc;
    quint32 ntpSeconds;
    quint32 ntpMillis;
    quint32 rtpTimestamp;
    quint32 packetsSent;
    quint32 octetsSent;
};

class QUdpSocket;

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

    bool start();
    
    void setRemoteHost(QHostAddress& host) { m_remoteHost = host; }
    
signals:
    void receiverReportReceived();
    void senderReportReceived(quint32 lastSenderTimestamp);
    
protected slots:

    void readPendingDatagrams();

    void sendSenderReport(qint64 currentTimeMillis, quint32 currentTimeSecs, quint32 timestamp, quint32 packetsSent, quint32 octetsSent);
    
    void sendReceiverReport(quint32 senderSsrc, qint64 firstSeqNumThisInt, qint64 maxSeqNumThisInt, quint64 packetsThisInt, quint32 firstSeqNum, quint64 maxExtendedSeqNum, quint64 packets, quint32 jitter, quint32 lastSenderTimestamp, qint64 delayMillis);

protected:
    
    void read_receiver_report(QDataStream& stream);
    void read_sender_report(QDataStream& stream);
    
    QUdpSocket* m_socket;
    quint16 m_localPort;
    quint32 m_ssrc;
    QHostAddress m_remoteHost;
    quint16 m_remotePort;
    
    RtcpHandlerReport m_receiverReport;
    RtcpSenderReport m_senderReport;
    
};

#endif // RTCP_H
