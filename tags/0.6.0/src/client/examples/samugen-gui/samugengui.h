/**
 * @file samugen-gui/samugengui.h
 * Interface for a simple unit-generator client with primitive GUI
 * @author Michelle Daniels
 * @date September 2013
 * @copyright UCSD 2013
 * @license New BSD License: http://opensource.org/licenses/BSD-3-Clause
 */

#ifndef SAMUGENGUI_H
#define SAMUGENGUI_H

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>

#include "sam_client.h"

class SamUgenGui : public QMainWindow
{
    Q_OBJECT
public:
    explicit SamUgenGui(sam::SacParams& defaultParams, QWidget *parent = 0);

    ~SamUgenGui();

    bool onSacAudio(unsigned int numChannels, unsigned int nframes, float** out);
    void onSacMute(bool mute);
    void onSacSolo(bool solo);
    void onSacDisconnect();

signals:

public slots:
    void onClientButtonClicked();
    void onClientDestroyed();
    void on_muteCheckBox_toggled(bool);
    void on_soloCheckBox_toggled(bool);
    void on_portSpinBox_valueChanged(int);
private:

    void start_client();
    void stop_client();

    void set_widgets_enabled(bool enabled);

    static bool sac_audio_callback(unsigned int numChannels, unsigned int nframes, float** out, void* ugen);
    static void sac_mute_callback(bool mute, void* ugen);
    static void sac_solo_callback(bool solo, void* ugen);
    static void sac_disconnect_callback(void* ugen);

    sam::StreamingAudioClient* m_client;
    QPushButton* m_clientButton;
    QCheckBox* m_muteCheckBox;
    QCheckBox* m_soloCheckBox;
    QSpinBox* m_portSpinBox;
    QLabel* m_portLabel;
    QLineEdit* m_ipLineEdit;
    QLabel* m_ipLabel;

    sam::SacParams m_defaultParams;
};

#endif // SAMUGENGUI_H
