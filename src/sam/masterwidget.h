/**
 * @file masterwidget.h
 * Widget for master controls in SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2013
 */

#ifndef MASTERWIDGET_H
#define MASTERWIDGET_H

#include <QtGui>

namespace sam
{
static const int VOLUME_SLIDER_SCALE = 100;

class MasterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MasterWidget(float volume, bool mute, float delay, QWidget *parent = 0);

    float getVolume() { return m_volumeSlider->value() / (float)VOLUME_SLIDER_SCALE; }
    bool getMute() { return m_muteCheckBox->isChecked(); }
    float getDelay() { return (float)m_delaySpinBox->value(); }

signals:

    // for notifying SAM of changes
    void volumeChanged(float volume);
    void muteChanged(bool mute);
    void delayChanged(float delay);

public slots:
    void setVolume(float);
    void setMute(bool);
    void setDelay(float delay);

    void on_volumeSlider_valueChanged(int);
    void on_muteCheckBox_toggled(bool);
    void on_delaySpinBox_valueChanged(double);

protected:
    // widgets
    QSlider* m_volumeSlider;
    QLabel* m_volumeLabel;
    QCheckBox* m_muteCheckBox;
    QDoubleSpinBox* m_delaySpinBox;
};

} // end of namespace SAM

#endif // MASTERWIDGET_H
