/**
 * @file clientwidget.h
 * Widgets for SAM clients in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#include <QtGui>

#include "clientwidget.h"

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

ClientWidget::ClientWidget(int id, const char* name, int channels, float volume, bool mute, bool solo, float delay, int x, int y, int w, int h, int d, QWidget *parent) :
    QWidget(parent),
    m_channels(channels),
    m_id(id),
    m_volumeSlider(NULL),
    m_muteCheckBox(NULL),
    m_soloCheckBox(NULL)
{
    m_name.append(name);
    setMinimumSize(280, 400);

    QScrollArea *scrollArea = new QScrollArea(this);
    //scrollArea->setMinimumSize(400, 400);
    QGroupBox *clientBox = new QGroupBox(scrollArea);
    QVBoxLayout* clientLayout = new QVBoxLayout(clientBox);

    // add name label
    QLabel* nameLabel = new QLabel(this);
    nameLabel->setText(m_name);
    nameLabel->setAlignment(Qt::AlignHCenter);
    QFont font = nameLabel->font();
    int pointSize = font.pointSize();
    font.setPointSize(pointSize + 2);
    font.setBold(true);
    nameLabel->setFont(font);
    clientLayout->addWidget(nameLabel);

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
    m_volumeSlider->setValue(volume * VOLUME_SLIDER_SCALE);
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(on_volumeSlider_valueChanged(int)));
    levelLayout->addWidget(m_volumeSlider);

    // add meter widgets
    MeterWidget** meters = new MeterWidget*[m_channels];
    for (int i = 0; i < m_channels; i++)
    {
        meters[i] = new MeterWidget(this);
        meters[i]->setMinimumSize(50,100);
        meters[i]->setLevel(0.5f, 1.0f);
        levelLayout->addWidget(meters[i]);
    }

    levelBox->setLayout(levelLayout);
    clientLayout->addWidget(levelBox);

    // add mute/solo checkboxes
    m_muteCheckBox = new QCheckBox(QString("Mute"), this);
    m_muteCheckBox->setChecked(mute);
    clientLayout->addWidget(m_muteCheckBox);
    connect(m_muteCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_muteCheckBox_toggled(bool)));
    m_soloCheckBox = new QCheckBox(QString("Solo"), this);
    m_soloCheckBox->setChecked(solo);
    clientLayout->addWidget(m_soloCheckBox);
    connect(m_soloCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_soloCheckBox_toggled(bool)));

    clientBox->setLayout(clientLayout);
    scrollArea->setWidget(clientBox);
}

void ClientWidget::on_volumeSlider_valueChanged(int val)
{
    float fval = val / (float)VOLUME_SLIDER_SCALE;
    qWarning("ClientWidget::on_volumeSlider_valueChanged fval = %f", fval);
    emit volumeChanged(fval);
}

void ClientWidget::on_muteCheckBox_toggled(bool checked)
{
    qWarning("ClientWidget::on_muteCheckBox_toggled checked = %d", checked);
    emit muteChanged(checked);
}

void ClientWidget::on_soloCheckBox_toggled(bool checked)
{
    qWarning("ClientWidget::on_soloCheckBox_toggled checked = %d", checked);
    emit soloChanged(checked);
}
