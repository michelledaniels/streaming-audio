/**
 * @file rtp.h
 * RTP functionality
 * @author Michelle Daniels
 * @date June 2012
 * @copyright UCSD 2012
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef RTP_H
#define RTP_H

#include <QByteArray>
#include <QtEndian>

namespace sam
{
static const quint8 PAYLOAD_PCM_16 = 96; ///< signed 16-bit int
static const quint8 PAYLOAD_PCM_24 = 97; ///< signed 24-bit int
static const quint8 PAYLOAD_PCM_32 = 98; ///< 32-bit float
static const quint8 PAYLOAD_MIN = PAYLOAD_PCM_16;
static const quint8 PAYLOAD_MAX = PAYLOAD_PCM_32;

/**
 * @class RtpPacket
 * Encapsulates reading to/writing from RTP packets.
 */
class RtpPacket
{
public:

    /**
     * Constructor.
     */
    RtpPacket();

    /**
     * Destructor.
     */
    virtual ~RtpPacket();

    /**
     * Copy constructor.
     */
    RtpPacket(const RtpPacket& packet);

    /**
     * Assignment operator.
     */
    RtpPacket& operator=(const RtpPacket packet);

    /**
     * Read an RTP packet from the given byte array.
     * @param data byte array to read from
     * @param arrivalTime timestamp when packet was received
     * @return true on success, false on failure
     */
    bool read(QByteArray& data, quint32 arrivalTime);

    /**
     * Write an RTP packet to the given byte array.
     * @param data byte array to write to
     * @return true on success, false on failure
     */
    bool write(QByteArray& data);

    /**
     * Initialize this RTP packet.
     * @param timestamp sending timestamp
     * @param sequenceNum sequence number
     * @param payloadType payload type
     * @param ssrc sender SSRC
     * @return true on success, false on failure
     */
    bool init(quint32 timestamp, quint16 sequenceNum, quint8 payloadType, quint32 ssrc);

    /**
     * Set the payload audio data.
     * @param numChannels the number of channels of audio data
     * @param numSamples the number of samples of audio data
     * @param data the audio data indexed as data[channel][sample]
     * @return true on success, false otherwise
     */
    bool setPayload(int numChannels, int numSamples, float** data);

    /**
     * Get the payload audio data.
     * @param numChannels the number of channels of audio data
     * @param numSamples the number of samples of audio data to get
     * @param data the pre-allocated storage for the audio data indexed as data[channel][sample]
     * @return true on success, false otherwise
     */
    bool getPayload(int numChannels, int numSamples, float** data);

    RtpPacket* m_next;          ///< pointer to next packet in queue
    quint32 m_arrivalTime;      ///< arrival time
    quint32 m_timestamp;        ///< timestamp
    quint16 m_sequenceNum;      ///< sequence number
    quint64 m_extendedSeqNum;   ///< extended sequence number
    quint32 m_playoutTime;      ///< time this packet should be played
    quint32 m_ssrc;             ///< SSRC of packet's sender
    quint8 m_payloadType;       ///< payload type
    QByteArray m_payload;       ///< payload data
    bool m_used;                ///< whether this packet has been used already (played or skipped)
};

} // end of namespace SAM

#endif // RTP_H
