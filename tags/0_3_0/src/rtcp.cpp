/**
 * @file rtcp.cpp
 * Implementation of RTCP functionality
 * @author Michelle Daniels
 * @date June 2012
 * @copyright UCSD 2012
 */

#include <QDataStream>
#include <QUdpSocket>
#include <QtEndian>
#include <QDebug>
#include "rtcp.h"

/* ----- RtcpHandler implementation ----- */
RtcpHandler::RtcpHandler(quint16 localPort, quint32 ssrc, const QString& remoteAddress, quint16 remotePort, QObject* parent) :
    QObject(parent),
    m_socket(NULL),
    m_localPort(localPort),
    m_ssrc(ssrc),
    m_remotePort(remotePort)
{
    m_remoteHost.setAddress(remoteAddress);
}

RtcpHandler::~RtcpHandler() 
{
    if (m_socket) m_socket->close();
    // the socket will be freed by its parent so we don't have to do anything
}

bool RtcpHandler::start()
{
    // init RTCP socket
    m_socket = new QUdpSocket(this);
    if (!m_socket->bind(m_localPort))
    {
        qWarning("RtcpHandler::start RTCP socket couldn't bind to port %d: %s", m_localPort, m_socket->errorString().toAscii().data());
        return false;
    }
    else
    {
        qDebug("RtcpHandler::start RTCP socket binded to port %d", m_localPort);
    }
    
    // start receiving packets
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    
    return true;
}

void RtcpHandler::sendSenderReport(qint64 currentTimeMillis, quint32 currentTimeSecs, quint32 timestamp, quint32 packetsSent, quint32 octetsSent)
{
    if (m_remoteHost.isNull())
    {
        qWarning("RtcpHandler::sendSenderReport NOT sending RTCP sender report: remote host is NULL!  ssrc = %u, local port = %u", m_ssrc, m_localPort);
        return;
    }
    qDebug("RtcpHandler::sendSenderReport SENDING RTCP SENDER REPORT");
    
    QByteArray data; // TODO: don't allocate this new each time?
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // write version + report count
    quint8 versionByte = 128; // set version to be 2 (uppermost 2 bits), report count to be 0 (lower 5 bits)
    stream << versionByte;

    // write packet type
    quint8 typeByte = RTCP_SR_PACKET_TYPE;
    stream << typeByte;

    // write length (number of 32-bit words following initial 32-bit common header)
    quint16 length = 6;
    stream << length;

    // write reporter ssrc
    stream << m_ssrc;
    
    // write NTP timestamp (64 bits)
    quint32 millisDiff = currentTimeMillis - (currentTimeSecs * 1000);
    float currentTimeMillisFloat = millisDiff / (float)1000.0f;
    quint32 currentTimeMillisFixed = currentTimeMillisFloat * 4294967296; // convert from [0.0, 1.0] to [0 2^32]
    currentTimeSecs += 2208988800; // add 2,208,988,800 to convert seconds since 1970 to seconds since 1900
    stream << currentTimeSecs; 
    stream << currentTimeMillisFixed;
    qDebug("RtcpHandler::sendSenderReport NTP timestamp seconds = %u, millis float = %0.4f, fixed = %u", currentTimeSecs, currentTimeMillisFloat, currentTimeMillisFixed);
    
    // write rtp timestamp (32 bits) // TODO: this should be actual media clock time synchronized with NTP time above
    stream << timestamp;
    qDebug("RtcpHandler::sendSenderReport RTP timestamp = %u", timestamp);
    
    // write packets sent
    stream << packetsSent;
    
    // write octets sent
    stream << octetsSent;
    
    qDebug("RtcpHandler::sendSenderReport %u packets sent containing %u payload octets total", packetsSent, octetsSent);
    
    qDebug("RtcpHandler::sendSenderReport sending packet with length %d bytes", data.length());
    
    // send packet
    if (m_socket->writeDatagram(data, m_remoteHost, m_remotePort) < 0)
    {
        qWarning("RtcpHandler::sendSenderReport couldn't write datagram");
    }
}

