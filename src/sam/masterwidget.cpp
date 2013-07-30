/**
 * @file masterwidget.h
 * Widget for master controls in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#include <QtGui>

#include "masterwidget.h"

static const int VOLUME_SLIDER_SCALE = 100;

MasterWidget::MasterWidget(float volume, bool mute, float delay, QWidget *parent) :
    QWidget(parent),
    m_volumeSlider(NULL),
    m_muteCheckBox(NULL)
{
    setMinimumSize(200, 200);

    QScrollArea *scrollArea = new QScrollArea(this);
    QGroupBox *groupBox1 = new QGroupBox(scrollArea);
    QVBoxLayout* vLayout = new QVBoxLayout(groupBox1);

    // add name label
    QLabel* nameLabel = new QLabel(this);
    nameLabel->setText("SAM Master Controls");
    nameLabel->setAlignment(Qt::AlignHCenter);
    QFont font = nameLabel->font();
    int pointSize = font.pointSize();
    font.setPointSize(pointSize + 2);
    font.setBold(true);
    nameLabel->setFont(font);
    vLayout->addWidget(nameLabel);

    // add volume slider
    m_volumeSlider = new QSlider(Qt::Vertical, this);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(VOLUME_SLIDER_SCALE);
    m_volumeSlider->setValue(volume * VOLUME_SLIDER_SCALE);
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(on_volumeSlider_valueChanged(int)));
    vLayout->addWidget(m_volumeSlider);

    // add mute checkbox
    m_muteCheckBox = new QCheckBox(QString("Mute"), this);
    m_muteCheckBox->setChecked(mute);
    vLayout->addWidget(m_muteCheckBox);
    connect(m_muteCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_muteCheckBox_toggled(bool)));

    groupBox1->setLayout(vLayout);
    scrollArea->setWidget(groupBox1);
}

void MasterWidget::on_volumeSlider_valueChanged(int val)
{
    float fval = val / (float)VOLUME_SLIDER_SCALE;
    qWarning("MasterWidget::on_volumeSlider_valueChanged fval = %f", fval);
    emit volumeChanged(fval);
}

void MasterWidget::on_muteCheckBox_toggled(bool checked)
{
    qWarning("MasterWidget::on_muteCheckBox_toggled checked = %d", checked);
    emit muteChanged(checked);
}
