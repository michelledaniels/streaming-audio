#ifndef SAMUI_H
#define SAMUI_H

#include <QMainWindow>

#include "sam.h"

namespace Ui {
class SamUI;
}

class SamUI : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit SamUI(const SamParams& params, QWidget *parent = 0);
    ~SamUI();

public slots:
    void doBeforeQuit();
    
private slots:
    void on_startSamButton_clicked();
    void on_actionAbout_triggered();

    void on_volumeSlider_valueChanged(int value);

    void on_muteCheckBox_stateChanged(int arg1);

private:
    Ui::SamUI *ui;
    StreamingAudioManager* m_sam;
};

#endif // SAMUI_H
