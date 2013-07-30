/**
 * @file masterwidget.h
 * Widget for master controls in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#ifndef MASTERWIDGET_H
#define MASTERWIDGET_H

#include <QtGui>

class MasterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MasterWidget(float volume, bool mute, float delay, QWidget *parent = 0);

signals:

    // for notifying SAM of changes
    void volumeChanged(float volume);
    void muteChanged(bool mute);
    void delayChanged(float delay);

public slots:
    void setVolume(float);
    void setMute(bool);
    //void setDelay(float delay);

    void on_volumeSlider_valueChanged(int);
    void on_muteCheckBox_toggled(bool);

protected:
    // widgets
    QSlider* m_volumeSlider;
    QCheckBox* m_muteCheckBox;
};

#endif // MASTERWIDGET_H
