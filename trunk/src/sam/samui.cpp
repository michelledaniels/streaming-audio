/**
 * @file samui.cpp
 * SAM GUI implementation
 * @author Michelle Daniels
 * @copyright UCSD 2012
 */

#include <QMessageBox>

#include "samui.h"
#include "ui_samui.h"

SamUI::SamUI(const SamParams& params, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SamUI),
    m_sam(NULL)
{
    ui->setupUi(this);

    setWindowTitle("SAGE Audio Manager");

    m_sam = new StreamingAudioManager(params);
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

void SamUI::on_startSamButton_clicked()
{
    if (m_sam)
    {
        if (!m_sam->isRunning())
        {
            qWarning("SamUI::on_startSamButton_clicked starting SAM");
            m_sam->start();
            ui->startSamButton->setText("Stop SAM");
        }
        else
        {
            qWarning("SamUI::on_startSamButton_clicked stopping SAM");
            if (!m_sam->stop())
            {

            }
            else
            {
                ui->startSamButton->setText("Start SAM");
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

void SamUI::on_volumeSlider_valueChanged(int value)
{
    float volume = (value - ui->volumeSlider->minimum()) / (float)(ui->volumeSlider->maximum() - ui->volumeSlider->minimum());
    m_sam->setVolume(volume);
}

void SamUI::on_muteCheckBox_stateChanged(int arg1)
{
    m_sam->setMute(arg1);
}
