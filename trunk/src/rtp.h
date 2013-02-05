/**
 * @file rtp.h
 * RTP functionality
 * Michelle Daniels  June 2012
 * Copyright UCSD 2012
 */

#ifndef RTP_H
#define RTP_H

#include <QByteArray>
#include <QtEndian>

static const quint8 PAYLOAD_PCM_16 = 96; ///< signed 16-bit int
static const quint8 PAYLOAD_PCM_24 = 97; ///< signed 24-bit int
static const quint8 PAYLOAD_PCM_32 = 98; ///< 32-bit float
static const quint8 PAYLOAD_MIN = PAYLOAD_PCM_16;
static const quint8 PAYLOAD_MAX = PAYLOAD_PCM_32;

// TODO: put this and other RTP code in SAM namespace

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

    bool read(QByteArray& data, quint32 arrivalTime);

    bool write(QByteArray& data);

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

    RtpPacket* m_next;
    quint32 m_arrivalTime;
    quint32 m_timestamp;
    quint16 m_sequenceNum;
    quint64 m_extendedSeqNum;
    quint32 m_playoutTime;
    quint32 m_ssrc;
    quint8 m_payloadType;
    QByteArray m_payload;
    bool m_used;
};

#endif // RTP_H
