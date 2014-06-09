/**
 * @file samui.h
 * Interface for SAM GUI
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

#ifndef SAMUI_H
#define SAMUI_H

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>

#include "sam.h"
#include "samparams.h"

namespace Ui {
class SamUI;
}

namespace sam
{

class MasterWidget;
class ClientWidget;
class SamParams;

/**
 * @class SamUI
 * SamUI is the main window of a GUI for SAM
 */
class SamUI : public QMainWindow
{
    Q_OBJECT
    
public:
    /**
     * SamUI constructor.
     * @param params SAM parameters
     * @param parent parent Qt widget
     */
    explicit SamUI(const SamParams& params, QWidget *parent = 0);

    /**
     * SamUI destructor.
     */
    ~SamUI();

signals:
    void startSam();
    
public slots:
    /**
     * Perform necessary cleanup before app closes.
     */
    void doBeforeQuit();

    /**
     * Add client.
     */
    void addClient(int);

    /**
     * Remove client.
     */
    void removeClient(int);
    
    void onSamStartupError();
    void onSamStarted();
    void onSamStopped();
    void onSetAppTypeFailed(int, int, int, int, int, int);

    void setAppVolume(int, float);
    void setAppMute(int, bool);
    void setAppSolo(int, bool);
    void setAppDelay(int, float);
    void setAppPosition(int, int, int, int, int, int);
    void setAppType(int, int, int);
    void setAppMeter(int, int, float, float, float, float);
    
private slots:
    /**
     * Respond to start button click.
     */
    void onSamButtonClicked();

    /**
     * Respond to about action triggered.
     */
    void on_actionAbout_triggered();

private:

    void connect_client(int id);
    void disconnect_client(int id);
    
    void start_sam();
    void stop_sam();

    Ui::SamUI *ui;                  ///< UI
    StreamingAudioManager* m_sam;   ///< SAM instance
    QThread* m_samThread;

    MasterWidget* m_master;
    QPushButton* m_samButton;
    QGroupBox* m_clientGroup;
    QHBoxLayout* m_clientLayout;
    QLabel* m_oscDirections;

    ClientWidget** m_clients;
    SamParams m_samParams;
};

} // end of namespace SAM
#endif // SAMUI_H
