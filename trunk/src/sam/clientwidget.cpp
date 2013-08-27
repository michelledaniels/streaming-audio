/**
 * @file clientwidget.h
 * Widgets for SAM clients in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#include <QtGui>

#include "clientwidget.h"

namespace sam
{
static const int METER_NUM_TICKS = 5;
static const int METER_TICK_LENGTH = 5;
static const int METER_TICK_LABEL_LENGTH = 30;

static const int VOLUME_SLIDER_SCALE = 100;

MeterWidget::MeterWidget(QWidget *parent) :
    QWidget(parent),
    m_rms(0.0f),
    m_peak(0.0f)
{
}

void MeterWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    int w = width();
    int h = height();
    int halfTick = h / (METER_NUM_TICKS * 2 + 1);

    // draw bounding rectangle
    painter.setPen(Qt::black);
    int meterWidth = w - METER_TICK_LENGTH - METER_TICK_LABEL_LENGTH;
    int meterHeight = h - (2 * halfTick);
    painter.drawRect(METER_TICK_LENGTH + METER_TICK_LABEL_LENGTH, halfTick, meterWidth - 1, meterHeight - 1);

    // draw colored meter
    QLinearGradient gradient(0, 0, meterWidth, h);
    gradient.setColorAt(1.0, Qt::green);
    gradient.setColorAt(0.5, Qt::yellow);
    //gradient.setColorAt(0.25, Qt::yellow);
    gradient.setColorAt(0.0, Qt::red);
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRect(METER_TICK_LENGTH + METER_TICK_LABEL_LENGTH + 1, halfTick + (meterHeight - 3) * (1.0f - m_rms) + 1, meterWidth - 2, (meterHeight - 3) * m_rms + 1);

    // draw tick marks
    painter.setPen(Qt::black);
    for (int i = 0; i <= METER_NUM_TICKS; i++)
    {
        int tick = (meterHeight * i) / METER_NUM_TICKS + halfTick;
        float tickLabel = 1.0 - i / (float)METER_NUM_TICKS;
        painter.drawText(0, tick - halfTick, METER_TICK_LABEL_LENGTH, halfTick*2, Qt::AlignVCenter | Qt::AlignRight, QString::number(tickLabel));
        painter.drawLine(METER_TICK_LABEL_LENGTH, tick, METER_TICK_LABEL_LENGTH + METER_TICK_LENGTH - 1, tick);
    }
}

ClientWidget::ClientWidget(int id, const char* name, ClientParams& params, QWidget *parent) :
    QWidget(parent),
    m_channels(params.channels),
    m_id(id),
    m_nameLabel(NULL),
    m_volumeSlider(NULL),
    m_muteCheckBox(NULL),
    m_soloCheckBox(NULL),
    m_metersIn(NULL),
    m_metersOut(NULL)
{
    m_name.append(name);

    QGroupBox *clientBox = new QGroupBox(this);
    QVBoxLayout* clientLayout = new QVBoxLayout(clientBox);

    // add name label
    m_nameLabel = new QLabel(this);
    m_nameLabel->setText(m_name);
    m_nameLabel->setAlignment(Qt::AlignHCenter);
    QFont font = m_nameLabel->font();
    int pointSize = font.pointSize();
    font.setPointSize(pointSize + 2);
    font.setBold(true);
    m_nameLabel->setFont(font);
    clientLayout->addWidget(m_nameLabel);

    // add id label
    QLabel* idLabel = new QLabel(this);
    idLabel->setText("SAM client ID: " + QString::number(m_id));
    idLabel->setAlignment(Qt::AlignHCenter);
    clientLayout->addWidget(idLabel);

    QWidget *levelBox = new QWidget(clientBox);
    QHBoxLayout* levelLayout = new QHBoxLayout(levelBox);

    // add volume slider
    m_volumeSlider = new QSlider(Qt::Vertical, this);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(VOLUME_SLIDER_SCALE);
    m_volumeSlider->setValue(params.volume * VOLUME_SLIDER_SCALE);
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(on_volumeSlider_valueChanged(int)));
    levelLayout->addWidget(m_volumeSlider);

    // add meter widgets
    m_metersIn = new MeterWidget*[m_channels];
    for (int i = 0; i < m_channels; i++)
    {
        m_metersIn[i] = new MeterWidget(this);
        m_metersIn[i]->setMinimumSize(50,100);
        m_metersIn[i]->setLevel(0.0f, 0.0f);
        levelLayout->addWidget(m_metersIn[i]);
    }

    m_metersOut = new MeterWidget*[m_channels];
    for (int i = 0; i < m_channels; i++)
    {
        m_metersOut[i] = new MeterWidget(this);
        m_metersOut[i]->setMinimumSize(50,100);
        m_metersOut[i]->setLevel(0.0f, 0.0f);
        levelLayout->addWidget(m_metersOut[i]);
    }

    levelBox->setLayout(levelLayout);
    clientLayout->addWidget(levelBox);

    // add mute/solo checkboxes
    m_muteCheckBox = new QCheckBox(QString("Mute"), this);
    m_muteCheckBox->setChecked(params.mute);
    clientLayout->addWidget(m_muteCheckBox);
    connect(m_muteCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_muteCheckBox_toggled(bool)));
    m_soloCheckBox = new QCheckBox(QString("Solo"), this);
    m_soloCheckBox->setChecked(params.solo);
    clientLayout->addWidget(m_soloCheckBox);
    connect(m_soloCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_soloCheckBox_toggled(bool)));

    clientBox->setLayout(clientLayout);

    setMinimumSize(m_channels * 50, 400);
}

void ClientWidget::setName(const char* name)
{
    m_name.clear();
    m_name.append(name);
    m_nameLabel->setText(m_name);
}

void ClientWidget::setVolume(float volume)
{
    m_volumeSlider->setValue(volume * VOLUME_SLIDER_SCALE);
}

void ClientWidget::setMute(bool mute)
{
    m_muteCheckBox->setChecked(mute);
}

void ClientWidget::setSolo(bool solo)
{
    m_soloCheckBox->setChecked(solo);
}

void ClientWidget::setDelay(float delay)
{

}

void ClientWidget::setPosition(int x, int y, int w, int h, int d)
{

}

void ClientWidget::setType(int type)
{

}

void ClientWidget::setMeter(int ch, float rmsIn, float peakIn, float rmsOut, float peakOut)
{
    if (ch < 0 || ch >= m_channels)
    {
        qWarning("ClientWidget::setMeter tried to set meter for invalid channel: %d", ch);
        return;
    }

    if (m_metersIn && m_metersOut && m_metersIn[ch] && m_metersOut[ch])
    {
        m_metersIn[ch]->setLevel(rmsIn, peakIn);
        m_metersOut[ch]->setLevel(rmsOut, peakOut);
    }
    else
    {
        qWarning("ClientWidget::setMeter meter %d was null!", ch);
    }
}

void ClientWidget::on_volumeSlider_valueChanged(int val)
{
    float fval = val / (float)VOLUME_SLIDER_SCALE;
    //qWarning("ClientWidget::on_volumeSlider_valueChanged fval = %f", fval);
    emit volumeChanged(m_id, fval);
}

void ClientWidget::on_muteCheckBox_toggled(bool checked)
{
    //qWarning("ClientWidget::on_muteCheckBox_toggled checked = %d", checked);
    emit muteChanged(m_id, checked);
}

void ClientWidget::on_soloCheckBox_toggled(bool checked)
{
    //qWarning("ClientWidget::on_soloCheckBox_toggled checked = %d", checked);
    emit soloChanged(m_id, checked);
}

} // end of namespace SAM
