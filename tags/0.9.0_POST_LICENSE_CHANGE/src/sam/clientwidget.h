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
    explicit ClientWidget(int id, const char* name, ClientParams& params, double maxDelayMillis, StreamingAudioManager* sam, QWidget *parent = 0);
    ~ClientWidget();

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
    void typeChanged(int, int, int);
    
public slots:
    void on_volumeSlider_valueChanged(int);
    void on_muteCheckBox_toggled(bool);
    void on_soloCheckBox_toggled(bool);
    void on_delaySpinBox_valueChanged(double);
    void on_typeComboBox_activated(int);
    void on_presetComboBox_activated(int);
    
    void addType(int id);
    void removeType(int id);

protected:
    QString m_name;
    int m_channels;
    int m_id;
    StreamingAudioManager* m_sam;

    // widgets
    QLabel* m_nameLabel;
    QSlider* m_volumeSlider;
    QCheckBox* m_muteCheckBox;
    QCheckBox* m_soloCheckBox;
    QDoubleSpinBox* m_delaySpinBox;
    QComboBox* m_typeComboBox;
    QComboBox* m_presetComboBox;

    MeterWidget** m_metersIn;
    MeterWidget** m_metersOut;
};

} // end of namespace SAM

#endif // CLIENTWIDGET_H
