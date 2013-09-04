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
    printf("Streaming Audio Manager version %d.%d.%d\n", sam::VERSION_MAJOR, sam::VERSION_MINOR, sam::VERSION_PATCH);
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
    printf("[--maxout or -m max number of output channels to use]\n");
    printf("[--gui or '-g' run in gui mode]\n");
    printf("[--help or '-h' print help]\n");
    printf("\nLinux example usage:\n");
    printf("sam -n 2 -r 48000 -p 256 -d alsa -o 7770 -j 4464 -m 32\n");
    printf("OS X example usage:\n");
    printf("sam -n 2 -r 48000 -p 256 -d coreaudio -o 7770 -j 4464 -m 32\n");
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

bool parse_channels(QList<unsigned int>& channels, QString& channelString, unsigned int channelMax)
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

int main(int argc, char* argv[])
{
    print_info();

    // read initial settings from config file
    QSettings settings("sam.conf", QSettings::IniFormat);

    // TODO: add jackd command/path parameter
    // set parameter defaults
    SamParams params;
    QVariant temp;
    temp = settings.value("NumBasicChannels", 0);
    params.numBasicChannels = temp.toInt();

    temp = settings.value("SampleRate", 48000);
    params.sampleRate = temp.toInt();

    temp = settings.value("BufferSize", 256);
    params.bufferSize = temp.toInt();

#if defined __APPLE__
    temp = settings.value("JackDriver", "coreaudio");
    QString driver = temp.toString();
#else
    temp = settings.value("JackDriver", "alsa");
    QString driver = temp.toString();
#endif
    QByteArray driverBytes = driver.toLocal8Bit();
    params.jackDriver = driverBytes.data();

    temp = settings.value("OscPort", 7770);
    params.oscPort = temp.toInt();

    temp = settings.value("RtpPort", 4464);
    params.rtpPort = temp.toInt();

    temp = settings.value("MaxOutputChannels", 32768);
    params.maxOutputChannels = temp.toInt();

    temp = settings.value("Volume", 1.0f);
    params.volume = temp.toFloat();

    temp = settings.value("DelayMillis", 0.0f);
    params.delayMillis = temp.toFloat();

    temp = settings.value("MaxDelayMillis", 1000);
    params.maxDelayMillis = temp.toFloat();

    temp = settings.value("MaxClientDelayMillis", params.maxDelayMillis);
    params.maxClientDelayMillis = temp.toFloat();

    temp = settings.value("RenderHost", "");
    QString renderHost =  temp.toString();
    QByteArray renderHostBytes = renderHost.toLocal8Bit();
    params.renderHost = renderHostBytes.data();

    temp = settings.value("RenderPort", 0);
    params.renderPort = temp.toInt();

    temp = settings.value("PacketQueueSize", 4);
    params.packetQueueSize = temp.toInt();
    
    temp = settings.value("OutputJackClientNameBasic", "system");
    QString outputJackClientNameBasic = temp.toString();
    QByteArray outputJackClientBytesBasic = outputJackClientNameBasic.toLocal8Bit();
    params.outJackClientNameBasic = outputJackClientBytesBasic.data();

    temp = settings.value("OutputJackPortBaseBasic", "playback_");
    QString outputJackPortBaseBasic = temp.toString();
    QByteArray outputJackPortBytesBasic = outputJackPortBaseBasic.toLocal8Bit();
    params.outJackPortBaseBasic = outputJackPortBytesBasic.data();

    temp = settings.value("OutputJackClientNameDiscrete", "system");
    QString outputJackClientNameDiscrete = temp.toString();
    QByteArray outputJackClientBytesDiscrete = outputJackClientNameDiscrete.toLocal8Bit();
    params.outJackClientNameDiscrete = outputJackClientBytesDiscrete.data();

    temp = settings.value("OutputJackPortBaseDiscrete", "playback_");
    QString outputJackPortBaseDiscrete = temp.toString();
    QByteArray outputJackPortBytesDiscrete = outputJackPortBaseDiscrete.toLocal8Bit();
    params.outJackPortBaseDiscrete = outputJackPortBytesDiscrete.data();

    temp = settings.value("MaxClients", 100);
    params.maxClients = temp.toInt();

    temp = settings.value("MeterIntervalMillis", 1000.0f);
    params.meterIntervalMillis = temp.toFloat();

    temp = settings.value("VerifyPatchVersion", true);
    params.verifyPatchVersion = temp.toBool();

    temp = settings.value("OutputPortOffset", -1);
    int outputPortOffset = temp.toInt();
    if (outputPortOffset >= 0)
    {
        qWarning("WARNING: OutputPortOffset is no longer a valid config file parameter. Specify desired channels using BasicChannels and DiscreteChannels instead.");
    }

    if (params.maxClients <= 0)
    {
        qWarning("ERROR: MaxClients parameter must be at least 1");
        exit(EXIT_FAILURE);
    }

    temp = settings.value("UseGui", false);
    bool useGui = temp.toBool();

    temp = settings.value("HostAddress", "");
    params.hostAddress = temp.toString();

    // parse command-line parameters which will override config file settings
    bool basicChOverride = false;
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
            basicChOverride = true;
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
            qWarning("WARNING: outoffset or -f is no longer a valid parameter. Specify desired channels using BasicChannels and DiscreteChannels in sam.conf instead.");
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
    
    temp = settings.value("ClockSkewThreshold", params.bufferSize);
    params.clockSkewThreshold = temp.toInt(); // read this after parsing command-line params in case buffer size is specified on command line

    if (basicChOverride || !settings.contains("BasicChannels"))
    {
        // populate channel list based on numBasicChannels
        unsigned int maxChannel = params.maxOutputChannels > params.numBasicChannels ? params.numBasicChannels : params.maxOutputChannels;
        for (unsigned int ch = 1; ch <= maxChannel; ch++)
        {
            params.basicChannels.append(ch);
        }
    }
    else
    {
        // populate channel list from config file
        temp = settings.value("BasicChannels", "");
        QString basicChannelString = temp.toString();
        qDebug() << "basic channel string: " << basicChannelString;
        if (!basicChannelString.isEmpty() && !parse_channels(params.basicChannels, basicChannelString, params.maxOutputChannels))
        {
            qWarning("Error: couldn't parse basic channel string from config file");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < params.basicChannels.size(); i++)
    {
        qWarning("Configuring with basic channel %u", params.basicChannels[i]);
    }

    if (!settings.contains("DiscreteChannels"))
    {
        // populate channel list based on numBasicChannels and maxOutputChannels
        for (unsigned int ch = params.numBasicChannels + 1; ch <= params.maxOutputChannels; ch++)
        {
            params.discreteChannels.append(ch);
        }
    }
    else
    {
        // populate discrete channel list from config file
        temp = settings.value("DiscreteChannels", "");
        QString discreteChannelString = temp.toString();
        qDebug() << "discrete channel string: " << discreteChannelString;
        if (!discreteChannelString.isEmpty() && !parse_channels(params.discreteChannels, discreteChannelString, params.maxOutputChannels))
        {
            qWarning("Error: couldn't parse discrete channel string from config file");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < params.discreteChannels.size(); i++)
    {
        bool jackClientNamesMatch = (strcmp(params.outJackClientNameBasic, params.outJackClientNameDiscrete) == 0);
        bool jackPortNamesMatch = (strcmp(params.outJackPortBaseBasic, params.outJackPortBaseDiscrete) == 0);
        if (jackClientNamesMatch && jackPortNamesMatch && params.basicChannels.contains(params.discreteChannels[i]))
        {
            qWarning("Error: channel %d can't be both basic and discrete when JACK output client and port names are the same", params.discreteChannels[i]);
            exit(EXIT_FAILURE);
        }
        qWarning("Configuring with discrete channel %u", params.discreteChannels[i]);
    }

    // print params
    if (basicChOverride) printf("Number of basic channels: %d\n", params.numBasicChannels);
    printf("Sample rate: %d\n", params.sampleRate);
    printf("JACK period (buffer size): %d\n", params.bufferSize);
    printf("JACK driver: %s\n", params.jackDriver);
    printf("OSC server port: %u\n", params.oscPort);
    printf("Base RTP port: %u\n", params.rtpPort);
    printf("Max output channels: %d\n", params.maxOutputChannels);
    printf("Volume: %f\n", params.volume);
    printf("Delay in millis: %f\n", params.delayMillis);
    printf("Max delay in millis: %f\n", params.maxDelayMillis);
    printf("Render host: %s\n", params.renderHost);
    printf("Render OSC port: %u\n", params.renderPort);
    printf("Packet queue size: %u\n", params.packetQueueSize);
    printf("Clock skew threshold: %d\n", params.clockSkewThreshold);
    printf("Output JACK client name (Basic): %s\n", params.outJackClientNameBasic);
    printf("Output JACK port base (Basic): %s\n", params.outJackPortBaseBasic);
    printf("Output JACK client name (Discrete): %s\n", params.outJackClientNameDiscrete);
    printf("Output JACK port base (Discrete): %s\n", params.outJackPortBaseDiscrete);
    printf("Max clients: %d\n", params.maxClients);
    printf("Meter interval in millis: %f\n", params.meterIntervalMillis);
    printf("Verify patch version: %d\n", params.verifyPatchVersion);
    QByteArray hostBytes = params.hostAddress.toLocal8Bit();
    printf("Host address: %s\n", hostBytes.constData());

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
        printf("\nStarting SAM...\n");
        QCoreApplication app(argc, argv);
        QCoreApplication::setOrganizationName("UCSD Sonic Arts");
        QCoreApplication::setOrganizationDomain("sonicarts.ucsd.edu");
        QCoreApplication::setApplicationName("Streaming Audio Manager");

        StreamingAudioManager sam(params);

        signal(SIGINT, signalhandler);
        signal(SIGTERM, signalhandler);

        QObject::connect(&app, SIGNAL(aboutToQuit()), &sam, SLOT(doBeforeQuit()));
        QObject::connect(&sam, SIGNAL(startupError()), &app, SLOT(quit()));

        sam.start();

        app.exec();
    }

    // TODO: free char* data from params struct??

    exit(EXIT_SUCCESS);
}
