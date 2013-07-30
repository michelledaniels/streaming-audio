/**
 * @file masterwidget.h
 * Widget for master controls in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#include <QtGui>

#include "masterwidget.h"

MasterWidget::MasterWidget(float volume, bool mute, float delay, QWidget *parent) :
    QWidget(parent),
    m_volumeSlider(NULL),
    m_muteCheckBox(NULL)
{
    setMinimumSize(200, 350);

    QGroupBox *groupBox1 = new QGroupBox(this);
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

    // add delay spinbox
    QLabel* delayLabel = new QLabel(QString("Delay (ms)"), this);
    m_delaySpinBox = new QDoubleSpinBox(this);
    m_delaySpinBox->setMinimum(0.0);
    m_delaySpinBox->setMaximum(1000.0);
    m_delaySpinBox->setValue(delay);
    QGroupBox *groupBox2 = new QGroupBox(groupBox1);
    //groupBox2->setStyleSheet("border:0;");
    QHBoxLayout* hLayout = new QHBoxLayout(groupBox2);
    hLayout->addWidget(delayLabel);
    hLayout->addWidget(m_delaySpinBox);
    vLayout->addWidget(groupBox2);
    connect(m_delaySpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_delaySpinBox_valueChanged(double)));
}

void MasterWidget::setVolume(float volume)
{
    m_volumeSlider->setValue(volume * VOLUME_SLIDER_SCALE);
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
