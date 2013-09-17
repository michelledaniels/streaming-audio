/**
 * @file sam_main.cpp
 * Streaming Audio Manager (SAM) main function
 * @author Michelle Daniels
 * @date September 2011
 * @copyright UCSD 2011-2013
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>

#include "samui.h"
#include "sam.h"
#include "samparams.h"

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

int main(int argc, char* argv[])
{
    print_info();

    SamParams params;
    bool success = params.parseConfig("sam.conf", argc, argv);
    if (!success) exit(EXIT_FAILURE);
    if (params.printHelp)
    {
        print_help();
        exit(EXIT_SUCCESS);
    }

    // TODO: check that all arguments are valid, non-null??
    if (params.useGui) // run in GUI mode
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

    exit(EXIT_SUCCESS);
}
