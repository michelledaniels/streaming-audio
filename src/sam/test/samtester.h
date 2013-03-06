#ifndef SAMTESTER_H
#define SAMTESTER_H

#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QList>

#include "sam_client.h"

static const char* CLIENT_NAME = "test client";

using namespace sam;

class SamTester : public QObject
{
    Q_OBJECT
public:
    SamTester(const char* samAddress, quint16 samPort, int interval, QObject* parent) :
        QObject(parent),
        m_samAddress(NULL),
        m_samPort(samPort)
    {
        int len = strlen(samAddress) + 1;
        m_samAddress = new char[len];
        strncpy(m_samAddress, samAddress, len);

        qsrand(QTime::currentTime().msec());

        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(testSam()));
        timer->start(interval);
    }

    ~SamTester()
    {
        if (!m_clients.isEmpty())
        {
            for (int i = 0; i < m_clients.size(); i++)
            {
                delete m_clients[i];
                m_clients[i] = NULL;
            }
            m_clients.clear();
        }
    }

public slots:
    void testSam()
    {
        double val = qrand() / (double)RAND_MAX;
        double valIndex = qrand() / (double)RAND_MAX;
        int index = valIndex * m_clients.size();
        qDebug("SamTester::testSam() val = %f", val);
        double numOptions = 7.0;

        if (val > (6/numOptions) && !m_clients.isEmpty())
        {
            // unregister a client
            qDebug("SamTester::testSam() unregistering client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index])
            {
                delete m_clients[index];
                m_clients[index] = NULL;
                m_clients.removeAt(index);
            }
        }
        else if (val > (5/numOptions) && val <= (6/numOptions) && !m_clients.isEmpty())
        {
            // change a client's mute status
            qDebug("SamTester::testSam() changing mute status for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setMute(val > 0.5 ? 0: 1);
        }
        else if (val > (4/numOptions) && val <= (5/numOptions) && !m_clients.isEmpty())
        {
            // change a client's volume
            qDebug("SamTester::testSam() changing volume for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setVolume(val);
        }
        else if (val > (3/numOptions) && val <= (4/numOptions) && !m_clients.isEmpty())
        {
            // change a client's solo status
            qDebug("SamTester::testSam() changing solo status for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setSolo(val > 0.5 ? 0: 1);
        }
        else if (val > (2/numOptions) && val <= (3/numOptions) && !m_clients.isEmpty())
        {
            // change a client's delay
            qDebug("SamTester::testSam() changing delay for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setDelay(val * 100);
        }
        else
        {
            // register a client
            qDebug("SamTester::testSam() registering a client");
            StreamingAudioClient* client = new StreamingAudioClient(2, TYPE_BASIC, CLIENT_NAME, m_samAddress, m_samPort);
            if (client->start(0, 0, 0, 0, 0) != SAC_SUCCESS)
            {
                qDebug("SamTester::testSam() ERROR: couldn't register a client.");
                delete client;
            }
            else
            {
                m_clients.append(client);
            }
        }
    }

private:

    char* m_samAddress;
    quint16 m_samPort;
    QList<StreamingAudioClient*> m_clients;

};



#endif // SAMTESTER_H