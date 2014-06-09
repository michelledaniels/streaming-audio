/**
 * @file samui.cpp
 * SAM GUI implementation
 * @author Michelle Daniels
 * @license 
 * This software is Copyright 2011-2014 The Regents of the University of California. 
 * All Rights Reserved.
 *
 * Permission to copy, modify, and distribute this software and its documentation for 
 * educational, research and non-profit purposes by non-profit entities, without fee, 
 * and without a written agreement is hereby granted, provided that the above copyright
 * notice, this paragraph and the following three paragraphs appear in all copies.
 *
 * Permission to make commercial use of this software may be obtained by contacting:
 * Technology Transfer Office
 * 9500 Gilman Drive, Mail Code 0910
 * University of California
 * La Jolla, CA 92093-0910
 * (858) 534-5815
 * invent@ucsd.edu
 *
 * This software program and documentation are copyrighted by The Regents of the 
 * University of California. The software program and documentation are supplied 
 * "as is", without any accompanying services from The Regents. The Regents does 
 * not warrant that the operation of the program will be uninterrupted or error-free. 
 * The end-user understands that the program was developed for research purposes and 
 * is advised not to rely exclusively on the program for any reason.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 * OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. THE UNIVERSITY OF
 * CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, 
 * AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 */

#include <QMessageBox>
#include <QPushButton>

