/**
 * @file sam_main.cpp
 * Streaming Audio Manager (SAM) main function
 * @author Michelle Daniels
 * @date September 2011
 * @copyright UCSD 2011-2013
 */

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QSettings>

#include "samui.h"
#include "sam.h"

using namespace sam;

void print_info()
{
    printf("\n");
    printf("--------------------------------------\n");
    printf("Streaming Audio Manager version %s\n", sam::VERSION_STRING);
    printf("Built on %s at %s\n", __DATE__, __TIME__);
    printf("Copyright UCSD 2011-2013\n");
    printf("--------------------------------------\n\n");
}

void print_help()
{
    printf("Usage: [sam --numchannels or -n number of basic (non-spatialized) channels]\n");
    printf("[--samplerate or -r sample rate]\n");
    printf("[--period or -p period (buffer size)]\n");
    printf("[--driver or -d driver to use for JACK (ie coreaudio, alsa, etc.)]\n");
    printf("[--oscport or -o OSC port]\n");
    printf("[--jtport or -j base JackTrip port]\n");
    printf("[--outoffset or -f output port offset]\n");
    printf("[--maxout or -m max number of output channels to use]\n");
    printf("[--gui or '-g' run in gui mode]\n");
    printf("[--help or '-h' print help]\n");
    printf("\nLinux example usage:\n");
    printf("sam -n 2 -r 48000 -p 256 -d alsa -o 7770 -j 4464 -f 0 -m 32\n");
    printf("OS X example usage:\n");
    printf("sam -n 2 -r 48000 -p 256 -d coreaudio -o 7770 -j 4464 -f 0 -m 32\n");
    printf("\n");
}

void signalhandler(int sig)
{
    qDebug("signal handler called: signal %d", sig);
    if (sig == SIGINT || sig == SIGTERM)
    {
        qDebug("telling app to exit");
        QCoreApplication::exit(0);
    }
}

bool parse_channels(QList<unsigned int>& channels, QString& channelString)
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
                qWarning("Could not parse channels");
                return false;
            }
            channels.append(ch);
        }
        else
        {
            QStringList channelRange = channelElem.split('-');
            if (channelRange.length() != 2)
            {
                qWarning("Could not parse channels");
                 return false;
            }
            bool ok = 0;
            unsigned int start = channelRange[0].toUInt(&ok);
            if (!ok)
            {
                qWarning("Could not parse channels");
                return false;
            }
            unsigned int stop = channelRange[1].toUInt(&ok);
            if (!ok)
            {
                qWarning("Could not parse channels");
                return false;
            }
            for (unsigned int ch = start; ch <= stop; ch++)
            {
                channels.append(ch);
            }
        }
        ++it;
    }

    return true;
}

