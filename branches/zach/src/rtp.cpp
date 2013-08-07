/**
 * @file rtp.cpp
 * Implementation of RTP functionality
 * @author Michelle Daniels
 * @date June 2012
 * @copyright UCSD 2012
 */

#include <QDataStream>
#include <QtEndian>

#include "rtp.h"

namespace sam
{
static const float Q_16BIT = 32768.5;
static const float Q_24BIT = 8388607.5;

/* ----- RtpPacket implementation ----- */
RtpPacket::RtpPacket() :
    m_next(NULL),
    m_arrivalTime(0),
    m_timestamp(0),
    m_sequenceNum(0),
    m_playoutTime(0),
    m_ssrc(0),
    m_payloadType(0),
    m_used(false)
{

}

RtpPacket::RtpPacket(const RtpPacket& packet)
{
    m_next = packet.m_next;
    m_arrivalTime = packet.m_arrivalTime;
    m_timestamp = packet.m_timestamp;
    m_sequenceNum = packet.m_sequenceNum;
    m_extendedSeqNum = packet.m_extendedSeqNum;
    m_playoutTime = packet.m_playoutTime;
    m_ssrc = packet.m_ssrc;
    m_payloadType = packet.m_payloadType;
    m_payload.append(packet.m_payload);
    m_used = packet.m_used;
}

RtpPacket& RtpPacket::operator=(const RtpPacket packet)
{
    m_next = packet.m_next;
    m_arrivalTime = packet.m_arrivalTime;
    m_timestamp = packet.m_timestamp;
    m_sequenceNum = packet.m_sequenceNum;
    m_extendedSeqNum = packet.m_extendedSeqNum;
    m_playoutTime = packet.m_playoutTime;
    m_ssrc = packet.m_ssrc;
    m_payloadType = packet.m_payloadType;
    m_payload.append(packet.m_payload);
    m_used = packet.m_used;
    return *this;
}

RtpPacket::~RtpPacket() {}

bool RtpPacket::read(QByteArray& data, quint32 arrivalTime)
{
    m_arrivalTime = arrivalTime;

    // validate size
    if (data.size() < 12)
    {
        qWarning("RtpPacket::read Invalid packet size = %d bytes", data.size());
        return false;
    }
    else if (data.size() == 12)
    {
        qWarning("RtpPacket::read Received empty packet");
        return false;
    }

    QDataStream stream(&data, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // check version
    quint8 versionByte = 0;
    stream >> versionByte;
    if (versionByte != 128)
    {
        qWarning("RtpPacket::read Invalid RTP version");
        return false;
    }

    // get payload type
    quint8 typeByte = 0;
    stream >> typeByte;
    typeByte &= 127; // only want lower 7 bits
    if (typeByte < PAYLOAD_MIN || typeByte > PAYLOAD_MAX)
    {
        qWarning("RtpPacket::read Invalid payload type: %d", typeByte);
        return false;
    }
    else
    {
        m_payloadType = typeByte;
        //qDebug("RtpPacket::read payload type = %d", typeByte);
    }

    // get sequence number
    stream >> m_sequenceNum;

    // get timestamp
    stream >> m_timestamp;

    // get ssrc
    stream >> m_ssrc;

    // read payload data
    m_payload = data.right(data.size() - 12);

    return true;
}

bool RtpPacket::write(QByteArray& data)
{
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // write version
    quint8 versionByte = 128; // set version to be 2 (uppermost 2 bits)
    stream << versionByte;

    // write payload type
    quint8 typeByte = m_payloadType & 127;
    stream << typeByte;

    // write sequence number
    stream << m_sequenceNum;

    // write timestamp
    stream << m_timestamp;

    // write ssrc
    stream << m_ssrc;

    // write payload data
    data.append(m_payload);

    return true;
}

bool RtpPacket::init(quint32 timestamp, quint16 sequenceNum, quint8 payloadType, quint32 ssrc)
{
    m_timestamp = timestamp;
    m_sequenceNum = sequenceNum;
    m_extendedSeqNum = sequenceNum;
    m_payloadType = payloadType;
    m_ssrc = ssrc;
    m_payload.clear();
    return true;
}

bool RtpPacket::setPayload(int numChannels, int numSamples, float** data)
{
    QDataStream stream(&m_payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    switch (m_payloadType)
    {
    case PAYLOAD_PCM_16:
    {
        for (int ch = 0; ch < numChannels; ch++)
        {
            for (int n = 0; n < numSamples; n++)
            {
                // hard clipping
                float floatSample = data[ch][n] > 1.0f ? 1.0f : data[ch][n];
                floatSample = floatSample < -1.0f ? -1.0f : floatSample;

                // quantization
                qint16 sample = (qint16)(data[ch][n] * Q_16BIT);
                stream << sample;
            }
        }
        break;
    }
    case PAYLOAD_PCM_24:
    {
        for (int ch = 0; ch < numChannels; ch++)
        {
            for (int n = 0; n < numSamples; n++)
            {
                // hard clipping
                float floatSample = data[ch][n] > 1.0f ? 1.0f : data[ch][n];
                floatSample = floatSample < -1.0f ? -1.0f : floatSample;

                // quantization
                qint32 sample = (qint32)(data[ch][n] * Q_24BIT);

                // convert to network byte order
                quint8 sampleBytes[4];
                qToBigEndian(sample, sampleBytes);
                stream << sampleBytes[1];
                stream << sampleBytes[2];
                stream << sampleBytes[3];
            }
        }
        break;
    }
    case PAYLOAD_PCM_32:
    {
        for (int ch = 0; ch < numChannels; ch++)
        {
            for (int n = 0; n < numSamples; n++)
            {
                // TODO: need clipping?
                stream << data[ch][n];
            }
        }
        break;
    }
    default:
        return false;
    }
    return true;
}

bool RtpPacket::getPayload(int numChannels, int numSamples, float** data)
{
    QDataStream stream(&m_payload, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    switch (m_payloadType)
    {
    case PAYLOAD_PCM_16:
    {
        int expectedSize = numChannels * numSamples * 2;
        if (m_payload.size() != expectedSize)
        {
            qWarning("RtpPacket::getPayload payload size doesn't match: expected %d bytes, found %d", expectedSize, m_payload.size());
            return false;
        }
        qint16 sample = 0;
        for (int ch = 0; ch < numChannels; ch++)
        {
            for (int n = 0; n < numSamples; n++)
            {
                stream >> sample;
                data[ch][n] = sample / Q_16BIT;
            }
        }
        break;
    }
    case PAYLOAD_PCM_24:
    {
        int expectedSize = numChannels * numSamples * 3;
        if (m_payload.size() != expectedSize)
        {
            qWarning("RtpPacket::getPayload payload size doesn't match: expected %d bytes, found %d", expectedSize, m_payload.size());
            return false;
        }

        for (int ch = 0; ch < numChannels; ch++)
        {
            for (int n = 0; n < numSamples; n++)
            {
                // convert from network byte order
                quint8 sampleBytes[4];
                sampleBytes[0] = 0;
                stream >> sampleBytes[1];
                stream >> sampleBytes[2];
                stream >> sampleBytes[3];
                qint32* samplePtr = reinterpret_cast<qint32*>(sampleBytes);
                qint32 sample = qFromBigEndian(*samplePtr);
                if (sample & 0x800000) sample = (sample | 0xFF000000); // restore sign

                data[ch][n] = sample / Q_24BIT;
            }
        }
        break;
    }
    case PAYLOAD_PCM_32:
    {
        int expectedSize = numChannels * numSamples * 4;
        if (m_payload.size() != expectedSize)
        {
            qWarning("RtpPacket::getPayload payload size doesn't match: expected %d bytes, found %d", expectedSize, m_payload.size());
            return false;
        }
        for (int ch = 0; ch < numChannels; ch++)
        {
            for (int n = 0; n < numSamples; n++)
            {
                stream >> data[ch][n];
            }
        }
        break;
    }
    default:
        return false;
    }
    return true;
}

} // end of namespace SAM
