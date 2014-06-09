/**
 * @file clientwidget.h
 * Widgets for SAM clients in SAM GUI
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
    painter.drawRect(METER_TICK_LENGTH + METER_TICK_LABEL_LENGTH + 1, halfTick + (meterHeight - 3) * (1.0f - m_rms) + 1, meterWidth - 2, (meterHeight - 3) * m_rms);

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

ClientWidget::ClientWidget(int id, const char* name, ClientParams& params, double maxDelayMillis, StreamingAudioManager* sam, QWidget *parent) :
    QWidget(parent),
    m_channels(params.channels),
    m_id(id),
    m_sam(sam),
    m_nameLabel(NULL),
    m_volumeSlider(NULL),
    m_muteCheckBox(NULL),
    m_soloCheckBox(NULL),
    m_delaySpinBox(NULL),
    m_typeComboBox(NULL),
    m_presetComboBox(NULL),
    m_metersIn(NULL),
    m_metersOut(NULL)
{
    m_name.append(name);

    QGroupBox *clientBox = new QGroupBox(this);
    QVBoxLayout* clientLayout = new QVBoxLayout(clientBox);
    clientLayout->setContentsMargins(5, 5, 5, 5);

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
    QGridLayout* levelLayout = new QGridLayout(levelBox);
    levelLayout->setContentsMargins(0, 0, 0, 0);

    // add volume slider
    m_volumeSlider = new QSlider(Qt::Vertical, this);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(VOLUME_SLIDER_SCALE);
    m_volumeSlider->setValue(params.volume * VOLUME_SLIDER_SCALE);
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(on_volumeSlider_valueChanged(int)));
    levelLayout->addWidget(m_volumeSlider, 1, 0, 1, 1, Qt::AlignCenter);

    QString volumeString("Volume");
    QLabel* volumeLabel = new QLabel(volumeString, this);
    levelLayout->addWidget(volumeLabel, 0, 0, 1, 1, Qt::AlignCenter);

    // add meter widgets
    m_metersIn = new MeterWidget*[m_channels];
    for (int i = 0; i < m_channels; i++)
    {
        m_metersIn[i] = new MeterWidget(this);
        m_metersIn[i]->setMinimumSize(50,100);
        m_metersIn[i]->setLevel(0.0f, 0.0f);
        levelLayout->addWidget(m_metersIn[i], 1, (i + 1), 1, 1, Qt::AlignCenter);
    }
    QString inString("In");
    QLabel* inLabel = new QLabel(inString, this);
    levelLayout->addWidget(inLabel, 0, 1, 1, m_channels, Qt::AlignCenter);

    m_metersOut = new MeterWidget*[m_channels];
    for (int i = 0; i < m_channels; i++)
    {
        m_metersOut[i] = new MeterWidget(this);
        m_metersOut[i]->setMinimumSize(50,100);
        m_metersOut[i]->setLevel(0.0f, 0.0f);
        levelLayout->addWidget(m_metersOut[i], 1, (i + 1 + m_channels), 1, 1, Qt::AlignCenter);
    }
    QString outString("Out");
    QLabel* outLabel = new QLabel(outString, this);
    levelLayout->addWidget(outLabel, 0, (1 + m_channels) , 1, m_channels, Qt::AlignCenter);

    levelBox->setLayout(levelLayout);
    clientLayout->addWidget(levelBox);

    // init layout for controls
    QWidget *controlBox = new QWidget(this);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlBox);

    // add mute/solo checkboxes
    QWidget* checksBox = new QWidget(this);
    QVBoxLayout* checksLayout = new QVBoxLayout(checksBox);
    QString muteString("Mute");
    m_muteCheckBox = new QCheckBox(muteString, this);
    m_muteCheckBox->setChecked(params.mute);
    checksLayout->addWidget(m_muteCheckBox);
    connect(m_muteCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_muteCheckBox_toggled(bool)));
    QString soloString("Solo");
    m_soloCheckBox = new QCheckBox(soloString, this);
    m_soloCheckBox->setChecked(params.solo);
    checksLayout->addWidget(m_soloCheckBox);
    connect(m_soloCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_soloCheckBox_toggled(bool)));
    controlLayout->addWidget(checksBox);

    // add delay spinbox
    QString delayString("Delay (ms)");
    QLabel* delayLabel = new QLabel(delayString, this);
    m_delaySpinBox = new QDoubleSpinBox(this);
    m_delaySpinBox->setRange(0.0, maxDelayMillis);
    m_delaySpinBox->setValue(params.delayMillis);
    controlLayout->addWidget(delayLabel);
    controlLayout->addWidget(m_delaySpinBox);
    connect(m_delaySpinBox, SIGNAL(valueChanged(double)), this, SLOT(on_delaySpinBox_valueChanged(double)));
    clientLayout->addWidget(controlBox);
    
    // init layout for type
    QWidget *typeBox = new QWidget(this);
    QHBoxLayout* typeLayout = new QHBoxLayout(typeBox);
    typeLayout->setContentsMargins(5, 0, 5, 0);
            
    // add type combo box
    m_typeComboBox = new QComboBox(this);
    const QList<RenderingType>& types = m_sam->getRenderingTypes();
    for (int t = 0; t < types.size(); t++)
    {
        m_typeComboBox->addItem(types[t].name, types[t].id);
    }
    
    QString typeString("Type:");
    QLabel* typeLabel = new QLabel(typeString, this);
    typeLabel->setAlignment(Qt::AlignRight);
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(m_typeComboBox);
    clientLayout->addWidget(typeBox);
    
    // init layout for preset
    QWidget *presetBox = new QWidget(this);
    QHBoxLayout* presetLayout = new QHBoxLayout(presetBox);
    presetLayout->setContentsMargins(5, 0, 5, 0);

    // add preset combo box
    m_presetComboBox = new QComboBox(this);
    QString presetString("Preset:");
    QLabel* presetLabel = new QLabel(presetString, this);
    presetLabel->setAlignment(Qt::AlignRight);
    presetLayout->addWidget(presetLabel);
    presetLayout->addWidget(m_presetComboBox);
    clientLayout->addWidget(presetBox);
    
    clientBox->setLayout(clientLayout);
    
    setType(params.type, params.preset); // this will init preset combo box with appropriate presets for this type
    connect(m_typeComboBox, SIGNAL(activated(int)), this, SLOT(on_typeComboBox_activated(int)));
    connect(m_presetComboBox, SIGNAL(activated(int)), this, SLOT(on_presetComboBox_activated(int)));
    
    setMinimumSize(100 + 110 * m_channels, 300);
}

ClientWidget::~ClientWidget()
{
    if (m_metersIn) // individual meter widgets will be deleted by their parent
    {
        delete[] m_metersIn;
        m_metersIn = NULL;
    }
    if (m_metersOut) // individual meter widgets will be deleted by their parent
    {
        delete[] m_metersOut;
        m_metersOut = NULL;
    }
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
    m_delaySpinBox->setValue(delay);
}

void ClientWidget::setPosition(int x, int y, int w, int h, int d)
{

}

void ClientWidget::setType(int type, int preset)
{
    //qWarning("ClientWidget::setType type = %d, preset = %d", type, preset);
    const RenderingType* typeStruct = m_sam->getType(type);
    for (int t = 0; t < m_typeComboBox->count(); t++)
    {
        QVariant data = m_typeComboBox->itemData(t);
        if (data.toInt() == type)
        {
            //qWarning("ClientWidget::setType found type %d in combo box at index %d", type, t);
            m_typeComboBox->setCurrentIndex(t);
        }
    }
    
    // update presets 
    m_presetComboBox->clear();
    for (int p = 0; p < typeStruct->presets.size(); p++)
    {
        m_presetComboBox->addItem(typeStruct->presets[p].name, typeStruct->presets[p].id);
        if (typeStruct->presets[p].id == preset)
        {
            //qWarning("ClientWidget::setType found preset %d in combo box at index %d", preset, p);
            m_presetComboBox->setCurrentIndex(m_presetComboBox->count() - 1);
        }
    }
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

void ClientWidget::on_delaySpinBox_valueChanged(double val)
{
    //qWarning("ClientWidget::on_delaySpinBox_valueChanged val = %f", val);
    emit delayChanged(m_id, (float)val);
}

void ClientWidget::addType(int id)
{
    const RenderingType* type = m_sam->getType(id);
    m_typeComboBox->addItem(type->name, type->id);
}

void ClientWidget::removeType(int id)
{
    int index = m_typeComboBox->findData(id);
    m_typeComboBox->removeItem(index);
    
    // TODO: is it possible for the currently-selected type to be removed?
}

void ClientWidget::on_typeComboBox_activated(int index)
{
    //qWarning("ClientWidget::on_typeComboBox_activated");
    int type = m_typeComboBox->itemData(index).toInt();
    //int preset = m_presetComboBox->itemData(m_presetComboBox->currentIndex()).toInt();
    emit typeChanged(m_id, type, 0);
}

void ClientWidget::on_presetComboBox_activated(int index)
{
    //qWarning("ClientWidget::on_presetComboBox_activated");
    int type = m_typeComboBox->itemData(m_typeComboBox->currentIndex()).toInt();
    int preset = m_presetComboBox->itemData(index).toInt();
    emit typeChanged(m_id, type, preset);
}

} // end of namespace SAM
