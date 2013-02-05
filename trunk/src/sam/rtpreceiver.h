#ifndef RTPRECEIVER_H
#define RTPRECEIVER_H

#include <QElapsedTimer>
#include <QMutex>
#include <QUdpSocket>

#include "jack/jack.h"

#include "rtcp.h"
#include "rtp.h"

class RtpReceiver : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    RtpReceiver(quint16 portRtp, quint16 portRtcpLocal, quint16 portRtcpRemote, quint32 reportInterval, quint32 ssrc, qint32 sampleRate, qint32 bufferSize, QObject *parent = 0);

    /**
     * Destructor.
     */
    virtual ~RtpReceiver();

    /**
     * Copy constructor (not used).
     */
    RtpReceiver(const RtpReceiver&);

    /**
     * Assignment operator (not used).
     */
    RtpReceiver& operator=(const RtpReceiver);

    /**
     * Get the receiver's RTP port.
     * @return the receiver's RTP port
     */
    quint16 getPortRtp() const { return m_portRtp; }

    bool start();

     /**
     * Return audio data.
     * @param audio pointer to pre-allocated arrays of samples
     * @param channels number of audio channels
     * @param frames the number of audio frames
     */
    int receiveAudio(float** audio, int channels, int frames);
    
signals:
    void receiverReportReady(quint32 senderSsrc, qint64 firstSeqNumThisInt, qint64 maxSeqNumThisInt, quint64 packetsThisInt, quint32 firstSeqNum, quint64 maxExtendedSeqNum, quint64 packets, quint32 jitter, quint32 lastSenderTimestamp, qint64 delayMillis);

protected slots:

    /**
     * Read a newly-received UDP datagram.
     */
    void readPendingDatagramsRtp();
    
    void sendRtcpReport();
    
    void handleSenderReport(quint32 lastSenderTimestamp);

protected:

    RtpPacket* read_datagram();
    
    quint32 update_timestamp_offset(RtpPacket* packet);
    
    void reset_timestamp_offset(qint32 diff);
    
    void init_stats(RtpPacket* packet, quint32 currentOffset);
    
    bool set_extended_seq_num(RtpPacket* packet, quint32 currentOffset);
    
    void insert_packet_in_queue(RtpPacket* packet);
            
    qint32 adjust_for_clock_skew(RtpPacket* packet);

    qint32 adjust_for_jitter(RtpPacket* packet);

    QUdpSocket* m_socketRtp;    ///< The socket receiving incoming RTP UDP datagrams
    quint16 m_portRtp;          ///< The port to listen on
    quint16 m_remotePortRtcp;
    QElapsedTimer m_timer;
    QElapsedTimer m_reportTimer;
    quint32 m_ssrc;
    QHostAddress m_sender;
    quint32 m_senderSsrc;

    quint32 m_playtime;
    quint32 m_timestampOffset;
    qint32 m_sampleRate;
    
    bool m_firstPacket;
    quint16 m_sequenceMax;
    quint32 m_sequenceWrapCount;
    quint16 m_badSequence;
    quint16 m_numLate;
    qint64 m_numMissed;

    RtpPacket* m_packetQueue;

    qint32 m_bufferSamples;

    // for clock skew estimates
    bool m_clockFirstTime;
    quint32 m_clockDelayEstimate;
    quint32 m_clockActiveDelay;

    // for jitter estimates
    bool m_jitterFirstTime;
    quint32 m_transitTimePrev;
    quint32 m_jitter;

    // other stats
    quint64 m_maxExtendedSeqNum;
    quint64 m_maxSeqNumThisInt;
    quint32 m_firstSeqNum;
    quint64 m_firstSeqNumThisInt;
    quint64 m_packetsReceived;
    quint64 m_packetsReceivedThisInt;
    quint32 m_reportInterval; // in millis
    quint32 m_lastSenderTimestamp;
    
    RtcpHandler* m_rtcpHandler;
    
    QMutex m_queueMutex;
};

#endif // RTPRECEIVER_H
