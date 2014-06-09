/**
 * @file rtp.h
 * RTP functionality
 * @author Michelle Daniels
 * @date June 2012
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
