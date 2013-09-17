/**
 * @file sam/samtester.h
 * Routines for stress-testing SAM with multiple clients etc.
 * @author Michelle Daniels
 * @date September 2013
 * @copyright UCSD 2012-2013
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

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

// stress testing of client registering/unregistering/etc.
class SamStressTester : public QObject
{
    Q_OBJECT
public:
    SamStressTester(const char* samAddress, quint16 samPort, int interval, int maxClients, int channels, QObject* parent) :
        QObject(parent),
        m_samAddress(NULL),
        m_samPort(samPort),
        m_maxClients(maxClients),
        m_channels(channels)
    {
        int len = strlen(samAddress) + 1;
        m_samAddress = new char[len];
        strncpy(m_samAddress, samAddress, len);

        qsrand(QTime::currentTime().msec());

        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(stressTest()));
        timer->start(interval);
    }

    ~SamStressTester()
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
    void stressTest()
    {
        double val = qrand() / (double)RAND_MAX;
        double valIndex = qrand() / (double)RAND_MAX;
        int index = valIndex * m_clients.size();
        qDebug("SamStressTester::testSam() val = %f", val);
        double numOptions = 7.0;

        if (val > (6/numOptions) && !m_clients.isEmpty())
        {
            // unregister a client
            qDebug("SamStressTester::stressTest() unregistering client at index %d out of %d clients", index, m_clients.size());
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
            qDebug("SamStressTester::stressTest() changing mute status for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setMute(val > 0.5 ? 0: 1);
        }
        else if (val > (4/numOptions) && val <= (5/numOptions) && !m_clients.isEmpty())
        {
            // change a client's volume
            qDebug("SamStressTester::stressTest() changing volume for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setVolume(val);
        }
        else if (val > (3/numOptions) && val <= (4/numOptions) && !m_clients.isEmpty())
        {
            // change a client's solo status
            qDebug("SamStressTester::stressTest() changing solo status for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setSolo(val > 0.5 ? 0: 1);
        }
        else if (val > (2/numOptions) && val <= (3/numOptions) && !m_clients.isEmpty())
        {
            // change a client's delay
            qDebug("SamStressTester::stressTest() changing delay for client at index %d out of %d clients", index, m_clients.size());
            if (m_clients[index]) m_clients[index]->setDelay(val * 100);
        }
        else
        {
            if (m_clients.size() < m_maxClients)
            {
                // register a client
                qDebug("SamStressTester::stressTest() registering a client");
                StreamingAudioClient* client = new StreamingAudioClient();
                if (client->init(m_channels, TYPE_BASIC, CLIENT_NAME, m_samAddress, m_samPort) != SAC_SUCCESS)
                {
                    qWarning("SamStressTester::stressTest()  ERROR: couldn't intialize a client.");
                    delete client;
                }
                if (client->start(0, 0, 0, 0, 0) != SAC_SUCCESS)
                {
                    qWarning("SamStressTester::stressTest()  ERROR: couldn't register a client.");
                    delete client;
                }
                else
                {
                    m_clients.append(client);
                }
            }
            else
            {
                qWarning("SamStressTester::stressTest() max number of clients reached");
            }
        }
    }

private:

    char* m_samAddress;
    quint16 m_samPort;
    QList<StreamingAudioClient*> m_clients;
    int m_maxClients;
    int m_channels;
};

// testing of many clients in parallel
class SamParallelTester : public QObject
{
    Q_OBJECT
public:
    SamParallelTester(const char* samAddress, quint16 samPort, int interval, int maxClients, int channels, QObject* parent) :
        QObject(parent),
        m_samAddress(NULL),
        m_samPort(samPort)
    {
        int len = strlen(samAddress) + 1;
        m_samAddress = new char[len];
        strncpy(m_samAddress, samAddress, len);

        for (int i = 0; i < maxClients; i++)
        {
            // register a client
            StreamingAudioClient* client = new StreamingAudioClient();
            if (client->init(channels, TYPE_BASIC, CLIENT_NAME, m_samAddress, m_samPort) != SAC_SUCCESS)
            {
                qWarning("SamParallelTester ERROR: couldn't intialize a client.");
                delete client;
            }
            if (client->start(0, 0, 0, 0, 0) != SAC_SUCCESS)
            {
                qWarning("SamParallelTester ERROR: couldn't register a client.");
                delete client;
            }
            else
            {
                m_clients.append(client);
            }

            // wait until time to add next client (yes, there are more elegant ways to do this)
            QEventLoop loop;
            QTimer::singleShot(interval, &loop, SLOT(quit()));
            loop.exec();
        }
        qWarning("SamParallelTester finished adding clients.");
    }

    ~SamParallelTester()
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

private:

    char* m_samAddress;
    quint16 m_samPort;
    QList<StreamingAudioClient*> m_clients;
};


#endif // SAMTESTER_H
