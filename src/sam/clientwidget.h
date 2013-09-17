/**
 * @file clientwidget.h
 * Widgets for SAM clients in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef CLIENTWIDGET_H
#define CLIENTWIDGET_H

#include <QtGui>

#include "sam.h"

namespace sam
{

class MeterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MeterWidget(QWidget *parent = 0);
    void setLevel(float rms, float peak) { m_rms = rms; m_peak = peak; update(); }

signals:

public slots:

protected:
    virtual void paintEvent(QPaintEvent* event);

    float m_rms;
    float m_peak;
};

class ClientWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClientWidget(int id, const char* name, ClientParams& params, double maxDelayMillis, QWidget *parent = 0);

    void setName(const char* name);
    void setVolume(float volume);
    void setMute(bool mute);
    void setSolo(bool solo);
    void setDelay(float delay);
    void setPosition(int x, int y, int w, int h, int d);
    void setType(int type, int preset);
    void setMeter(int ch, float rmsIn, float peakIn, float rmsOut, float peakOut);

signals:

    // for notifying SAM of changes
    void volumeChanged(int id, float volume);
    void muteChanged(int id, bool mute);
    void soloChanged(int id, bool solo);
    void delayChanged(int id, float delay);
    void positionChanged(int id, int x, int y, int w, int h, int d);

public slots:
    void on_volumeSlider_valueChanged(int);
    void on_muteCheckBox_toggled(bool);
    void on_soloCheckBox_toggled(bool);
    void on_delaySpinBox_valueChanged(double);

protected:
    QString m_name;
    int m_channels;
    int m_id;

    // widgets
    QLabel* m_nameLabel;
    QSlider* m_volumeSlider;
    QCheckBox* m_muteCheckBox;
    QCheckBox* m_soloCheckBox;
    QDoubleSpinBox* m_delaySpinBox;

    MeterWidget** m_metersIn;
    MeterWidget** m_metersOut;
};

} // end of namespace SAM

#endif // CLIENTWIDGET_H
