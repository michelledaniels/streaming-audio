/**
 * @file clientwidget.h
 * Widgets for SAM clients in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#ifndef CLIENTWIDGET_H
#define CLIENTWIDGET_H

#include <QtGui>

#include "sam.h"

class MeterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MeterWidget(QWidget *parent = 0);

    void setLevel(float rms, float peak) { m_rms = rms; m_peak = peak; }

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
    explicit ClientWidget(int id, const char* name, ClientParams& params, QWidget *parent = 0);

signals:

    // for notifying SAM of changes
    void volumeChanged(float volume);
    void muteChanged(bool mute);
    void soloChanged(bool solo);
    void delayChanged(float delay);
    void positionChanged(int x, int y, int w, int h, int d);

public slots:
    /*void setVolume(float volume);
    void setMute(bool mute);
    void setSolo(bool solo);
    void setDelay(float delay);
    void setPosition(int x, int y, int w, int h, int d);
    void setMeter(const float* rms, const float* peak);
*/
    void on_volumeSlider_valueChanged(int);
    void on_muteCheckBox_toggled(bool);
    void on_soloCheckBox_toggled(bool);

protected:
    QString m_name;
    int m_channels;
    int m_id;

    // widgets
    QSlider* m_volumeSlider;
    QCheckBox* m_muteCheckBox;
    QCheckBox* m_soloCheckBox;
};

#endif // CLIENTWIDGET_H
