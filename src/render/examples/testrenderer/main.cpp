/**
 * @file testrenderer/main.cpp
 * testrenderer: command-line application using JACK for testing SAM renderer library
 * @author Michelle Daniels
 * @date January 2014
 * @copyright UCSD 2014
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <getopt.h>
#include <signal.h>

#include <QtCore/QCoreApplication>

#include "testrenderer.h"

using namespace sam;

void print_help()
{
    printf("Usage: [testrenderer --name or -n JACK client name]\n");
    printf("--ip or -i SAM ip\n");
    printf("--port or -p SAM OSC port\n");
    printf("--oscport or -o renderer OSC port\n");
    printf("--channels or -c number of input channels (JACK input ports)\n");
    printf("\nExample usage:\n");
    printf("testrenderer -n \"TestRenderer\" -i \"127.0.0.1\" -p 7770 -o 0 -c 64\n");
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
    if (argc < 9)
    {
        print_help();
        exit(EXIT_SUCCESS);
    }

    QCoreApplication a(argc, argv);

    SamRenderParams params;
    params.samIP = NULL;
    params.samPort = 0;
    params.replyIP = NULL;
    params.replyPort = 0;

    char* jackClientName = NULL;
    int numChannels = 0;

    // parse command-line parameters
    while (true)
    {
        static struct option long_options[] = {
            // These options set a flag.
            {"name", required_argument, NULL, 'n'},
            {"ip", required_argument, NULL, 'i'},
            {"port", required_argument, NULL, 'p'},
            {"oscport", required_argument, NULL, 'o'},
            {"channels", required_argument, NULL, 'c'},
            {NULL, 0, NULL, 0}
        };

        // getopt_long stores the option index here.
        int option_index = 0;
        int c = getopt_long(argc, argv, "n:i:p:o:c:", long_options, &option_index);

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
            jackClientName = optarg;
            qWarning("setting JACK client name = %s", jackClientName);
            break;

        case 'i':
            params.samIP = optarg;
            qWarning("setting SAM IP = %s", params.samIP);
            break;

        case 'p':
            params.samPort = atoi(optarg);
            qWarning("setting SAM OSC port = %u", params.samPort);
            break;

        case 'o':
            params.replyPort = atoi(optarg);
            qWarning("setting renderer OSC port = %u", params.replyPort);
            break;

        case 'c':
        {
            int temp = atoi(optarg);
            if (temp <=0)
            {
                qCritical("Number of channels must be at least 1");
            }
            numChannels = temp;
            qWarning("setting number of channels = %u", numChannels);
            break;
        }
        default:
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

    if (!jackClientName)
    {
        print_help();
        exit(EXIT_FAILURE);
    }

    qDebug("finished parsing args");

    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);

    TestRenderer renderer(params, jackClientName, numChannels);
    if (!renderer.init())
    {
        qWarning("Couldn't initialize TestRenderer");
        exit(EXIT_FAILURE);
    }

    return a.exec();
}
