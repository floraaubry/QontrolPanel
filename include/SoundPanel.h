#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include "audiomanager.h"
#include <QObject>
#include <QQmlApplicationEngine>
#include <QWindow>
#include <QIcon>
#include <QComboBox>
#include <QSettings>

class SoundPanel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int playbackVolume READ playbackVolume WRITE setPlaybackVolume NOTIFY playbackVolumeChanged)
    Q_PROPERTY(int recordingVolume READ recordingVolume WRITE setRecordingVolume NOTIFY recordingVolumeChanged)
    Q_PROPERTY(bool playbackMuted READ playbackMuted WRITE setPlaybackMuted NOTIFY playbackMutedChanged)
    Q_PROPERTY(bool recordingMuted READ recordingMuted WRITE setRecordingMuted NOTIFY recordingMutedChanged)

public:
    explicit SoundPanel(QObject* parent = nullptr);
    ~SoundPanel() override;

    QWindow* soundPanelWindow;

    int playbackVolume() const;
    void setPlaybackVolume(int volume);

    int recordingVolume() const;
    void setRecordingVolume(int volume);

    bool playbackMuted() const;
    void setPlaybackMuted(bool muted);

    bool recordingMuted() const;
    void setRecordingMuted(bool muted);

    void animateOut();

public slots:
    void onPlaybackVolumeChanged(int volume);
    void onRecordingVolumeChanged(int volume);

    void onPlaybackDeviceChanged(const QString &deviceName);
    void onRecordingDeviceChanged(const QString &deviceName);

    void onOutputMuteButtonClicked();
    void onInputMuteButtonClicked();

    void onOutputSliderReleased();

    void onApplicationVolumeSliderValueChanged(QString appID, int volume);
    void onApplicationMuteButtonClicked(QString appID, bool state);

private slots:
    void onVolumeChangedWithTray(int volume);
    void onOutputMuteStateChanged(bool mutedState);

signals:
    void playbackVolumeChanged();
    void recordingVolumeChanged();
    void playbackMutedChanged();
    void recordingMutedChanged();

    void shouldUpdateTray();
    void panelClosed();

private:
    QQmlApplicationEngine* engine;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;
    QList<Application> applications;
    int m_playbackVolume = 0;
    int m_recordingVolume = 0;
    bool m_playbackMuted = false;
    bool m_recordingMuted = false;

    void animateIn();
    void configureQML();
    void setupUI();
    void populateComboBoxes();
    void populateApplicationModel();
    bool isWindows10;
    bool isAnimating;
    HWND hWnd;
    QSettings settings;
    bool mixerOnly;
    bool systemSoundsMuted;
};

#endif // MEDIASFLYOUT_H
