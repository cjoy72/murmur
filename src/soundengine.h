#pragma once

#include <QAudioSink>
#include <QIODevice>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>
#include <atomic>
#include <cstdint>

class SoundEngine;

class SoundGeneratorDevice : public QIODevice {
    Q_OBJECT
public:
    explicit SoundGeneratorDevice(SoundEngine *engine, QObject *parent = nullptr);
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override { return true; }

private:
    SoundEngine *m_engine;
};

class SoundEngine : public QObject {
    Q_OBJECT
public:
    static constexpr int SampleRate = 48000;
    static constexpr int ChannelCount = 17;

    enum SoundId {
        WhiteNoise = 0,
        PinkNoise = 1,
        BrownNoise = 2,
        GentleRain = 3,
        HeavyRain = 4,
        DistantThunder = 5,
        OceanWaves = 6,
        FlowingStream = 7,
        Wind = 8,
        Campfire = 9,
        ForestNight = 10,
        BoxFan = 11,
        TrainTracks = 12,
        CafeHum = 13,
        BinauralAlpha = 14,
        BinauralTheta = 15,
        BinauralDelta = 16
    };

    explicit SoundEngine(bool enableAudioOutput = true, QObject *parent = nullptr);
    ~SoundEngine() override;

    void setChannelVolume(int index, float volume);
    float channelVolume(int index) const;

    void setMasterVolume(float volume);
    float masterVolume() const;

    void setFadeMultiplier(float multiplier);
    float fadeMultiplier() const;

    void setMuted(bool muted);
    bool isMuted() const;

    void setPlaying(bool playing);
    bool isPlaying() const;

    void setWaveModulation(bool enabled);
    bool waveModulation() const;

    // Effective instant modulation factor [0.0 .. 1.0] for UI breathing indicator
    float channelModulationLevel(int index) const;

    QString audioError() const { return m_audioError; }

    // Renders interleaved stereo float samples [-1.0, 1.0] for audio output or unit tests.
    void renderFrames(float *stereoOut, int frameCount);

private:
    void updateAudioSinkState();
    float nextWhite();

    struct PinkState {
        float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        float step(float white);
    };

    struct Biquad {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void setLowPass(float cutoffHz, float q);
        void setBandPass(float centerHz, float q);
        float process(float in);
    };

    bool m_enableAudioOutput = true;
    QAudioSink *m_audioSink = nullptr;
    SoundGeneratorDevice *m_device = nullptr;
    QString m_audioError;

    std::atomic<float> m_targetVolumes[ChannelCount];
    float m_currentVolumes[ChannelCount]{};
    std::atomic<float> m_modLevels[ChannelCount];

    std::atomic<float> m_masterVolume{0.80f};
    std::atomic<float> m_fadeMultiplier{1.0f};
    std::atomic<bool> m_muted{false};
    std::atomic<bool> m_playing{false};
    std::atomic<bool> m_waveModulation{true};
    float m_currentMasterGain = 0.0f;

    // PRNG and synthesis state accessed on the audio render thread
    QMutex m_renderMutex;
    uint64_t m_rngState = 0x853c49e6748fea9bULL;
    uint64_t m_sampleCounter = 0;

    PinkState m_pinkL, m_pinkR;
    PinkState m_rainPinkL, m_rainPinkR;
    PinkState m_heavyRainPinkL, m_heavyRainPinkR;
    PinkState m_oceanPinkL, m_oceanPinkR;
    PinkState m_streamPinkL, m_streamPinkR;
    PinkState m_windPinkL, m_windPinkR;
    PinkState m_cafePinkL, m_cafePinkR;

    float m_brownL = 0.0f, m_brownR = 0.0f;
    float m_thunderBrownL = 0.0f, m_thunderBrownR = 0.0f;
    float m_campfireBrownL = 0.0f, m_campfireBrownR = 0.0f;
    float m_trainBrownL = 0.0f, m_trainBrownR = 0.0f;

    // Droplet / crackle envelopes
    float m_rainDropL = 0.0f, m_rainDropR = 0.0f;
    float m_heavyDropL = 0.0f, m_heavyDropR = 0.0f;
    float m_campfireCrackleL = 0.0f, m_campfireCrackleR = 0.0f;
    float m_campfirePopPhase = 0.0f;

    // Thunder state
    float m_thunderEnvL = 0.0f, m_thunderEnvR = 0.0f;
    float m_thunderTarget = 0.0f;

    // Filters
    Biquad m_rainFilterL, m_rainFilterR;
    Biquad m_rainDropFilterL, m_rainDropFilterR;
    Biquad m_heavyRainFilterL, m_heavyRainFilterR;
    Biquad m_thunderFilterL, m_thunderFilterR;
    Biquad m_oceanFilterL, m_oceanFilterR;
    Biquad m_streamFilter1L, m_streamFilter1R;
    Biquad m_streamFilter2L, m_streamFilter2R;
    Biquad m_windFilterL, m_windFilterR;
    Biquad m_campfireCrackFilterL, m_campfireCrackFilterR;
    Biquad m_fanFilterL, m_fanFilterR;
    Biquad m_cafeFilter1L, m_cafeFilter1R;
    Biquad m_cafeFilter2L, m_cafeFilter2R;

    // Oscillator phases
    double m_fanPhase = 0.0;
    double m_forestPhase1 = 0.0, m_forestPhase2 = 0.0;
    double m_alphaPhaseL = 0.0, m_alphaPhaseR = 0.0;
    double m_thetaPhaseL = 0.0, m_thetaPhaseR = 0.0;
    double m_deltaPhaseL = 0.0, m_deltaPhaseR = 0.0;
};
