/**
 * @file samugen-gui/main.cpp
 * samugen-gui: gui application for unit generators streaming to SAM
 * @author Michelle Daniels
 * @date February 2013
 * @copyright UCSD 2013
 */

#include <getopt.h>
#include <signal.h>

#include <QApplication>

#include "sam_client.h"
#include "samugengui.h"

using namespace sam;

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
    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("UCSD Sonic Arts");
    QCoreApplication::setOrganizationDomain("sonicarts.ucsd.edu");
    QCoreApplication::setApplicationName("samugen-gui");

    SamUgenGui gui;
    gui.show();

    return a.exec();
}
