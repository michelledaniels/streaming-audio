/**
 * @file saminput/main.cpp
 * saminput: command-line application to allow streaming to SAM from a physical audio input
 * @author Michelle Daniels
 * @date November 2012
 * @copyright UCSD 2012
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <getopt.h>
#include <signal.h>

#include <QtCore/QCoreApplication>
#include <QString>
#include <QStringList>

#include "sam_client.h"


using namespace sam;

void print_help()
{
    printf("Usage:\nsaminput --name or -n client name\n");
    printf("\t--ip or -i SAM ip\n");
    printf("\t--port or -p SAM port\n");
    printf("\t--channels or -c string containing list of input channels to use\n");
    printf("\t--type or -t rendering type\n");
    printf("\t[--x or -x initial x position coordinate]\n");
    printf("\t[--y or -y initial y position coordinate]\n");
    printf("\t[--width or -w initial width for SAM stream]\n");
    printf("\t[--height or -h initial height for SAM stream]\n");
    printf("\t[--depth or -d initial depth for SAM stream]\n");
    printf("\t[--queue or -q receiver packet queue size]\n");
    printf("\t[--preset or -r rendering preset]\n");
    printf("\nExample usage:\n");
    printf("saminput -n \"Example Client\" -i \"127.0.0.1\" -p 7770 -c \"1-2\" -t 0 -r 0 -q 2\n");
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

int main(int argc, char *argv[])
{

    if (argc < 11)
    {
        print_help();
        exit(EXIT_SUCCESS);
    }

    QCoreApplication a(argc, argv);

    StreamingAudioClient sac;

    unsigned int* inputChannels = NULL;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int depth = 0;
    SacParams params;
    params.numChannels = 0;
    params.type = (StreamingAudioType)-1;
    params.name = NULL;
    params.samIP = NULL;
    params.samPort = 0;
    params.packetQueueSize = -1;
    params.preset = 0;

    // parse command-line parameters
    while (true)
    {
        static struct option long_options[] = {
            // These options set a flag.
            {"name", required_argument, NULL, 'n'},
            {"ip", required_argument, NULL, 'i'},
            {"port", required_argument, NULL, 'p'},
            {"channels", required_argument, NULL, 'c'},
            {"type", required_argument, NULL, 't'},
            {"x", optional_argument, NULL, 'x'},
            {"y", optional_argument, NULL, 'y'},
            {"width", optional_argument, NULL, 'w'},
            {"height", optional_argument, NULL, 'h'},
            {"depth", optional_argument, NULL, 'd'},
            {"queue", optional_argument, NULL, 'q'},
            {"preset", optional_argument, NULL, 'r'},
            {NULL, 0, NULL, 0}
        };

        // getopt_long stores the option index here.
        int option_index = 0;
        int c = getopt_long(argc, argv, "n:i:p:c:t:x:y:w:h:d:q:r:", long_options, &option_index);

        // Detect the end of the options.
        if (c == -1) break;

        if (!optarg)
        {
            qWarning("c = %d, optarg NULL!", c);
            exit(EXIT_FAILURE);
        }
        //qWarning("c = %d, optarg = %s", c, optarg);

        switch (c)
        {
        case 'n':
            params.name = optarg;
            qWarning("setting name = %s", params.name);
            break;

        case 'i':
            params.samIP = optarg;
            qWarning("setting samIP = %s", params.samIP);
            break;

        case 'p':
            params.samPort = atoi(optarg);
            qWarning("setting samPort = %u", params.samPort);
            break;

        case 'c':
        {
            QString channelString(optarg);
            qWarning("setting channel string = %s", optarg);
            QStringList channelSplit = channelString.split(',');
            QStringList::iterator it = channelSplit.begin();
            QList<unsigned int> channels;
            while (it != channelSplit.end())
            {
                QString channelElem = *it;
                if (!channelElem.contains('-'))
                {
                    bool ok = 0;
                    unsigned int ch = channelElem.toUInt(&ok);
                    if (!ok)
                    {
                        qWarning("Could not parse channels");
                        exit(EXIT_FAILURE);
                    }
                    channels.append(ch);
                }
                else
                {
                    QStringList channelRange = channelElem.split('-');
                    if (channelRange.length() != 2)
                    {
                        qWarning("Could not parse channels");
                        exit(EXIT_FAILURE);
                    }
                    bool ok = 0;
                    unsigned int start = channelRange[0].toUInt(&ok);
                    if (!ok)
                    {
                        qWarning("Could not parse channels");
                        exit(EXIT_FAILURE);
                    }
                    unsigned int stop = channelRange[1].toUInt(&ok);
                    if (!ok)
                    {
                        qWarning("Could not parse channels");
                        exit(EXIT_FAILURE);
                    }
                    for (unsigned int ch = start; ch <= stop; ch++)
                    {
                        channels.append(ch);
                    }
                }
                ++it;
            }
            if (channels.isEmpty())
            {
                qWarning("No channels found");
                exit(EXIT_FAILURE);
            }
            else
            {
                params.numChannels = channels.length();
                inputChannels = new unsigned int[params.numChannels];
                for (unsigned int i = 0; i < params.numChannels; i++)
                {
                    inputChannels[i] = channels[i];
                    qDebug("Adding input channel %u", inputChannels[i]);
                }
            }
            break;
        }
        case 'x':
            x = atoi(optarg);
            qWarning("setting x = %d", x);
            break;

        case 'y':
            y = atoi(optarg);
            qWarning("setting y = %d", y);
            break;

        case 'w':
            width = atoi(optarg);
            qWarning("setting width = %d", width);
            break;

        case 'h':
            height = atoi(optarg);
            qWarning("setting height = %d", height);
            break;

        case 'd':
            //depth = atoi(optarg);
            qWarning("setting depth = %d", depth);
            break;

        case 't':
            params.type = (StreamingAudioType)atoi(optarg);
            qWarning("setting type = %d", params.type);
            break;

        case 'q':
            params.packetQueueSize = atoi(optarg);
            qWarning("setting packetQueueSize = %d", params.packetQueueSize);
            break;

        case 'r':
            params.preset = atoi(optarg);
            qWarning("setting preset = %d", params.preset);
            break;

        default:
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

    if (!params.name)
    {
        print_help();
        exit(EXIT_FAILURE);
    }

    printf("finished parsing args");

    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);

    if (sac.init(params) != SAC_SUCCESS)
    {
        qWarning("Couldn't initialize client");
        exit(EXIT_FAILURE);
    }

    if (sac.start(x, y, width, height, depth) != SAC_SUCCESS)
    {
        qWarning("Couldn't start client");
        exit(EXIT_FAILURE);
    }

    // set physical inputs
    if (sac.setPhysicalInputs(inputChannels) != SAC_SUCCESS)
    {
        qWarning("Couldn't set input channels");
        exit(EXIT_FAILURE);
    }
    
    return a.exec();
}