int main(int argc, char* argv[])
{
    print_info();

    // read initial settings from config file
    QSettings settings("sam.conf", QSettings::IniFormat);

    // TODO: add jackd command/path parameter
    // set parameter defaults
    SamParams params;
    params.numBasicChannels = settings.value("NumBasicChannels", 2).toInt();
    params.sampleRate = settings.value("SampleRate", 48000).toInt();
    params.bufferSize = settings.value("BufferSize", 256).toInt();
#if defined __APPLE__
    QString driver = settings.value("JackDriver", "coreaudio").toString();
#else
    QString driver = settings.value("JackDriver", "alsa").toString();
#endif
    QByteArray driverBytes = driver.toAscii();
    params.jackDriver = driverBytes.data();
    params.oscPort = settings.value("OscPort", 7770).toInt();
    params.rtpPort = settings.value("RtpPort", 4464).toInt();
    params.outputPortOffset = settings.value("OutputPortOffset", 0).toInt();
    params.maxOutputChannels = settings.value("MaxOutputChannels", 32768).toInt();
    params.volume = settings.value("Volume", 1.0f).toFloat();
    params.delayMillis = settings.value("DelayMillis", 0.0f).toFloat();
    params.maxDelayMillis = settings.value("MaxDelayMillis", MAX_DELAY_MILLIS).toFloat();
    QString renderHost =  settings.value("RenderHost", "").toString();
    QByteArray renderHostBytes = renderHost.toAscii();
    params.renderHost = renderHostBytes.data();
    params.renderPort = settings.value("RenderPort", 0).toInt();

    bool useGui = settings.value("UseGui", false).toBool();

    // parse command-line parameters which will override config file settings
    while (true)
    {
        static struct option long_options[] = {
            // These options set a flag.
            //{"verbose", no_argument, &verbose_flag, 1},
            //{"brief", no_argument, &verbose_flag, 0},
            // These options don't set a flag. We distinguish them by their indices.
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
        int c = getopt_long(argc, argv, "n:r:p:d:o:j:f:m:h:g", long_options, &option_index);
        
        // Detect the end of the options.
        if (c == -1) break;

        switch (c)
        {
        case 'n':
            params.numBasicChannels = atoi(optarg);
            break;

        case 'r':
            params.sampleRate = atoi(optarg);
            break;

        case 'p':
            params.bufferSize = atoi(optarg);
            break;

        case 'd':
            params.jackDriver = optarg;
            break;

        case 'o':
            params.oscPort = (quint16)atoi(optarg);
            // TODO: check for valid range for port?
            break;

        case 'j':
            params.rtpPort = (quint16)atoi(optarg);
            // TODO: check for valid range for port?
            break;

        case 'f':
            params.outputPortOffset = atoi(optarg);
            break;

        case 'm':
            params.maxOutputChannels = atoi(optarg);
            break;

        case 'g':
            useGui = true;
            break;

        case 'h':
        default:
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

    /*QString basicChannelString = settings.value("BasicChannels", "").toString();
    qDebug() << "basic channel string: " << basicChannelString;
    QList<unsigned int> basicChannelList;
    if (!parse_channels(basicChannelList, basicChannelString) || basicChannelList.isEmpty())
    {
        qWarning("Couldn't parse basic channel string from config file");
    }
    // TODO: populate channel list based on numBasicChannels and outputPortOffset
    for (int i = 0; i < basicChannelList.size(); i++)
    {
        qWarning("Configuring with basic channel %u", basicChannelList[i]);
    }

    QString discreteChannelString = settings.value("DiscreteChannels", "").toString();
    qDebug() << "discrete channel string: " << discreteChannelString;
    QList<unsigned int> discreteChannelList;
    if (!parse_channels(discreteChannelList, discreteChannelString) || discreteChannelList.isEmpty())
    {
        qWarning("Couldn't parse discrete channel string from config file");
    }

    // TODO: populate channel list based on numBasicChannels, outputPortOffset, and maxOutputChannels
    for (int i = 0; i < discreteChannelList.size(); i++)
    {
        qWarning("Configuring with discrete channel %u", discreteChannelList[i]);
    }*/

    // print params
    printf("Number of basic channels: %d\n", params.numBasicChannels);
    printf("Sample rate: %d\n", params.sampleRate);
    printf("JACK period (buffer size): %d\n", params.bufferSize);
    printf("JACK driver: %s\n", params.jackDriver);
    printf("OSC server port: %u\n", params.oscPort);
    printf("Base RTP port: %u\n", params.rtpPort);
    printf("Output port offset: %d\n", params.outputPortOffset);
    printf("Max output channels: %d\n", params.maxOutputChannels);
    printf("Volume: %f\n", params.volume);
    printf("Delay in millis: %f\n", params.delayMillis);
    printf("Max delay in millis: %f\n", params.maxDelayMillis);
    printf("Render host: %s\n", params.renderHost);
    printf("Render OSC port: %u\n", params.renderPort);

    // TODO: check that all arguments are valid, non-null??
    if (useGui) // run in GUI mode
    {
        printf("\nStarting SAM in GUI mode...\n");
        QApplication app(argc, argv);
        QCoreApplication::setOrganizationName("UCSD Sonic Arts");
        QCoreApplication::setOrganizationDomain("sonicarts.ucsd.edu");
        QCoreApplication::setApplicationName("Streaming Audio Manager");
        SamUI sui(params);
        sui.show();

        signal(SIGINT, signalhandler);
        signal(SIGTERM, signalhandler);

        QObject::connect(&app, SIGNAL(aboutToQuit()), &sui, SLOT(doBeforeQuit()));

        app.exec();
    }
    else // run without GUI
    {
        printf("\nStarting SAM with no GUI...\n");
        QCoreApplication app(argc, argv);
        QCoreApplication::setOrganizationName("UCSD Sonic Arts");
        QCoreApplication::setOrganizationDomain("sonicarts.ucsd.edu");
        QCoreApplication::setApplicationName("Streaming Audio Manager");
        StreamingAudioManager sam(params);

        signal(SIGINT, signalhandler);
        signal(SIGTERM, signalhandler);

        QObject::connect(&app, SIGNAL(aboutToQuit()), &sam, SLOT(doBeforeQuit()));

        if (!sam.start()) exit(EXIT_FAILURE);

        app.exec();
    }

    exit(EXIT_SUCCESS);
}
