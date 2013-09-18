/**
 * @file samugen-gui/main.cpp
 * samugen-gui: gui application for unit generators streaming to SAM
 * @author Michelle Daniels
 * @date September 2013
 * @copyright UCSD 2013
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <getopt.h>
#include <signal.h>

#include <QApplication>

#include "sam_client.h"
#include "samugengui.h"

using namespace sam;

void print_help()
{
    printf("Usage: samugen-gui");// --name or -n client name]\n");
    printf("[--ip or -i SAM ip]\n");
    printf("[--port or -p SAM port]\n");
    //printf("--channels or -c number of channels to stream\n");
    //printf("--type or -t rendering type\n");
    //printf("[--x or -x initial x position coordinate]\n");
    //printf("[--y or -y initial y position coordinate]\n");
    //printf("[--width or -w initial width for SAM stream]\n");
    //printf("[--height or -h initial height for SAM stream]\n");
    //printf("[--depth or -d initial depth for SAM stream]\n");
    //printf("[--preset or -r rendering preset]\n");
    printf("\nExample usage:\n");
    //printf("samugen-gui -n \"Example Client\" -i \"127.0.0.1\" -p 7770 -c 2 -t 0 -r 0\n");
    printf("samugen-gui -i \"127.0.0.1\" -p 7770\n");
    printf("\n");
}

void signalhandler(int sig)
{
    qDebug("signal handler called: signal %d", sig);
    if (sig == SIGINT || sig == SIGTERM)
    {
        qDebug("telling app to exit");
        QApplication::exit(0);
    }
}

int main(int argc, char *argv[])
{
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
    params.samPort = 7770;
    params.preset = 0;

    // parse command-line parameters
    while (true)
    {
        static struct option long_options[] = {
            //{"name", optional_argument, NULL, 'n'},
            {"ip", optional_argument, NULL, 'i'},
            {"port", optional_argument, NULL, 'p'},
            //{"channels", optional_argument, NULL, 'c'},
            //{"type", optional_argument, NULL, 't'},
            //{"x", optional_argument, NULL, 'x'},
            //{"y", optional_argument, NULL, 'y'},
            //{"width", optional_argument, NULL, 'w'},
            //{"height", optional_argument, NULL, 'h'},
            //{"depth", optional_argument, NULL, 'd'},
            //{"preset", optional_argument, NULL, 'r'},
            {NULL, 0, NULL, 0}
        };

        // getopt_long stores the option index here.
        int option_index = 0;
        //int c = getopt_long(argc, argv, "n:i:p:c:t:x:y:w:h:d:r:", long_options, &option_index);
        int c = getopt_long(argc, argv, "i:p:", long_options, &option_index);

        // Detect the end of the options.
        if (c == -1) break;

        if (!optarg)
        {
            qWarning("c = %d, optarg NULL!", c);
            print_help();
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
            int temp = atoi(optarg);
            if (temp <=0)
            {
                qCritical("Number of channels must be at least 1");
            }
            params.numChannels = temp;
            qWarning("setting number of channels = %u", params.numChannels);
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

        case 'r':
            params.preset = atoi(optarg);
            qWarning("setting preset = %d", params.preset);
            break;

        default:
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("UCSD Sonic Arts");
    QCoreApplication::setOrganizationDomain("sonicarts.ucsd.edu");
    QCoreApplication::setApplicationName("samugen-gui");

    SamUgenGui gui(params);
    gui.show();

    return a.exec();
}
