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

SamUI::SamUI(const SamParams& params, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SamUI),
    m_sam(NULL),
    m_samButton(NULL)
{
    ui->setupUi(this);

    setWindowTitle("Streaming Audio Manager");

    m_sam = new StreamingAudioManager(params);

    m_samButton = new QPushButton(QString("Start SAM"), this);
    connect(m_samButton, SIGNAL(clicked()), this, SLOT(on_samButton_clicked()));

    MasterWidget* master = new MasterWidget(1.0f, false, 0.0f, this);
    connect(master, SIGNAL(volumeChanged(float)), m_sam, SLOT(setVolume(float)));
    connect(master, SIGNAL(muteChanged(bool)), m_sam, SLOT(setMute(bool)));

    ClientWidget* client1 = new ClientWidget(0, "client 1", 2, 1.0f, false, false, 0.0f, 0, 0, 0, 0, 0, this);
    ClientWidget* client2 = new ClientWidget(1, "client 2", 2, 0.5f, false, false, 0.0f, 0, 0, 0, 0, 0, this);

    QScrollArea* scrollArea = new QScrollArea(this);
    QGroupBox* groupBox1 = new QGroupBox(scrollArea);
    QVBoxLayout* layout1 = new QVBoxLayout(groupBox1);
    groupBox1->setLayout(layout1);
    layout1->addWidget(m_samButton);
    layout1->addWidget(master);
    layout1->setAlignment(master, Qt::AlignHCenter);

    QGroupBox* groupBox2 = new QGroupBox(groupBox1);
    QHBoxLayout* layout2 = new QHBoxLayout(groupBox2);
    groupBox2->setLayout(layout2);
    layout2->addWidget(client1);
    layout2->addWidget(client2);

    layout1->addWidget(groupBox2);

    scrollArea->setWidget(groupBox1);
    setCentralWidget(scrollArea);

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

void SamUI::on_samButton_clicked()
{
    if (m_sam)
    {
        if (!m_sam->isRunning())
        {
            qWarning("SamUI::on_samButton_clicked starting SAM");
            m_sam->start();
            m_samButton->setText("Stop SAM");
        }
        else
        {
            qWarning("SamUI::on_samButton_clicked stopping SAM");
            if (!m_sam->stop())
            {
                qWarning("SamUI::on_samButton_clicked couldn't stop SAM");
            }
            else
            {
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
