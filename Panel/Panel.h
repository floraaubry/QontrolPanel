#ifndef PANEL_H
#define PANEL_H

#include "AudioManager.h"
#include <QVBoxLayout>
#include <QMainWindow>

namespace Ui {
class Panel;
}

class Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Panel(QWidget *parent = nullptr);
    ~Panel() override;
    void animateIn(QRect trayIconGeometry);
    void animateOut();
    bool mergeApps;
    void populateApplications();
    bool isAnimating;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::Panel *ui;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;

    void populateComboBoxes();
    void setSliders();
    void setButtons();
    void setFrames();
    void updateUi();
    void addApplicationControls(QVBoxLayout *vBoxLayout, const QList<Application> &apps, bool isGroup);
    QColor borderColor;

private slots:
    void onOutputComboBoxIndexChanged(int index);
    void onInputComboBoxIndexChanged(int index);
    void onOutputValueChanged();
    void onInputValueChanged();
    void onOutputMuteButtonPressed();
    void onInputMuteButtonPressed();
    void outputAudioMeter();
    void inputAudioMeter();

signals:
    void lostFocus();
    void panelClosed();
};

#endif // PANEL_H
