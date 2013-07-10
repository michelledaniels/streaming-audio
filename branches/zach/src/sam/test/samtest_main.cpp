#include <QtCore/QCoreApplication>

#include "samtester.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    const char* samIP = (argc < 2) ? "127.0.0.1" : argv[1];
    SamTester* tester = new SamTester(samIP, 7770, 200, &a);

    return a.exec();
}
