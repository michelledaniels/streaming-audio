/**
 * @file masterwidget.h
 * Widget for master controls in SAM GUI
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
    explicit MasterWidget(float volume, bool mute, float delay, float maxDelayMillis, QWidget *parent = 0);

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
