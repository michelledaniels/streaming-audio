/**
 * @file test/samtest_main.cpp
 * samtester: command-line application for testing SAM
 * @author Michelle Daniels
 * @date August 2013
 * @copyright UCSD 2013
 */

#include <getopt.h>
#include <signal.h>

#include <QtCore/QCoreApplication>

#include "samtester.h"

void print_help()
{
    printf("Usage: [samtest --ip or -i SAM ip]\n");
    printf("--port or -p SAM port\n");
    printf("--maxclients or -m max number of clients to register\n");
    printf("--channels or -c number of channels per client\n");
    printf("--interval or -t interval between adding clients (millis)\n");
    printf("--mode or -d testing mode (0 = stress test, 1 = parallel test)\n");
    printf("\nExample usage:\n");
    printf("samtest -i \"127.0.0.1\" -p 7770 -m 16 -c 2 -t 100 -d 0\n");
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
    QCoreApplication a(argc, argv);

    if (argc < 6)
    {
        print_help();
        exit(EXIT_SUCCESS);
    }

    char* samIP = NULL;
    quint16 samPort = 0;
    int channels = 0;
    int maxClients = 0;
    int interval = 0;
    int mode = 0;

    // parse command-line parameters
    while (true)
    {
        static struct option long_options[] = {
            // These options set a flag.
            {"ip", required_argument, NULL, 'i'},
            {"port", required_argument, NULL, 'p'},
            {"maxclients", required_argument, NULL, 'm'},
            {"channels", required_argument, NULL, 'c'},
            {"interval", required_argument, NULL, 't'},
            {"mode", required_argument, NULL, 'd'},
            {NULL, 0, NULL, 0}
        };

        // getopt_long stores the option index here.
        int option_index = 0;
        int c = getopt_long(argc, argv, "i:p:m:c:t:d:", long_options, &option_index);

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
        case 'i':
            samIP = optarg;
            qWarning("setting samIP = %s", samIP);
            break;

        case 'p':
            samPort = atoi(optarg);
            qWarning("setting samPort = %u", samPort);
            break;

        case 'm':
        {
            int temp = atoi(optarg);
            if (temp <=0)
            {
                qCritical("Number of clients must be at least 1");
            }
            maxClients = temp;
            qWarning("setting max number of clients = %d", maxClients);
            break;
        }

        case 'c':
        {
            int temp = atoi(optarg);
            if (temp <=0)
            {
                qCritical("Number of channels must be at least 1");
            }
            channels = temp;
            qWarning("setting number of channels = %d", channels);
            break;
        }

        case 't':
        {
            int temp = atoi(optarg);
            if (temp <= 0)
            {
                qCritical("interval must be at least 1");
            }
            interval = temp;
            qWarning("setting interval = %d milliseconds", interval);
            break;
        }

        case 'd':
        {
            int temp = atoi(optarg);
            if (temp < 0 || temp > 1)
            {
                qCritical("only modes 0 and 1 are defined");
            }
            mode = temp;
            qWarning("setting mode = %d", mode);
            break;
        }

        default:
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

    printf("finished parsing args");

    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);
    
    switch (mode)
    {
    case 0:
    {
        SamStressTester stressTester(samIP, samPort, interval, maxClients, channels, NULL);
        return a.exec();
    }
    case 1:
    default:
    {
        SamParallelTester parallelTester(samIP, samPort, interval, maxClients, channels, NULL);
        return a.exec();
    }    
    }
}
