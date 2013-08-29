/**
 * @file masterwidget.h
 * Widget for master controls in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#include <QtGui>

#include "masterwidget.h"

namespace sam
{

MasterWidget::MasterWidget(float volume, bool mute, float delay, float maxDelayMillis, QWidget *parent) :
    QWidget(parent),
    m_volumeSlider(NULL),
    m_muteCheckBox(NULL)
{
    setMinimumSize(250, 350);

    QGroupBox *masterGroup = new QGroupBox(this);
    QVBoxLayout* masterLayout = new QVBoxLayout(masterGroup);

    // add name label
    QLabel* nameLabel = new QLabel(this);
    nameLabel->setText("SAM Master Controls");
    nameLabel->setAlignment(Qt::AlignHCenter);
    QFont font = nameLabel->font();
    int pointSize = font.pointSize();
    font.setPointSize(pointSize + 2);
    font.setBold(true);
    nameLabel->setFont(font);
    masterLayout->addWidget(nameLabel);

    // add volume slider
    m_volumeSlider = new QSlider(Qt::Vertical, this);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(VOLUME_SLIDER_SCALE);
    m_volumeSlider->setValue(volume * VOLUME_SLIDER_SCALE);
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(on_volumeSlider_valueChanged(int)));
    QLabel* volumeNameLabel = new QLabel(QString("Volume"), this);
    m_volumeLabel = new QLabel(this);
    m_volumeLabel->setNum(volume);
    QWidget *volumeBox = new QWidget(masterGroup);
    volumeBox->setMinimumSize(60, 150);
    volumeBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QVBoxLayout* volumeLayout = new QVBoxLayout(volumeBox);
    volumeLayout->addWidget(volumeNameLabel, 0, Qt::AlignCenter);
    volumeLayout->addWidget(m_volumeSlider, 0, Qt::AlignCenter);
    volumeLayout->addWidget(m_volumeLabel, 0, Qt::AlignCenter);
    masterLayout->addWidget(volumeBox, 0, Qt::AlignCenter);

    // add mute checkbox
    m_muteCheckBox = new QCheckBox(QString("Mute"), this);
    m_muteCheckBox->setChecked(mute);
    masterLayout->addWidget(m_muteCheckBox, 0, Qt::AlignCenter);
    connect(m_muteCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_muteCheckBox_toggled(bool)));

    masterGroup->setLayout(masterLayout);

    // add delay spinbox
    QLabel* delayLabel = new QLabel(QString("Delay (ms)"), this);
    m_delaySpinBox = new QDoubleSpinBox(this);
    m_delaySpinBox->setMinimum(0.0);
    m_delaySpinBox->setMaximum(maxDelayMillis);
    m_delaySpinBox->setValue(delay);
    QWidget *delayBox = new QWidget(masterGroup);
    QHBoxLayout* delayLayout = new QHBoxLayout(delayBox);
    delayLayout->addWidget(delayLabel);
    delayLayout->addWidget(m_delaySpinBox);
    masterLayout->addWidget(delayBox, 0, Qt::AlignCenter);
    connect(m_delaySpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_delaySpinBox_valueChanged(double)));
}

void MasterWidget::setVolume(float volume)
{
    m_volumeSlider->setValue(volume * VOLUME_SLIDER_SCALE);
    m_volumeLabel->setNum(volume);
}

void MasterWidget::setMute(bool mute)
{
    m_muteCheckBox->setChecked(mute);
}

void MasterWidget::setDelay(float delay)
{
    m_delaySpinBox->setValue(delay);
}

void MasterWidget::on_volumeSlider_valueChanged(int val)
{
    float fval = val / (float)VOLUME_SLIDER_SCALE;
    //qWarning("MasterWidget::on_volumeSlider_valueChanged fval = %f", fval);
    m_volumeLabel->setNum(fval);
    emit volumeChanged(fval);
}

void MasterWidget::on_muteCheckBox_toggled(bool checked)
{
    //qWarning("MasterWidget::on_muteCheckBox_toggled checked = %d", checked);
    emit muteChanged(checked);
}

void MasterWidget::on_delaySpinBox_valueChanged(double val)
{
    //qWarning("MasterWidget::on_delaySpinBox_valueChanged val = %f", val);
    emit delayChanged((float)val);
}

} // end of namespace SAM
