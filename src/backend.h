#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>

#include "soundengine.h"

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList channels READ channels NOTIFY channelsChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY channelsChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(double masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool waveModulation READ waveModulation WRITE setWaveModulation NOTIFY waveModulationChanged)
    Q_PROPERTY(QString selectedCategory READ selectedCategory WRITE setSelectedCategory NOTIFY selectedCategoryChanged)
    Q_PROPERTY(int timerRemainingSeconds READ timerRemainingSeconds NOTIFY timerChanged)
    Q_PROPERTY(int timerTotalSeconds READ timerTotalSeconds NOTIFY timerChanged)
    Q_PROPERTY(double fadeMultiplier READ fadeMultiplier NOTIFY timerChanged)
    Q_PROPERTY(QString timerLabel READ timerLabel NOTIFY timerChanged)
    Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(int activePreset READ activePreset NOTIFY presetsChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString audioError READ audioError CONSTANT)

public:
    struct ChannelDef {
        QString id;
        QString name;
        QString subtitle;
        QString category;
        QString icon;
        double volume = 0.0;
        double lastNonZeroVolume = 0.55;
    };

    struct PresetDef {
        int slot = 1;
        QString name;
        QVector<double> volumes;
        bool isCustom = false;
    };

    explicit Backend(const QString &stateFilePath = {}, bool enableAudioOutput = true, QObject *parent = nullptr);

    QVariantList channels() const;
    int activeCount() const;

    bool isPlaying() const { return m_playing; }
    void setPlaying(bool playing);

    double masterVolume() const { return m_masterVolume; }
    void setMasterVolume(double volume);

    bool isMuted() const { return m_muted; }
    void setMuted(bool muted);

    bool waveModulation() const { return m_waveModulation; }
    void setWaveModulation(bool enabled);

    QString selectedCategory() const { return m_selectedCategory; }
    void setSelectedCategory(const QString &category);

    int timerRemainingSeconds() const { return m_timerRemainingSeconds; }
    int timerTotalSeconds() const { return m_timerTotalSeconds; }
    double fadeMultiplier() const { return m_fadeMultiplier; }
    QString timerLabel() const;

    QVariantList presets() const;
    int activePreset() const { return m_activePreset; }

    QString statusMessage() const { return m_statusMessage; }
    QString audioError() const { return m_engine.audioError(); }

    SoundEngine *engine() { return &m_engine; }

    Q_INVOKABLE void primaryAction();
    Q_INVOKABLE void toggleChannel(int index);
    Q_INVOKABLE void setChannelVolume(int index, double volume);
    Q_INVOKABLE double channelVolume(int index) const;
    Q_INVOKABLE void adjustMasterVolume(double delta);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void toggleWaveModulation();
    Q_INVOKABLE void resetMix();
    Q_INVOKABLE void randomizeMix();

    Q_INVOKABLE void cycleSleepTimer();
    Q_INVOKABLE void setSleepTimerMinutes(int minutes);
    Q_INVOKABLE void setSleepTimerSeconds(int seconds, int fadeWindowSeconds = 60);
    Q_INVOKABLE void tickTimer(int elapsedSeconds = 1);

    Q_INVOKABLE void loadPreset(int slot);
    Q_INVOKABLE void savePreset(int slot, const QString &customName = {});
    Q_INVOKABLE void saveCurrentToActiveOrNextPreset();

signals:
    void channelsChanged();
    void playingChanged();
    void masterVolumeChanged();
    void mutedChanged();
    void waveModulationChanged();
    void selectedCategoryChanged();
    void timerChanged();
    void presetsChanged();
    void statusMessageChanged();

private:
    void initChannels();
    void initDefaultPresets();
    void loadState();
    void saveState() const;
    void showStatus(const QString &message);

    QString m_stateFilePath;
    SoundEngine m_engine;
    QVector<ChannelDef> m_channels;
    QVector<PresetDef> m_presets;

    bool m_playing = false;
    double m_masterVolume = 0.80;
    bool m_muted = false;
    bool m_waveModulation = true;
    QString m_selectedCategory = "all";

    int m_timerRemainingSeconds = 0;
    int m_timerTotalSeconds = 0;
    int m_timerFadeWindowSeconds = 60;
    double m_fadeMultiplier = 1.0;
    QTimer m_secondTimer;

    int m_activePreset = 0;
    QString m_statusMessage;
    QTimer m_statusTimer;
};
