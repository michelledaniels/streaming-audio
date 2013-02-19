/**
 * @file rtpreceiver.h
 * RTP receiver interface
 * @author Michelle Daniels
 * @copyright UCSD 2012-2013
 */

#ifndef RTPRECEIVER_H
#define RTPRECEIVER_H

#include <QElapsedTimer>
#include <QMutex>
#include <QUdpSocket>

#include "jack/jack.h"

#include "rtcp.h"
#include "rtp.h"

/**
 * @class RtpReceiver
 * @author Michelle Daniels
 * @date 2012
 *
 * An RtpReceiver receives RTP packets from an RtpSender
 */
class RtpReceiver : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    RtpReceiver(quint16 portRtp, quint16 portRtcpLocal, quint16 portRtcpRemote, quint32 reportInterval, quint32 ssrc, qint32 sampleRate, qint32 bufferSize, quint32 playqueueSize, QObject *parent = 0);

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

    /**
     * Start receiving packets.
     * @return true on success, false on failure
     */
    bool start();

     /**
     * Return audio data.
     * @param audio pointer to pre-allocated arrays of samples
     * @param channels number of audio channels
     * @param frames the number of audio frames
     */
    int receiveAudio(float** audio, int channels, int frames);
    
signals:

    /**
     * Signaled when a receiver report is ready to be read.
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
    void receiverReportReady(quint32 senderSsrc, qint64 firstSeqNumThisInt, qint64 maxSeqNumThisInt, quint64 packetsThisInt, quint32 firstSeqNum, quint64 maxExtendedSeqNum, quint64 packets, quint32 jitter, quint32 lastSenderTimestamp, qint64 delayMillis);

protected slots:

    /**
     * Read a newly-received UDP datagram.
     */
    void readPendingDatagramsRtp();
    
    /**
     * Send an RTCP receiver report.
     */
    void sendRtcpReport();
    
    /**
     * Handle a received RTCP sender report.
     * @param lastSenderTimestamp timestamp when sender report was received
     */
    void handleSenderReport(quint32 lastSenderTimestamp);

protected:

    /**
     * Read a UDP datagram.
     * @return RTP packet read or NULL if no packet read.
     */
    RtpPacket* read_datagram();
    
    /**
     * Update the timestamp offset between sender and receiver.
     * @param packet current RTP packet
     * @return timestamp offset for current packet
     */
    quint32 update_timestamp_offset(RtpPacket* packet);
    
    /**
     * Set the timestamp offset to the current offset.
     * @param diff difference to be added to previous offset
     */
    void reset_timestamp_offset(qint32 diff);
    
    /**
     * Initialize all receiver stats.
     * @param packet current packet
     * @param currentOffset timestamp offset for current packet
     */
    void init_stats(RtpPacket* packet, quint32 currentOffset);
    
    /**
     * Set the extended sequence number for the given packet.
     * @param packet current packet
     * @param currentOffset timestamp offset for current packet
     * @return true on success, false on failure (packet badly misordered so extended sequence number could not be estimated).
     */
    bool set_extended_seq_num(RtpPacket* packet, quint32 currentOffset);
    
    /**
     * Insert a packet in the packet queue.
     * @param packet packet to insert
     */
    void insert_packet_in_queue(RtpPacket* packet);
            
    /**
     * Adjust play time based on clock skew estimate.
     * @param packet current packet
     * @return amount of play time adjustment needed
     */
    qint32 adjust_for_clock_skew(RtpPacket* packet);

    /**
     * Adjust play time based on jitter estimate.
     * @param packet current packet
     * @return amount of play time adjustment needed
     */
    qint32 adjust_for_jitter(RtpPacket* packet);

    QUdpSocket* m_socketRtp;        ///< The socket receiving incoming RTP UDP datagrams
    quint16 m_portRtp;              ///< The port to listen on
    quint16 m_remotePortRtcp;       ///< The remote port to send RTCP packets to
    QElapsedTimer m_timer;          ///< Timer for timestamping incoming packets
    QElapsedTimer m_reportTimer;    ///< Timer for measuring time since last sender report was received
    quint32 m_ssrc;                 ///< This receiver's SSRC
    QHostAddress m_sender;          ///< Sender's host address
    quint32 m_senderSsrc;           ///< Sender's SSRC

    quint32 m_playtime;             ///< current play time
    quint32 m_timestampOffset;      ///< estimate of offset between local timestamp and sender's timestamps
    qint32 m_sampleRate;            ///< playback sample rate
    
    bool m_firstPacket;             ///< flag: true if packet received is the first packet, false if first packet was previously received
    quint16 m_sequenceMax;          ///< maximum sequence number received
    quint32 m_sequenceWrapCount;    ///< number of times sequence number has wrapped around since first packet received
    quint16 m_badSequence;          ///< count of number of packets received with bad sequence numbers
    quint16 m_numLate;              ///< number of consecutive late packets received
    qint64 m_numMissed;             ///< number of consecutive missing packets

    RtpPacket* m_packetQueue;       ///< linked list/queue of packets received

    qint32 m_bufferSamples;         ///< audio buffer size for playback
    quint32 m_packetQueueSize;      ///< size of packet queue

    // for clock skew estimates
    bool m_clockFirstTime;          ///< flag: true if this is the initial estimate, false otherwise
    quint32 m_clockDelayEstimate;   ///< current delay estimate
    quint32 m_clockActiveDelay;     ///< delay estimate when last adjustment was made

    // for jitter estimates
    bool m_jitterFirstTime;         ///< flag: true if this is the initial estimate, false otherwise
    quint32 m_transitTimePrev;      ///< estimated transit time of previous packet
    quint32 m_jitter;               ///< jitter estimate

    // other stats
    quint64 m_maxExtendedSeqNum;        ///< max extended sequence number received
    quint64 m_maxSeqNumThisInt;         ///< max extended sequence number received since last RTCP report was sent
    quint32 m_firstSeqNum;              ///< first sequence number received
    quint64 m_firstSeqNumThisInt;       ///< sequence number of first packet received in current RTCP reporting interval
    quint64 m_packetsReceived;          ///< total number of packets received
    quint64 m_packetsReceivedThisInt;   ///< number of packets received in current RTCP reporting interval
    quint32 m_reportInterval;           ///< milliseconds between RTCP receiver report sending
    quint32 m_lastSenderTimestamp;      ///< timestamp of last sender report received
    
    RtcpHandler* m_rtcpHandler;         ///< RTCP handler
    
    QMutex m_queueMutex;                ///< mutex for safely adding/removing/playing from packet queue
};

#endif // RTPRECEIVER_H
