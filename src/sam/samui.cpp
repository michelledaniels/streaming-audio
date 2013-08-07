/**
 * @file samui.cpp
 * SAM GUI implementation
 * @author Michelle Daniels
 * @copyright UCSD 2012
 */

#include <QMessageBox>
#include <QPushButton>

#include "clientwidget.h"
#include "masterwidget.h"
#include "samui.h"
#include "ui_samui.h"

namespace sam
{

SamUI::SamUI(const SamParams& params, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SamUI),
    m_sam(NULL),
    m_master(NULL),
    m_samButton(NULL),
    m_clientGroup(NULL),
    m_clientLayout(NULL),
    m_maxClients(params.maxClients),
    m_clients(NULL)
{
    ui->setupUi(this);

    setWindowTitle("Streaming Audio Manager");

    m_sam = new StreamingAudioManager(params);

    connect(m_sam, SIGNAL(appAdded(int)), this, SLOT(addClient(int)));
    connect(m_sam, SIGNAL(appRemoved(int)), this, SLOT(removeClient(int)));
    connect(m_sam, SIGNAL(appVolumeChanged(int, float)), this, SLOT(setAppVolume(int,float)));
    connect(m_sam, SIGNAL(appMuteChanged(int, bool)), this, SLOT(setAppMute(int, bool)));
    connect(m_sam, SIGNAL(appSoloChanged(int, bool)), this, SLOT(setAppSolo(int, bool)));
    connect(m_sam, SIGNAL(appDelayChanged(int, float)), this, SLOT(setAppDelay(int,float)));
    connect(m_sam, SIGNAL(appPositionChanged(int, int, int, int, int, int)), this, SLOT(setAppPosition(int,int,int,int,int,int)));
    connect(m_sam, SIGNAL(appTypeChanged(int, int)), this, SLOT(setAppType(int, int)));
    connect(m_sam, SIGNAL(appMeterChanged(int, int, float, float, float, float)), this, SLOT(setAppMeter(int, int, float, float, float, float)));

    m_samButton = new QPushButton(QString("Start SAM"), this);
    connect(m_samButton, SIGNAL(clicked()), this, SLOT(onSamButtonClicked()));

    m_master = new MasterWidget(1.0f, false, 0.0f, this);

    m_clients = new ClientWidget*[m_maxClients];
    for (int i = 0; i < m_maxClients; i++)
    {
        m_clients[i] = NULL;
    }

    QGroupBox* mainBox = new QGroupBox(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(mainBox);
    mainBox->setLayout(mainLayout);
    mainLayout->addWidget(m_samButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_master, 0, Qt::AlignCenter);

    QScrollArea* clientScrollArea = new QScrollArea(mainBox);
    clientScrollArea->setWidgetResizable(true);
    m_clientGroup = new QGroupBox(clientScrollArea);
    m_clientLayout = new QHBoxLayout(m_clientGroup);
    m_clientGroup->setLayout(m_clientLayout);

    clientScrollArea->setWidget(m_clientGroup);
    mainLayout->addWidget(clientScrollArea);
    clientScrollArea->setMinimumSize(400, 300);
    setCentralWidget(mainBox);

}

SamUI::~SamUI()
{
    delete ui;

    if (m_sam)
    {
        delete m_sam;
        m_sam = NULL;
    }
}

void SamUI::doBeforeQuit()
{
    if (m_sam) m_sam->stop();
}

void SamUI::addClient(int id)
{
    qWarning("SamUI::addClient id = %d", id);
    if (id < 0 || id >= m_maxClients)
    {
        qWarning("SamUI::addClient received invalid id %d", id);
        return;
    }

    if (m_clients[id])
    {
        ClientParams params;
        if (!m_sam->getAppParams(id, params))
        {
            qWarning("SamUI::addClient couldn't get client parameters");
            return;
        }
        m_clientLayout->addWidget(m_clients[id], 0, Qt::AlignCenter);
        m_clients[id]->setName(m_sam->getAppName(id));
        m_clients[id]->setVolume(params.volume);
        m_clients[id]->setMute(params.mute);
        m_clients[id]->setSolo(params.solo);
        m_clients[id]->setDelay(params.delay);
        m_clients[id]->setPosition(params.pos.x, params.pos.y, params.pos.width, params.pos.height, params.pos.depth);

        connect_client(id);
        m_clients[id]->show();
    }
    else
    {
        ClientParams params;
        if (!m_sam->getAppParams(id, params))
        {
            qWarning("SamUI::addClient couldn't get client parameters");
            return;
        }
        m_clients[id] = new ClientWidget(id, m_sam->getAppName(id), params, this);
        connect_client(id);
        m_clientLayout->addWidget(m_clients[id]);
    }
}

void SamUI::removeClient(int id)
{
    qWarning("SamUI::removeClient id = %d", id);

    if (id < 0 || id >= m_maxClients)
    {
        qWarning("SamUI::addClient received invalid id %d", id);
        return;
    }

    if (m_clients[id])
    {
        m_clients[id]->hide();
        m_clientLayout->removeWidget(m_clients[id]);
    }
    else
    {
        qWarning("SamUI::removeClient tried to remove non-existent client widget");
    }
}

void SamUI::setAppVolume(int id, float volume)
{
    if (m_clients[id])
    {
        m_clients[id]->setVolume(volume);
    }
}

void SamUI::setAppMute(int id, bool mute)
{
    if (m_clients[id])
    {
        m_clients[id]->setMute(mute);
    }
}

void SamUI::setAppSolo(int id, bool solo)
{
    if (m_clients[id])
    {
        m_clients[id]->setSolo(solo);
    }
}

void SamUI::setAppDelay(int id, float delay)
{
    if (m_clients[id])
    {
        m_clients[id]->setDelay(delay);
    }
}

void SamUI::setAppPosition(int id, int x, int y, int width, int height, int depth)
{
    if (m_clients[id])
    {
        m_clients[id]->setPosition(x, y, width, height, depth);
    }
}

void SamUI::setAppType(int id, int type)
{
    if (m_clients[id])
    {
        m_clients[id]->setType(type);
    }
}

void SamUI::setAppMeter(int id, int ch, float rmsIn, float peakIn, float rmsOut, float peakOut)
{
    if (m_clients[id])
    {
        m_clients[id]->setMeter(ch, rmsIn, peakIn, rmsOut, peakOut);
    }
}

void SamUI::onSamButtonClicked()
{
    if (m_sam)
    {
        if (!m_sam->isRunning())
        {
            qWarning("SamUI::onSamButtonClicked starting SAM");
            m_sam->start();
            m_sam->setVolume(m_master->getVolume());
            m_sam->setMute(m_master->getMute());

            // messages from master widget to SAM
            connect(m_master, SIGNAL(volumeChanged(float)), m_sam, SLOT(setVolume(float)));
            connect(m_master, SIGNAL(muteChanged(bool)), m_sam, SLOT(setMute(bool)));
            connect(m_master, SIGNAL(delayChanged(float)), m_sam, SLOT(setDelay(float)));

            // messages from SAM to master widget
            connect(m_sam, SIGNAL(volumeChanged(float)), m_master, SLOT(setVolume(float)));
            connect(m_sam, SIGNAL(muteChanged(bool)), m_master, SLOT(setMute(bool)));
            connect(m_sam, SIGNAL(delayChanged(float)), m_master, SLOT(setDelay(float)));

            m_samButton->setText("Stop SAM");
        }
        else
        {
            qWarning("SamUI::onSamButtonClicked stopping SAM");
            if (!m_sam->stop())
            {
                qWarning("SamUI::onSamButtonClicked couldn't stop SAM");
            }
            else
            {
                // messages from master widget to SAM
                disconnect(m_master, SIGNAL(volumeChanged(float)), m_sam, SLOT(setVolume(float)));
                disconnect(m_master, SIGNAL(muteChanged(bool)), m_sam, SLOT(setMute(bool)));
                disconnect(m_master, SIGNAL(delayChanged(float)), m_sam, SLOT(setDelay(float)));

                // messages from SAM to master widget
                disconnect(m_sam, SIGNAL(volumeChanged(float)), m_master, SLOT(setVolume(float)));
                disconnect(m_sam, SIGNAL(muteChanged(bool)), m_master, SLOT(setMute(bool)));
                disconnect(m_sam, SIGNAL(delayChanged(float)), m_master, SLOT(setDelay(float)));

                m_samButton->setText("Start SAM");
            }
        }
    }
    else
    {
        // TODO: display error
    }
}

void SamUI::on_actionAbout_triggered()
{
    QMessageBox msgBox;
    QString about("Streaming Audio Manager");
    msgBox.setText(about);
    QString info("Version ");
    info.append(QString::number(sam::VERSION_MAJOR));
    info.append(".");
    info.append(QString::number(sam::VERSION_MINOR));
    info.append(".");
    info.append(QString::number(sam::VERSION_PATCH));
    msgBox.setText(about);
    info.append("\nBuilt on ");
    info.append(__DATE__);
    info.append(" at ");
    info.append(__TIME__);
    info.append("\nCopyright UCSD 2011-2013\n");
    msgBox.setInformativeText(info);
    msgBox.exec();
}

void SamUI::connect_client(int id)
{
    connect(m_clients[id], SIGNAL(volumeChanged(int, float)), m_sam, SLOT(setAppVolume(int, float)));
    connect(m_clients[id], SIGNAL(muteChanged(int, bool)), m_sam, SLOT(setAppMute(int, bool)));
    connect(m_clients[id], SIGNAL(soloChanged(int, bool)), m_sam, SLOT(setAppSolo(int, bool)));
}

void SamUI::disconnect_client(int id)
{
    disconnect(m_clients[id], SIGNAL(volumeChanged(int, float)), m_sam, SLOT(setAppVolume(int, float)));
    disconnect(m_clients[id], SIGNAL(muteChanged(int, bool)), m_sam, SLOT(setAppMute(int, bool)));
    disconnect(m_clients[id], SIGNAL(soloChanged(int, bool)), m_sam, SLOT(setAppSolo(int, bool)));
}

} // end of namespace SAM
