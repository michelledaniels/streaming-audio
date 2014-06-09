/**
 * @file samcpp
 * Implementation of parsing SAM parameters
 * @author Michelle Daniels
 * @date September 2013
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

#include <getopt.h>

#include <QDebug>
#include <QSettings>
#include <QStringList>

#include "samparams.h"

namespace sam
{
SamParams::SamParams() :
    sampleRate(48000),
    bufferSize(256),
    numBasicChannels(0),
    oscPort(7770),
    rtpPort(4464),
    maxOutputChannels(128),
    volume(1.0f),
    delayMillis(0.0f),
    maxDelayMillis(1000.0f),
    maxClientDelayMillis(1000.0f),
    renderPort(0),
    packetQueueSize(4),
    clockSkewThreshold(bufferSize),
    maxClients(100),
    meterIntervalMillis(1000.0f),
    verifyPatchVersion(false),
    useGui(false),
    printHelp(false)
{

}

bool SamParams::parseConfig(const char* configFile, int argc, char* argv[])
{
    // read initial settings from config file
    QSettings settings(configFile, QSettings::IniFormat);

    QVariant temp;
    temp = settings.value("NumBasicChannels", numBasicChannels);
    numBasicChannels = temp.toInt();

    temp = settings.value("SampleRate", sampleRate);
    sampleRate = temp.toInt();

    temp = settings.value("BufferSize", bufferSize);
    bufferSize = temp.toInt();

#if defined __APPLE__
    temp = settings.value("JackDriver", "coreaudio");
#else
    temp = settings.value("JackDriver", "alsa");
#endif
    jackDriver = temp.toString();

    temp = settings.value("OscPort", oscPort);
    oscPort = temp.toInt();

    temp = settings.value("RtpPort", rtpPort);
    rtpPort = temp.toInt();

    temp = settings.value("MaxOutputChannels", maxOutputChannels);
    maxOutputChannels = temp.toInt();

    temp = settings.value("Volume", volume);
    volume = temp.toFloat();

    temp = settings.value("DelayMillis", delayMillis);
    delayMillis = temp.toFloat();

    temp = settings.value("MaxDelayMillis", maxDelayMillis);
    maxDelayMillis = temp.toFloat();

    temp = settings.value("MaxClientDelayMillis", maxDelayMillis);
    maxClientDelayMillis = temp.toFloat();

    temp = settings.value("RenderHost", "");
    renderHost = temp.toString();

    temp = settings.value("RenderPort", renderPort);
    renderPort = temp.toInt();

    temp = settings.value("PacketQueueSize", packetQueueSize);
    packetQueueSize = temp.toInt();

    temp = settings.value("OutputJackClientNameBasic", "system");
    outJackClientNameBasic = temp.toString();

    temp = settings.value("OutputJackPortBaseBasic", "playback_");
    outJackPortBaseBasic = temp.toString();

    temp = settings.value("OutputJackClientNameDiscrete", "system");
    outJackClientNameDiscrete = temp.toString();

    temp = settings.value("OutputJackPortBaseDiscrete", "playback_");
    outJackPortBaseDiscrete = temp.toString();

    temp = settings.value("MaxClients", maxClients);
    maxClients = temp.toInt();

    temp = settings.value("MeterIntervalMillis", meterIntervalMillis);
    meterIntervalMillis = temp.toFloat();

    temp = settings.value("VerifyPatchVersion", verifyPatchVersion);
    verifyPatchVersion = temp.toBool();

    temp = settings.value("OutputPortOffset", -1);
    int outputPortOffset = temp.toInt();
    if (outputPortOffset >= 0)
    {
        qWarning("SamParams::parseConfig WARNING: OutputPortOffset is no longer a valid config file parameter. Specify desired channels using BasicChannels and DiscreteChannels instead.");
    }

    if (maxClients <= 0)
    {
        qWarning("SamParams::parseConfig ERROR: MaxClients parameter must be at least 1");
        return false;
    }

    temp = settings.value("UseGui", useGui);
    useGui = temp.toBool();

    temp = settings.value("HostAddress", "");
    hostAddress = temp.toString();

    // parse command-line parameters which will override config file settings
    bool basicChOverride = false;
    if (argc > 0)
    {
        while (true)
        {
            static struct option long_options[] = {
                {"numchannels", optional_argument, NULL, 'n'},
                {"samplerate", optional_argument, NULL, 'r'},
                {"period", optional_argument, NULL, 'p'},
                {"driver", optional_argument, NULL, 'd'},
                {"oscport", optional_argument, NULL, 'o'},
                {"jtport", optional_argument, NULL, 'j'},
                {"outoffset", optional_argument, NULL, 'f'},
                {"maxout", optional_argument, NULL, 'm'},
                {"gui", no_argument, NULL, 'g'},
                {"help", no_argument, NULL, 'h'},
                {NULL, 0, NULL, 0}
            };

            // getopt_long stores the option index here.
            int option_index = 0;
            int c = getopt_long(argc, argv, "n:r:p:d:o:j:f:m:hg", long_options, &option_index);

            // Detect the end of the options.
            if (c == -1) break;

            switch (c)
            {
            case 'n':
                numBasicChannels = atoi(optarg);
                basicChOverride = true;
                break;

            case 'r':
                sampleRate = atoi(optarg);
                break;

            case 'p':
                bufferSize = atoi(optarg);
                break;

            case 'd':
                jackDriver.append(optarg);
                break;

            case 'o':
                oscPort = (quint16)atoi(optarg);
                // TODO: check for valid range for port?
                break;

            case 'j':
                rtpPort = (quint16)atoi(optarg);
                // TODO: check for valid range for port?
                break;

            case 'f':
                qWarning("SamParams::parseConfig WARNING: outoffset or -f is no longer a valid parameter. Specify desired channels using BasicChannels and DiscreteChannels in sam.conf instead.");
                break;

            case 'm':
                maxOutputChannels = atoi(optarg);
                break;

            case 'g':
                useGui = true;
                break;

            case 'h':
            default:
                printHelp = true;
            }
        }
    }

    temp = settings.value("ClockSkewThreshold", bufferSize);
    clockSkewThreshold = temp.toInt(); // read this after parsing command-line params in case buffer size is specified on command line

    if (basicChOverride || !settings.contains("BasicChannels"))
    {
        // populate channel list based on numBasicChannels
        unsigned int maxChannel = maxOutputChannels > numBasicChannels ? numBasicChannels : maxOutputChannels;
        for (unsigned int ch = 1; ch <= maxChannel; ch++)
        {
            basicChannels.append(ch);
        }
    }
    else
    {
        // populate channel list from config file
        temp = settings.value("BasicChannels", "");
        QString basicChannelString = temp.toString();
        qDebug() << "basic channel string: " << basicChannelString;
        if (!basicChannelString.isEmpty() && !parseChannels(basicChannels, basicChannelString, maxOutputChannels))
        {
            qWarning("Error: couldn't parse basic channel string from config file");
            return false;
        }
    }
    for (int i = 0; i < basicChannels.size(); i++)
    {
        qWarning("Configuring with basic channel %u", basicChannels[i]);
    }

    if (!settings.contains("DiscreteChannels"))
    {
        // populate channel list based on numBasicChannels and maxOutputChannels
        for (unsigned int ch = numBasicChannels + 1; ch <= maxOutputChannels; ch++)
        {
            discreteChannels.append(ch);
        }
    }
    else
    {
        // populate discrete channel list from config file
        temp = settings.value("DiscreteChannels", "");
        QString discreteChannelString = temp.toString();
        qDebug() << "discrete channel string: " << discreteChannelString;
        if (!discreteChannelString.isEmpty() && !parseChannels(discreteChannels, discreteChannelString, maxOutputChannels))
        {
            qWarning("Error: couldn't parse discrete channel string from config file");
            return false;
        }
    }
    for (int i = 0; i < discreteChannels.size(); i++)
    {
        bool jackClientNamesMatch = (outJackClientNameBasic.compare(outJackClientNameDiscrete) == 0);
        bool jackPortNamesMatch = (outJackPortBaseBasic.compare(outJackPortBaseDiscrete) == 0);
        if (jackClientNamesMatch && jackPortNamesMatch && basicChannels.contains(discreteChannels[i]))
        {
            qWarning("Error: channel %d can't be both basic and discrete when JACK output client and port names are the same", discreteChannels[i]);
            return false;
        }
        qWarning("Configuring with discrete channel %u", discreteChannels[i]);
    }

    // print params
    if (basicChOverride) printf("Number of basic channels: %d\n", numBasicChannels);
    printf("Sample rate: %d\n", sampleRate);
    printf("JACK period (buffer size): %d\n", bufferSize);
    QByteArray jackDriverBytes = jackDriver.toLocal8Bit();
    printf("JACK driver: %s\n", jackDriverBytes.constData());
    printf("OSC server port: %u\n", oscPort);
    printf("Base RTP port: %u\n", rtpPort);
    printf("Max output channels: %d\n", maxOutputChannels);
    printf("Volume: %f\n", volume);
    printf("Delay in millis: %f\n", delayMillis);
    printf("Max delay in millis: %f\n", maxDelayMillis);
    QByteArray renderHostBytes = renderHost.toLocal8Bit();
    printf("Render host: %s\n", renderHostBytes.constData());
    printf("Render OSC port: %u\n", renderPort);
    printf("Packet queue size: %u\n", packetQueueSize);
    printf("Clock skew threshold: %d\n", clockSkewThreshold);
    QByteArray clientNameBasicBytes = outJackClientNameBasic.toLocal8Bit();
    printf("Output JACK client name (Basic): %s\n", clientNameBasicBytes.constData());
    QByteArray portBasicBytes = outJackPortBaseBasic.toLocal8Bit();
    printf("Output JACK port base (Basic): %s\n", portBasicBytes.constData());
    QByteArray clientNameDiscreteBytes = outJackClientNameDiscrete.toLocal8Bit();
    printf("Output JACK client name (Discrete): %s\n", clientNameDiscreteBytes.constData());
    QByteArray portDiscreteBytes = outJackPortBaseDiscrete.toLocal8Bit();
    printf("Output JACK port base (Discrete): %s\n", portDiscreteBytes.constData());
    printf("Max clients: %d\n", maxClients);
    printf("Meter interval in millis: %f\n", meterIntervalMillis);
    printf("Verify patch version: %d\n", verifyPatchVersion);
    QByteArray hostBytes = hostAddress.toLocal8Bit();
    printf("Host address: %s\n", hostBytes.constData());

    return true;
}

bool SamParams::parseChannels(QList<unsigned int>& channels, QString& channelString, unsigned int channelMax)
{
    QStringList channelSplit = channelString.split(',');
    QStringList::iterator it = channelSplit.begin();
    while (it != channelSplit.end())
    {
        QString channelElem = *it;
        //qDebug() << channelElem;
        if (!channelElem.contains('-'))
        {
            bool ok = 0;
            unsigned int ch = channelElem.toUInt(&ok);
            if (!ok)
            {
                //qWarning("Could not parse channels");
                return false;
            }
            if (ch <= channelMax)
            {
                channels.append(ch);
            }
            else
            {
                qWarning("ignoring channel %u which exceeds max channels (%u)", ch, channelMax);
            }
        }
        else
        {
            QStringList channelRange = channelElem.split('-');
            if (channelRange.length() != 2)
            {
                //qWarning("Could not parse channels");
                 return false;
            }
            bool ok = 0;
            unsigned int start = channelRange[0].toUInt(&ok);
            if (!ok)
            {
                //qWarning("Could not parse channels");
                return false;
            }
            unsigned int stop = channelRange[1].toUInt(&ok);
            if (!ok)
            {
                //qWarning("Could not parse channels");
                return false;
            }
            for (unsigned int ch = start; ch <= stop; ch++)
            {
                if (ch <= channelMax)
                {
                    channels.append(ch);
                }
                else
                {
                    qWarning("ignoring channel %u which exceeds max channels (%u)", ch, channelMax);
                }
            }
        }
        ++it;
    }

    return true;
}

} // end namespace sam
