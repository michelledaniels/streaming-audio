/**
 * @file samugen/main.cpp
 * samugen: command-line application for unit generators streaming to SAM
 * @author Michelle Daniels
 * @date February 2013
 * @copyright UCSD 2013
 */

#include <getopt.h>
#include <signal.h>

#include <QtCore/QCoreApplication>

#include "sam_client.h"


using namespace sam;

void print_help()
{
    printf("Usage: [samugen --name or -n client name]\n");
    printf("--ip or -i SAM ip\n");
    printf("--port or -p SAM port\n");
    printf("--channels or -c number of channels to stream\n");
    printf("--type or -t rendering type\n");
    printf("[--x or -x initial x position coordinate]\n");
    printf("[--y or -y initial y position coordinate]\n");
    printf("[--width or -w initial width for SAM stream]\n");
    printf("[--height or -h initial height for SAM stream]\n");
    printf("[--depth or -d initial depth for SAM stream]\n");
    printf("[--preset or -r rendering preset]\n");
    printf("\nExample usage:\n");
    printf("samugen -n \"Example Client\" -i \"127.0.0.1\" -p 7770 -c 2 -t 0 -r 0\n");
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

bool sac_audio_callback(unsigned int numChannels, unsigned int nframes, float** out, void*)
{
    // play white noise
    for (unsigned int ch = 0; ch < numChannels; ch++)
    {
        // TODO: check for null buffers?
        for (unsigned int n = 0; n < nframes; n++)
        {
            out[ch][n] = rand() / (float)RAND_MAX;
        }
    }

    return true;
}

void sac_mute_callback(bool mute, void*)
{
    qWarning("samugen: mute callback called, mute = %d", mute);
}

void sac_solo_callback(bool solo, void*)
{
    qWarning("samugen: solo callback called, solo = %d", solo);
}

void sac_disconnect_callback(void*)
{
    qWarning("samugen: lost connection with SAM, shutting down...");
    QCoreApplication::quit();
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
            {"preset", optional_argument, NULL, 'r'},
            {NULL, 0, NULL, 0}
        };

        // getopt_long stores the option index here.
        int option_index = 0;
        int c = getopt_long(argc, argv, "n:i:p:c:t:x:y:w:h:d:r:", long_options, &option_index);

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

    // register SAC callback
    sac.setAudioCallback(sac_audio_callback, NULL);

    // register other callbacks
    sac.setMuteCallback(sac_mute_callback, NULL);
    sac.setSoloCallback(sac_solo_callback, NULL);
    sac.setDisconnectCallback(sac_disconnect_callback, NULL);
    
    return a.exec();
}