void RtcpHandler::sendReceiverReport(quint32 senderSsrc, qint64 firstSeqNumThisInt, qint64 maxSeqNumThisInt, quint64 packetsThisInt, quint32 firstSeqNum, quint64 maxExtendedSeqNum, quint64 packets, quint32 jitter, quint32 lastSenderTimestamp, qint64 delayMillis)
{
    if (m_remoteHost.isNull())
    {
        qWarning("RtcpHandler::sendReceiverReport NOT sending RTCP receiver report: remote host is NULL!  ssrc = %u, local port = %u", m_ssrc, m_localPort);
        return;
    }
    //qDebug("RtcpHandler::sendReceiverReport SENDING RTCP RECEIVER REPORT");
    
    QByteArray data; // TODO: don't allocate this new each time?
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // write version + report count
    quint8 versionByte = 128 | 1; // set version to be 2 (uppermost 2 bits), report count to be 1 (lower 5 bits)
    stream << versionByte;

    // write packet type
    quint8 typeByte = RTCP_RR_PACKET_TYPE;
    stream << typeByte;

    // write length (number of 32-bit words)
    quint16 length = 7;
    stream << length;

    // write reporter ssrc
    stream << m_ssrc;
    
    // write reportee ssrc
    stream << senderSsrc;
    
    // write loss fraction (for this interval)
    qint64 packetsExpectedThisInt = maxSeqNumThisInt - firstSeqNumThisInt;
    qint32 packetsLostThisInt = packetsExpectedThisInt - packetsThisInt;
    double lossFraction = (packetsExpectedThisInt == 0) ? 0 : packetsLostThisInt / (double)packetsExpectedThisInt;
    quint8 lossFractionFixed = (quint8)(lossFraction * 256.0);
    stream << lossFractionFixed;
    //qDebug() << "RtcpHandler::sendReceiverReport packets loss fraction = " << lossFraction << ", fixed point = " << lossFractionFixed;
    
    // write cumulative packets lost (24 bits)
    qint64 packetsExpected = maxExtendedSeqNum - firstSeqNum;
    qint32 packetsLost = packetsExpected - packets;
    //qDebug() << "RtcpHandler::sendReceiverReport packets expected = " << packetsExpected << ", packets received = " << packets << ", packets lost = " << packetsLost;
    packetsLost &= 0xffffff; // get lower 24 bits (will preserve sign due to two's complement notation?)
    uchar packetsLostBytes[4];
    qToBigEndian(packetsLost, packetsLostBytes); // convert to network byte order
    stream << packetsLostBytes[1];
    stream << packetsLostBytes[2];
    stream << packetsLostBytes[3];
    
    // write extended highest sequence number
    quint32 maxExtendedSeqNumTrunc = maxExtendedSeqNum; // truncate to 32 bits
    stream << maxExtendedSeqNumTrunc;
    
    // write jitter
    stream << jitter;
    
    //qDebug("RtcpHandler::sendReceiverReport jitter = %u, max extended sequence number = %u", jitter, maxExtendedSeqNumTrunc);
    
    // write last sender report timestamp
    stream << lastSenderTimestamp;
    
    // write last sender report delay
    quint32 lastSenderDelay = (delayMillis * 65536) / 1000; // units of 1/65536 seconds
    stream << lastSenderDelay;
    
    //qDebug() << "RtcpHandler::sendReceiverReport last sender timestamp = " << lastSenderTimestamp << ", last sender delay in millis = " << delayMillis << ", last sender delay = " << lastSenderDelay;
    
    //qDebug() << "RtcpHandler::sendReceiverReport sending packet with length " << data.length() << " bytes, to host " << m_remoteHost << ", port " << m_remotePort;
    
    // send packet
    if (m_socket->writeDatagram(data, m_remoteHost, m_remotePort) < 0)
    {
        qWarning("RtcpHandler::sendReceiverReport couldn't write datagram: error %d: %s", m_socket->error(), m_socket->errorString().toAscii().data());
    }
}

void RtcpHandler::readPendingDatagrams()
{
    while (m_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        //qDebug("\nDATAGRAM RECEIVED on RTCP port");
        
        // TODO: validate RTCP packet
        QDataStream stream(&datagram, QIODevice::ReadOnly);
        stream.setByteOrder(QDataStream::BigEndian);

        // read version + report count
        quint8 versionByte = 0;
        stream >> versionByte;
        quint8 version = versionByte >> 6; // version is upper 2 bits
        quint8 padding = (versionByte & 32) >> 5; // padding flag is 3rd uppermost bit
        quint8 reportCount = versionByte & 31; // report count is lowest 5 bits
        
        //qDebug("RtcpHandler::readPendingDatagrams: version = %u, padding = %u, reportCount = %u", version, padding, reportCount);
        // TODO: handle padding and report count?
        
        if (version != 2)
        {
            qWarning("RtcpHandler::readPendingDatagrams: received INVALID RTCP packet: expected version 2, got version %d", version);
            break;
        }
                
        // read packet type
        quint8 typeByte = 0;
        stream >> typeByte;
        
        switch (typeByte)
        {
        case RTCP_RR_PACKET_TYPE:
            //qDebug("RtcpHandler::readPendingDatagrams: received RTCP RECEIVER REPORT");
            read_receiver_report(stream);
            break;
        case RTCP_SR_PACKET_TYPE:
            //qDebug("RtcpHandler::readPendingDatagrams: received RTCP SENDER REPORT");
            read_sender_report(stream);
            break;
        default:
            qWarning("RtcpHandler::readPendingDatagrams: received UNRECOGNIZED RTCP packet type:%d", typeByte);
            break;
        }
    }
}

