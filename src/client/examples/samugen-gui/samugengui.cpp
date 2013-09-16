/**
 * @file samugen-gui/samugengui.cpp
 * @author Michelle Daniels
 * @date September 2013
 * @copyright UCSD 2013
 */

#include <QGroupBox>
#include <QMessageBox>
#include <QVBoxLayout>

#include "samugengui.h"

using namespace sam;

SamUgenGui::SamUgenGui(QWidget *parent) :
    QMainWindow(parent),
    m_client(NULL)
{
    setWindowTitle("Streaming Audio Client - Unit Generator");
    setMinimumSize(300, 300);

    m_clientButton = new QPushButton(QString("Start Client"), this);
    connect(m_clientButton, SIGNAL(clicked()), this, SLOT(onClientButtonClicked()));

    QGroupBox* mainBox = new QGroupBox(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(mainBox);
    mainBox->setLayout(mainLayout);
    mainLayout->addWidget(m_clientButton, 0, Qt::AlignCenter);

    // add mute checkbox
    m_muteCheckBox = new QCheckBox(QString("Mute"), this);
    m_muteCheckBox->setChecked(false);
    mainLayout->addWidget(m_muteCheckBox, 0, Qt::AlignCenter);
    connect(m_muteCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_muteCheckBox_toggled(bool)));

    // add solo checkbox
    m_soloCheckBox = new QCheckBox(QString("Solo"), this);
    m_soloCheckBox->setChecked(false);
    mainLayout->addWidget(m_soloCheckBox, 0, Qt::AlignCenter);
    connect(m_soloCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_soloCheckBox_toggled(bool)));

    setCentralWidget(mainBox);
}

SamUgenGui::~SamUgenGui()
{
    if (m_client)
    {
        delete m_client;
        m_client = NULL;
    }
}

void SamUgenGui::onClientButtonClicked()
{
    m_clientButton->setEnabled(false); // disable so we don't try to double-start or double-stop
    if (m_client) stop_client();
    else start_client();
}

void SamUgenGui::onClientDestroyed()
{
    m_clientButton->setText("Start Client");
    m_clientButton->setEnabled(true);
}

void SamUgenGui::on_muteCheckBox_toggled(bool checked)
{
    if (m_client) m_client->setMute(checked);
}

void SamUgenGui::on_soloCheckBox_toggled(bool checked)
{
    if (m_client) m_client->setSolo(checked);
}

void SamUgenGui::start_client()
{
    qWarning("SamUgenGui::start_client()");

    SacParams params;
    params.numChannels = 2;
    params.type = TYPE_BASIC;
    params.preset = 0;
    params.name = "samugen-gui";
    params.samIP = "127.0.0.1";
    params.samPort = 7770;
    params.replyIP = NULL;
    params.replyPort = 0;
    params.payloadType = PAYLOAD_PCM_16;
    params.driveExternally = false;
    params.packetQueueSize = 4;

    m_client = new StreamingAudioClient();

    if (m_client->init(params) != SAC_SUCCESS)
    {
        qWarning("Couldn't initialize client");
        QMessageBox::critical(this, "SAM Ugen GUI", "Error initializing client.");
        m_clientButton->setEnabled(true);
        delete m_client;
        m_client = NULL;
        return;
    }

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int depth = 0;
    if (m_client->start(x, y, width, height, depth) != SAC_SUCCESS)
    {
        qWarning("Couldn't start client");
        QMessageBox::critical(this, "SAM Ugen GUI", "Error starting client.");
        m_clientButton->setEnabled(true);
        delete m_client;
        m_client = NULL;
        return;
    }

    m_client->setMute(m_muteCheckBox->isChecked());
    m_client->setSolo(m_soloCheckBox->isChecked());

    // register SAC callback
    m_client->setAudioCallback(sac_audio_callback, this);

    // register other callbacks
    m_client->setMuteCallback(sac_mute_callback, this);
    m_client->setSoloCallback(sac_solo_callback, this);
    m_client->setDisconnectCallback(sac_disconnect_callback, this);

    m_clientButton->setText("Stop Client");
    m_clientButton->setEnabled(true);
}

void SamUgenGui::stop_client()
{
    qWarning("SamUgenGui::stop_client()");
    if (m_client)
    {
        delete m_client;
        m_client = NULL;
    }
    m_clientButton->setText("Start Client");
    m_clientButton->setEnabled(true);
}

bool SamUgenGui::onSacAudio(unsigned int numChannels, unsigned int nframes, float** out)
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

void SamUgenGui::onSacMute(bool mute)
{
    m_muteCheckBox->setChecked(mute);
}

void SamUgenGui::onSacSolo(bool solo)
{
    m_soloCheckBox->setChecked(solo);
}

void SamUgenGui::onSacDisconnect()
{
    qWarning("SamUgenGui: lost connection with SAM");
    QMessageBox::critical(this, "SAM Ugen GUI", "Client lost connection with SAM!  Stopping...");
    if (m_client)
    {
        // NOTE: we MUST use deleteLater since we can't delete the client while we're processing its callback
        connect(m_client, SIGNAL(destroyed()), this, SLOT(onClientDestroyed()));
        m_client->deleteLater();
        m_client = NULL;
    }
}

bool SamUgenGui::sac_audio_callback(unsigned int numChannels, unsigned int nframes, float** out, void* ugen)
{
    return ((SamUgenGui*)ugen)->onSacAudio(numChannels, nframes, out);
}

void SamUgenGui::sac_mute_callback(bool mute, void* ugen)
{
    ((SamUgenGui*)ugen)->onSacMute(mute);
}

void SamUgenGui::sac_solo_callback(bool solo, void* ugen)
{
    ((SamUgenGui*)ugen)->onSacSolo(solo);
}

void SamUgenGui::sac_disconnect_callback(void* ugen)
{
    ((SamUgenGui*)ugen)->onSacDisconnect();
}