#include "clientwidget.h"
#include "masterwidget.h"
#include "sam_shared.h"
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
    m_clients(NULL),
    m_samParams(params)
{
    ui->setupUi(this);

    setWindowTitle("Streaming Audio Manager");
    setMinimumSize(420, 790);
    
    m_samThread = new QThread();
    m_samThread->start();

    m_samButton = new QPushButton(QString("Start SAM"), this);
    connect(m_samButton, SIGNAL(clicked()), this, SLOT(onSamButtonClicked()));

    m_oscDirections = new QLabel(this);

    m_master = new MasterWidget(1.0f, false, 0.0f, params.maxDelayMillis, this);

    m_clients = new ClientWidget*[m_samParams.maxClients];
    for (int i = 0; i < m_samParams.maxClients; i++)
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
    clientScrollArea->setMinimumSize(400, 360);
    mainLayout->addWidget(clientScrollArea);
    
    //mainLayout->addStretch();
    
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
    if (id < 0 || id >= m_samParams.maxClients)
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
    m_clients[id] = new ClientWidget(id, m_sam->getAppName(id), params, m_samParams.maxClientDelayMillis, m_sam, this);
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

    if (id < 0 || id >= m_samParams.maxClients)
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

void SamUI::onSetAppTypeFailed(int id, int errorCode, int type, int preset, int typeOld, int presetOld)
{
    const RenderingType* typeStruct = m_sam->getType(type);
    const RenderingType* typeStructOld = m_sam->getType(typeOld);
    int ret = QMessageBox::critical(this, "Streaming Audio Manager Error", 
                                    "Could not set type for app " + QString::number(id) + " to " + typeStruct->name + ".\nError code = " + QString::number(errorCode) + ".\nReverting to " + typeStructOld->name + " type and " + typeStructOld->presets[preset].name + " preset.");
    
    QStatusBar* sb = statusBar();
    sb->showMessage("Set type request failed with error code " + QString::number(errorCode), 5000);
    
    if (m_clients[id])
    {
        m_clients[id]->setType(typeOld, presetOld);
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
    m_samButton->setEnabled(false); // disable so we don't try to double-start or double-stop
    if (m_sam) stop_sam();
    else start_sam();
}

void SamUI::onSamStartupError()
{
    if (m_sam) 
    {
        m_sam->deleteLater();
        m_sam = NULL;
    }
    m_samButton->setText("Start SAM");
    m_samButton->setEnabled(true);

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
    connect(m_clients[id], SIGNAL(typeChanged(int, int, int)), m_sam, SLOT(setAppType(int, int, int)));
    connect(m_sam, SIGNAL(typeAdded(int)), m_clients[id], SLOT(addType(int)));
}

void SamUI::disconnect_client(int id)
{
    disconnect(m_clients[id], SIGNAL(volumeChanged(int, float)), m_sam, SLOT(setAppVolume(int, float)));
    disconnect(m_clients[id], SIGNAL(muteChanged(int, bool)), m_sam, SLOT(setAppMute(int, bool)));
    disconnect(m_clients[id], SIGNAL(soloChanged(int, bool)), m_sam, SLOT(setAppSolo(int, bool)));
    disconnect(m_clients[id], SIGNAL(delayChanged(int, float)), m_sam, SLOT(setAppDelay(int, float)));
    disconnect(m_clients[id], SIGNAL(typeChanged(int, int, int)), m_sam, SLOT(setAppType(int, int, int)));
    disconnect(m_sam, SIGNAL(typeAdded(int)), m_clients[id], SLOT(addType(int)));
}

void SamUI::onSamStarted()
{
    m_sam->setVolume(m_master->getVolume());
    m_sam->setMute(m_master->getMute());
    m_sam->setDelay(m_master->getDelay());

    // messages from SAM to UI
    connect(m_sam, SIGNAL(appAdded(int)), this, SLOT(addClient(int)));
    connect(m_sam, SIGNAL(appRemoved(int)), this, SLOT(removeClient(int)));
    connect(m_sam, SIGNAL(appVolumeChanged(int, float)), this, SLOT(setAppVolume(int,float)));
    connect(m_sam, SIGNAL(appMuteChanged(int, bool)), this, SLOT(setAppMute(int, bool)));
    connect(m_sam, SIGNAL(appSoloChanged(int, bool)), this, SLOT(setAppSolo(int, bool)));
    connect(m_sam, SIGNAL(appDelayChanged(int, float)), this, SLOT(setAppDelay(int,float)));
    connect(m_sam, SIGNAL(appPositionChanged(int, int, int, int, int, int)), this, SLOT(setAppPosition(int,int,int,int,int,int)));
    connect(m_sam, SIGNAL(appTypeChanged(int, int, int)), this, SLOT(setAppType(int, int, int)));
    connect(m_sam, SIGNAL(appMeterChanged(int, int, float, float, float, float)), this, SLOT(setAppMeter(int, int, float, float, float, float)));
    connect(m_sam, SIGNAL(setAppTypeFailed(int, int, int, int, int, int)), this, SLOT(onSetAppTypeFailed(int, int, int, int, int, int)));
            
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
    m_samButton->setEnabled(true);
}

void SamUI::onSamStopped()
{
    // remove client widgets
    for (int i = 0; i < m_samParams.maxClients; i++)
    {
        if (m_clients[i])
        {
            m_clientLayout->removeWidget(m_clients[i]);
            delete m_clients[i];
            m_clients[i] = NULL;
        }
    }

    m_samButton->setText("Start SAM");
    m_oscDirections->setText("");
    QStatusBar* sb = statusBar();
    sb->showMessage("Stopped SAM", STATUS_BAR_TIMEOUT);
    m_samButton->setEnabled(true);
}

void SamUI::start_sam()
{
    //qWarning("SamUI::start_sam starting SAM");
    QStatusBar* sb = statusBar();
    sb->showMessage("Starting SAM...", STATUS_BAR_TIMEOUT);
    
    m_sam = new StreamingAudioManager(m_samParams);
    m_sam->moveToThread(m_samThread);
    
    connect(m_sam, SIGNAL(startupError()), this, SLOT(onSamStartupError()));
    connect(m_sam, SIGNAL(started()), this, SLOT(onSamStarted()));
    connect(m_sam, SIGNAL(destroyed()), this, SLOT(onSamStopped()));
    connect(this, SIGNAL(startSam()), m_sam, SLOT(start()));
    
    // tell SAM to start (on its thread)
    emit startSam();
}

void SamUI::stop_sam()
{
    //qWarning("SamUI::stop_sam stopping SAM");
    QStatusBar* sb = statusBar();
    sb->showMessage("Stopping SAM...", STATUS_BAR_TIMEOUT);
    m_sam->deleteLater();
    m_sam = NULL;
}

} // end of namespace SAM
