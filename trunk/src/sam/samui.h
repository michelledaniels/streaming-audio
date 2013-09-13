/**
 * @file samui.h
 * Interface for SAM GUI
 * @author Michelle Daniels
 * @copyright UCSD 2012
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