void RtcpHandler::read_receiver_report(QDataStream& stream)
{
    // TODO: gracefully handle malformed packet (not enough data to read), or validate datagram size before reading
    
    // read length (number of 32-bit words)
    quint16 length = 0;
    stream >> length;
    if (length != 7)
    {
        qWarning("RtcpHandler::read_receiver_report received INVALID RTCP packet: expected length 7, got length %d", length);
        return;
    }

    // read reporter ssrc
    stream >> m_receiverReport.reporterSsrc;

    // read reportee ssrc
    stream >> m_receiverReport.reporteeSsrc;

    /*if (reporteeSsrc != m_ssrc)
    {
        qWarning("RtcpHandler::read_receiver_report IGNORING RTCP receiver report about a different reportee.  Reportee SSRC = %u, our SSRC = %u", reporteeSsrc, m_ssrc);
        break;
    }*/

    qDebug("RtcpHandler::read_receiver_report receiver report from SSRC %u", m_receiverReport.reporterSsrc);

    // read loss fraction (for this interval)
    stream >> m_receiverReport.lossFraction;
    float lossFraction = m_receiverReport.lossFraction / 256.0f; // TODO: make this a multiplication for efficiency
    qDebug("RtcpHandler::read_receiver_report packet loss fraction (this interval) = %0.4f", lossFraction);

    // read cumulative packets lost (24 bits)
    quint8 packetsLostBytes[4];
    packetsLostBytes[0] = 0;
    stream >> packetsLostBytes[1];
    stream >> packetsLostBytes[2];
    stream >> packetsLostBytes[3];
    qint32* packetsLostPtr = reinterpret_cast<qint32*>(packetsLostBytes);
    qint32 packetsLost = qFromBigEndian(*packetsLostPtr);
    if (packetsLost & 0x800000) packetsLost = (packetsLost | 0xFF000000); // restore sign
    m_receiverReport.packetsLost = packetsLost;
    
    qDebug("RtcpHandler::read_receiver_report cumulative packets lost = %d", packetsLost);

    // read extended highest sequence number
    stream >> m_receiverReport.maxExtendedSeqNum;
    qDebug("RtcpHandler::read_receiver_report extended highest sequence number received = %u", m_receiverReport.maxExtendedSeqNum);

    // read jitter
    stream >> m_receiverReport.jitter;
    qDebug("RtcpHandler::read_receiver_report jitter = %u", m_receiverReport.jitter);

    // read last sender report timestamp
    stream >> m_receiverReport.lastSenderTimestamp;

    // read last sender report delay
    stream >> m_receiverReport.lastSenderDelay;

    qDebug("RtcpHandler::read_receiver_report last sender timestamp = %u, last sender delay = %u", m_receiverReport.lastSenderTimestamp, m_receiverReport.lastSenderDelay);

    emit receiverReportReceived();
}

void RtcpHandler::read_sender_report(QDataStream& stream)
{
    // TODO: gracefully handle malformed packet (not enough data to read), or validate datagram size before reading
    
    // read length
    // TODO: validate length?
    quint16 length = 0;
    stream >> length;
    if (length != 6)
    {
        qWarning("RtcpHandler::read_sender_report received INVALID RTCP packet: expected length 6, got length %d", length);
        return;
    }

    // read reporter ssrc
    stream >> m_senderReport.reporterSsrc;
    //qDebug("RtcpHandler::read_sender_report received sender report from SSRC %u, length = %u", m_senderReport.reporterSsrc, length);

    // read NTP timestamp (64 bits)
    stream >> m_senderReport.ntpSeconds; 
    stream >> m_senderReport.ntpMillis;
    float ntpMillis = m_senderReport.ntpMillis / (float)4294967296.0f; // convert from [0 2^32]to [0.0, 1.0]
    //qDebug("RtcpHandler::read_sender_report NTP timestamp seconds = %u, millis float = %0.4f, fixed = %u", m_senderReport.ntpSeconds, ntpMillis, m_senderReport.ntpMillis);

    // read rtp timestamp (32 bits)
    stream >> m_senderReport.rtpTimestamp;
    //qDebug("RtcpHandler::read_sender_report RTP timestamp = %u", m_senderReport.rtpTimestamp);

    // read packets sent
    stream >> m_senderReport.packetsSent;

    // read octets sent
    stream >> m_senderReport.octetsSent;

    //qDebug("RtcpHandler::read_sender_report packets sent = %u, octets sent = %u", m_senderReport.packetsSent, m_senderReport.octetsSent);

    quint32 lastSenderTimestamp = (( m_senderReport.ntpSeconds & 0xFF00) << 16) | ( m_senderReport.ntpMillis >> 16); // middle 32 bits from NTP timestamp

    emit senderReportReceived(lastSenderTimestamp);
}
