#ifndef RTPSENDER_H
#define RTPSENDER_H

#include <QElapsedTimer>
#include <QThread>
#include <QUdpSocket>

#include "rtcp.h"
#include "rtp.h"

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

    bool init();
    
    void forceTimestamp(quint32 timestamp) { m_timestamp = timestamp; }

    void forceSequenceNum(quint16 n) { m_sequenceNum = n; }
    
    bool sendAudio(int numChannels, int numSamples, float** data);

signals:
    void sendRtcpTick(qint64 currentTimeMillis, quint32 currentTimeSecs, quint32 timestamp, quint32 packetsSent, quint32 octetsSent);
    
protected:

    void send_rtcp_report();
    
    QUdpSocket* m_socketRtp;
    QHostAddress m_remoteHost;
    quint16 m_remotePortRtp;
    quint16 m_remotePortRtcp;
    qint32 m_sampleRate;
    quint32 m_ssrc;
    quint8 m_payloadType;
    quint32 m_timestamp;
    quint16 m_sequenceNum;
    QByteArray m_packetData;
    RtpPacket m_packet;

    quint32 m_reportInterval;// in millis
    quint32 m_nextReportTick;// in millis
    
    quint32 m_packetsSent;
    quint32 m_octetsSent;
    
    RtcpHandler* m_rtcpHandler;
};

#endif // RTPSENDER_H
