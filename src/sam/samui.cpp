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
#include "samparams.h"
#include "ui_samui.h"

namespace sam
{

static const int STATUS_BAR_TIMEOUT = 0; //2000;

SamUI::SamUI(const SamParams& params, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SamUI),
    m_sam(NULL),
    m_master(NULL),
    m_samButton(NULL),
    m_clientGroup(NULL),
    m_clientLayout(NULL),
    m_oscDirections(NULL),
    m_maxClients(params.maxClients),
    m_clients(NULL),
    m_maxClientDelayMillis(params.maxClientDelayMillis)
{
    ui->setupUi(this);

    setWindowTitle("Streaming Audio Manager");
    setMinimumSize(400, 700);

    m_sam = new StreamingAudioManager(params);
    m_samThread = new QThread();
    m_sam->moveToThread(m_samThread);
    m_samThread->start();

    connect(m_sam, SIGNAL(startupError()), this, SLOT(onSamStartupError()));
    connect(m_sam, SIGNAL(appAdded(int)), this, SLOT(addClient(int)));
    connect(m_sam, SIGNAL(appRemoved(int)), this, SLOT(removeClient(int)));
    connect(m_sam, SIGNAL(appVolumeChanged(int, float)), this, SLOT(setAppVolume(int,float)));
    connect(m_sam, SIGNAL(appMuteChanged(int, bool)), this, SLOT(setAppMute(int, bool)));
    connect(m_sam, SIGNAL(appSoloChanged(int, bool)), this, SLOT(setAppSolo(int, bool)));
    connect(m_sam, SIGNAL(appDelayChanged(int, float)), this, SLOT(setAppDelay(int,float)));
    connect(m_sam, SIGNAL(appPositionChanged(int, int, int, int, int, int)), this, SLOT(setAppPosition(int,int,int,int,int,int)));
    connect(m_sam, SIGNAL(appTypeChanged(int, int, int)), this, SLOT(setAppType(int, int, int)));
    connect(m_sam, SIGNAL(appMeterChanged(int, int, float, float, float, float)), this, SLOT(setAppMeter(int, int, float, float, float, float)));
    connect(m_sam, SIGNAL(started()), this, SLOT(onSamStarted()));
    connect(m_sam, SIGNAL(stopped()), this, SLOT(onSamStopped()));

    connect(this, SIGNAL(startSam()), m_sam, SLOT(start()));
    connect(this, SIGNAL(stopSam()), m_sam, SLOT(stop()));

    m_samButton = new QPushButton(QString("Start SAM"), this);
    connect(m_samButton, SIGNAL(clicked()), this, SLOT(onSamButtonClicked()));

    m_oscDirections = new QLabel(this);

    m_master = new MasterWidget(1.0f, false, 0.0f, params.maxDelayMillis, this);

    m_clients = new ClientWidget*[m_maxClients];
    for (int i = 0; i < m_maxClients; i++)
    {
        m_clients[i] = NULL;
    }

    QGroupBox* mainBox = new QGroupBox(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(mainBox);
    mainBox->setLayout(mainLayout);
    mainLayout->addWidget(m_samButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_oscDirections, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_master, 0, Qt::AlignCenter);

    QScrollArea* clientScrollArea = new QScrollArea(mainBox);
    clientScrollArea->setWidgetResizable(true);
    m_clientGroup = new QGroupBox(clientScrollArea);
    m_clientLayout = new QHBoxLayout(m_clientGroup);
    m_clientLayout->setMargin(0);
    m_clientGroup->setLayout(m_clientLayout);

    clientScrollArea->setWidget(m_clientGroup);
    mainLayout->addWidget(clientScrollArea);
    clientScrollArea->setMinimumSize(400, 300);
    setCentralWidget(mainBox);
}

SamUI::~SamUI()
{
    delete ui;

    if (m_samThread)
    {
        m_samThread->quit();
        if (!m_samThread->wait(5000))
        {
            qWarning("Timed out waiting for SAM thread to quit");
        }
    }

    if (m_sam)
    {
        connect(m_sam, SIGNAL(destroyed()), m_samThread, SLOT(deleteLater()));
        m_sam->deleteLater();
        m_sam = NULL;
    }
    else if (m_samThread)
    {
        delete m_samThread;
    }
}

void SamUI::doBeforeQuit()
{
    //qWarning("SamUI::doBeforeQuit");
    //if (m_sam) m_sam->stop();
}

void SamUI::addClient(int id)
{
    //qWarning("SamUI::addClient id = %d", id);
    if (id < 0 || id >= m_maxClients)
    {
        qWarning("SamUI::addClient received invalid id %d", id);
        return;
    }

    ClientParams params;
    if (!m_sam->getAppParams(id, params))
    {
        qWarning("SamUI::addClient couldn't get client parameters");
        return;
    }
    m_clients[id] = new ClientWidget(id, m_sam->getAppName(id), params, m_maxClientDelayMillis, this);
    connect_client(id);
    m_clientLayout->addWidget(m_clients[id]);
    
    QStatusBar* sb = statusBar();
    QString msg("Client added with ID ");
    msg.append(QString::number(id));
    sb->showMessage(msg, STATUS_BAR_TIMEOUT);
}

void SamUI::removeClient(int id)
{
    //qWarning("SamUI::removeClient id = %d", id);

    if (id < 0 || id >= m_maxClients)
    {
        qWarning("SamUI::addClient received invalid id %d", id);
        return;
    }

    if (m_clients[id])
    {
        m_clientLayout->removeWidget(m_clients[id]);
        delete m_clients[id];
        m_clients[id] = NULL;
    }
    else
    {
        qWarning("SamUI::removeClient tried to remove non-existent client widget");
    }
    
    QStatusBar* sb = statusBar();
    QString msg("Client removed with ID ");
    msg.append(QString::number(id));
    sb->showMessage(msg, STATUS_BAR_TIMEOUT);
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

void SamUI::setAppType(int id, int type, int preset)
{
    if (m_clients[id])
    {
        m_clients[id]->setType(type, preset);
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
            emit startSam();
            QStatusBar* sb = statusBar();
            sb->showMessage("Starting SAM...", STATUS_BAR_TIMEOUT);
        }
        else
        {
            qWarning("SamUI::onSamButtonClicked stopping SAM");
            emit stopSam();
            QStatusBar* sb = statusBar();
            sb->showMessage("Stopping SAM...", STATUS_BAR_TIMEOUT);
        }
    }
    else
    {
        QStatusBar* sb = statusBar();
        sb->showMessage("SAM not initialized - cannot start.", STATUS_BAR_TIMEOUT);
        int ret = QMessageBox::critical(this, "Streaming Audio Manager Error", "SAM not initialized - cannot start.");
    }
}

void SamUI::onSamStartupError()
{
    emit stopSam();

    m_samButton->setText("Start SAM");

    int ret = QMessageBox::critical(this, "Streaming Audio Manager Error", "Error starting SAM.");
    
    QStatusBar* sb = statusBar();
    sb->showMessage("Error starting SAM", STATUS_BAR_TIMEOUT);
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
    connect(m_clients[id], SIGNAL(delayChanged(int, float)), m_sam, SLOT(setAppDelay(int, float)));
}

void SamUI::disconnect_client(int id)
{
    disconnect(m_clients[id], SIGNAL(volumeChanged(int, float)), m_sam, SLOT(setAppVolume(int, float)));
    disconnect(m_clients[id], SIGNAL(muteChanged(int, bool)), m_sam, SLOT(setAppMute(int, bool)));
    disconnect(m_clients[id], SIGNAL(soloChanged(int, bool)), m_sam, SLOT(setAppSolo(int, bool)));
    disconnect(m_clients[id], SIGNAL(delayChanged(int, float)), m_sam, SLOT(setAppDelay(int, float)));
}

void SamUI::onSamStarted()
{
    m_sam->setVolume(m_master->getVolume());
    m_sam->setMute(m_master->getMute());
    m_sam->setDelay(m_master->getDelay());

    // messages from master widget to SAM
    connect(m_master, SIGNAL(volumeChanged(float)), m_sam, SLOT(setVolume(float)));
    connect(m_master, SIGNAL(muteChanged(bool)), m_sam, SLOT(setMute(bool)));
    connect(m_master, SIGNAL(delayChanged(float)), m_sam, SLOT(setDelay(float)));

    // messages from SAM to master widget
    connect(m_sam, SIGNAL(volumeChanged(float)), m_master, SLOT(setVolume(float)));
    connect(m_sam, SIGNAL(muteChanged(bool)), m_master, SLOT(setMute(bool)));
    connect(m_sam, SIGNAL(delayChanged(float)), m_master, SLOT(setDelay(float)));

    m_samButton->setText("Stop SAM");
    QStatusBar* sb = statusBar();
    sb->showMessage("Started SAM", STATUS_BAR_TIMEOUT);

    m_oscDirections->setText(m_sam->getOscMessageString());
}

void SamUI::onSamStopped()
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
    m_oscDirections->setText("");
    QStatusBar* sb = statusBar();
    sb->showMessage("Stopped SAM", STATUS_BAR_TIMEOUT);
}

} // end of namespace SAM
