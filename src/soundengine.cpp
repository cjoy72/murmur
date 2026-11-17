#include "soundengine.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr float TwoPi = 6.28318530717958647692f;
}

SoundGeneratorDevice::SoundGeneratorDevice(SoundEngine *engine, QObject *parent)
    : QIODevice(parent), m_engine(engine) {
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

qint64 SoundGeneratorDevice::readData(char *data, qint64 maxlen) {
    const int bytesPerFrame = sizeof(int16_t) * 2;
    const int frames = static_cast<int>(maxlen / bytesPerFrame);
    if (frames <= 0)
        return 0;

    // Process in chunks of 512 frames to avoid large stack allocations
    int16_t *out = reinterpret_cast<int16_t *>(data);
    float temp[512 * 2];
    int remaining = frames;
    while (remaining > 0) {
        const int chunk = std::min(remaining, 512);
        m_engine->renderFrames(temp, chunk);
        for (int i = 0; i < chunk * 2; ++i) {
            const float clamped = std::clamp(temp[i], -1.0f, 1.0f);
            out[i] = static_cast<int16_t>(clamped * 32767.0f);
        }
        out += chunk * 2;
        remaining -= chunk;
    }
    return static_cast<qint64>(frames) * bytesPerFrame;
}

qint64 SoundGeneratorDevice::writeData(const char *, qint64) {
    return 0;
}

qint64 SoundGeneratorDevice::bytesAvailable() const {
    return 16384 + QIODevice::bytesAvailable();
}

float SoundEngine::PinkState::step(float white) {
    b0 = 0.99886f * b0 + white * 0.0555179f;
    b1 = 0.99332f * b1 + white * 0.0750759f;
    b2 = 0.96900f * b2 + white * 0.1538520f;
    b3 = 0.86650f * b3 + white * 0.3104856f;
    b4 = 0.55000f * b4 + white * 0.5329522f;
    b5 = -0.7616f * b5 - white * 0.0168980f;
    const float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
    b6 = white * 0.115926f;
    return pink * 0.18f;
}

void SoundEngine::Biquad::setLowPass(float cutoffHz, float q) {
    const float w0 = TwoPi * std::clamp(cutoffHz, 20.0f, 20000.0f) / float(SampleRate);
    const float cosw0 = std::cos(w0);
    const float sinw0 = std::sin(w0);
    const float alpha = sinw0 / (2.0f * std::max(0.1f, q));
    const float a0 = 1.0f + alpha;
    b0 = ((1.0f - cosw0) * 0.5f) / a0;
    b1 = (1.0f - cosw0) / a0;
    b2 = ((1.0f - cosw0) * 0.5f) / a0;
    a1 = (-2.0f * cosw0) / a0;
    a2 = (1.0f - alpha) / a0;
}

void SoundEngine::Biquad::setBandPass(float centerHz, float q) {
    const float w0 = TwoPi * std::clamp(centerHz, 25.0f, 18000.0f) / float(SampleRate);
    const float cosw0 = std::cos(w0);
    const float sinw0 = std::sin(w0);
    const float alpha = sinw0 / (2.0f * std::max(0.1f, q));
    const float a0 = 1.0f + alpha;
    b0 = alpha / a0;
    b1 = 0.0f;
    b2 = -alpha / a0;
    a1 = (-2.0f * cosw0) / a0;
    a2 = (1.0f - alpha) / a0;
}

float SoundEngine::Biquad::process(float in) {
    const float out = b0 * in + z1;
    z1 = b1 * in - a1 * out + z2;
    z2 = b2 * in - a2 * out;
    return out;
}

SoundEngine::SoundEngine(bool enableAudioOutput, QObject *parent)
    : QObject(parent), m_enableAudioOutput(enableAudioOutput) {
    for (int i = 0; i < ChannelCount; ++i) {
        m_targetVolumes[i].store(0.0f, std::memory_order_relaxed);
        m_currentVolumes[i] = 0.0f;
        m_modLevels[i].store(1.0f, std::memory_order_relaxed);
    }

    // Initialize filter coefficients
    m_rainFilterL.setBandPass(1100.0f, 0.7f);
    m_rainFilterR.setBandPass(1150.0f, 0.7f);
    m_rainDropFilterL.setBandPass(2800.0f, 1.8f);
    m_rainDropFilterR.setBandPass(3100.0f, 1.8f);

    m_heavyRainFilterL.setLowPass(2400.0f, 0.75f);
    m_heavyRainFilterR.setLowPass(2450.0f, 0.75f);

    m_thunderFilterL.setLowPass(85.0f, 1.1f);
    m_thunderFilterR.setLowPass(80.0f, 1.1f);

    m_oceanFilterL.setLowPass(420.0f, 0.9f);
    m_oceanFilterR.setLowPass(420.0f, 0.9f);

    m_streamFilter1L.setBandPass(650.0f, 2.2f);
    m_streamFilter1R.setBandPass(710.0f, 2.2f);
    m_streamFilter2L.setBandPass(1450.0f, 2.5f);
    m_streamFilter2R.setBandPass(1580.0f, 2.5f);

    m_windFilterL.setBandPass(320.0f, 3.2f);
    m_windFilterR.setBandPass(350.0f, 3.2f);

    m_campfireCrackFilterL.setBandPass(3200.0f, 1.6f);
    m_campfireCrackFilterR.setBandPass(3500.0f, 1.6f);

    m_fanFilterL.setLowPass(320.0f, 0.8f);
    m_fanFilterR.setLowPass(330.0f, 0.8f);

    m_cafeFilter1L.setBandPass(480.0f, 1.8f);
    m_cafeFilter1R.setBandPass(520.0f, 1.8f);
    m_cafeFilter2L.setBandPass(1150.0f, 2.0f);
    m_cafeFilter2R.setBandPass(1220.0f, 2.0f);

    if (m_enableAudioOutput && qEnvironmentVariable("QT_QPA_PLATFORM") != QLatin1String("offscreen")) {
        const QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
        if (defaultDevice.isNull()) {
            m_audioError = tr("No audio output device found.");
        } else {
            QAudioFormat format;
            format.setSampleRate(SampleRate);
            format.setChannelCount(2);
            format.setSampleFormat(QAudioFormat::Int16);
            if (!defaultDevice.isFormatSupported(format)) {
                m_audioError = tr("Default audio device does not support 48 kHz stereo PCM.");
            } else {
                m_device = new SoundGeneratorDevice(this, this);
                m_audioSink = new QAudioSink(defaultDevice, format, this);
                m_audioSink->setBufferSize(4096 * sizeof(int16_t) * 2);
            }
        }
    }
}

SoundEngine::~SoundEngine() {
    if (m_audioSink) {
        m_audioSink->stop();
    }
}

void SoundEngine::setChannelVolume(int index, float volume) {
    if (index < 0 || index >= ChannelCount)
        return;
    m_targetVolumes[index].store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_relaxed);
    updateAudioSinkState();
}

float SoundEngine::channelVolume(int index) const {
    if (index < 0 || index >= ChannelCount)
        return 0.0f;
    return m_targetVolumes[index].load(std::memory_order_relaxed);
}

void SoundEngine::setMasterVolume(float volume) {
    m_masterVolume.store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_relaxed);
}

float SoundEngine::masterVolume() const {
    return m_masterVolume.load(std::memory_order_relaxed);
}

void SoundEngine::setFadeMultiplier(float multiplier) {
    m_fadeMultiplier.store(std::clamp(multiplier, 0.0f, 1.0f), std::memory_order_relaxed);
}

float SoundEngine::fadeMultiplier() const {
    return m_fadeMultiplier.load(std::memory_order_relaxed);
}

void SoundEngine::setMuted(bool muted) {
    m_muted.store(muted, std::memory_order_relaxed);
}

bool SoundEngine::isMuted() const {
    return m_muted.load(std::memory_order_relaxed);
}

void SoundEngine::setPlaying(bool playing) {
    m_playing.store(playing, std::memory_order_relaxed);
    updateAudioSinkState();
}

bool SoundEngine::isPlaying() const {
    return m_playing.load(std::memory_order_relaxed);
}

void SoundEngine::setWaveModulation(bool enabled) {
    m_waveModulation.store(enabled, std::memory_order_relaxed);
}

bool SoundEngine::waveModulation() const {
    return m_waveModulation.load(std::memory_order_relaxed);
}

float SoundEngine::channelModulationLevel(int index) const {
    if (index < 0 || index >= ChannelCount)
        return 1.0f;
    return m_modLevels[index].load(std::memory_order_relaxed);
}

void SoundEngine::updateAudioSinkState() {
    if (!m_audioSink || !m_device)
        return;
    if (m_playing.load(std::memory_order_relaxed)) {
        if (m_audioSink->state() == QAudio::StoppedState) {
            m_audioSink->start(m_device);
        } else if (m_audioSink->state() == QAudio::SuspendedState) {
            m_audioSink->resume();
        }
    }
}

float SoundEngine::nextWhite() {
    // xorshift64* fast high-quality PRNG returning [-1.0, 1.0]
    uint64_t x = m_rngState;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    m_rngState = x;
    const uint32_t r = static_cast<uint32_t>((x * 0x2545F4914F6CDD1DULL) >> 32);
    return (static_cast<float>(r) / 2147483648.0f) - 1.0f;
}

void SoundEngine::renderFrames(float *stereoOut, int frameCount) {
    QMutexLocker locker(&m_renderMutex);

    const bool playing = m_playing.load(std::memory_order_relaxed);
    const bool muted = m_muted.load(std::memory_order_relaxed);
    const float masterVol = m_masterVolume.load(std::memory_order_relaxed);
    const float fadeMult = m_fadeMultiplier.load(std::memory_order_relaxed);
    const bool waveMod = m_waveModulation.load(std::memory_order_relaxed);
    const float targetMaster = (playing && !muted) ? (masterVol * fadeMult) : 0.0f;

    float targets[ChannelCount];
    for (int c = 0; c < ChannelCount; ++c) {
        targets[c] = m_targetVolumes[c].load(std::memory_order_relaxed);
    }

    // Per-channel LFO frequencies (Hz) so organic wave modulation never syncs up artificially
    static constexpr float lfoFreqs[ChannelCount] = {
        0.055f, 0.047f, 0.039f, // Noises
        0.063f, 0.051f, 0.034f, 0.078f, 0.089f, 0.044f, // Weather
        0.071f, 0.058f, 0.041f, 0.066f, 0.053f, // Cozy
        0.037f, 0.031f, 0.027f  // Binaural
    };

    const float smoothAlpha = 0.0015f;

    for (int f = 0; f < frameCount; ++f) {
        m_currentMasterGain += (targetMaster - m_currentMasterGain) * smoothAlpha;
        for (int c = 0; c < ChannelCount; ++c) {
            m_currentVolumes[c] += (targets[c] - m_currentVolumes[c]) * smoothAlpha;
        }

        const double t = static_cast<double>(m_sampleCounter) / double(SampleRate);
        ++m_sampleCounter;

        // Update dynamic filter sweeps every 64 samples to save CPU
        if ((m_sampleCounter & 63) == 0) {
            // Ocean waves: slow surf swell (0.11 Hz) with stereo phase offset
            const float swellL = 0.5f + 0.5f * std::sin(float(t) * TwoPi * 0.11f);
            const float swellR = 0.5f + 0.5f * std::sin(float(t) * TwoPi * 0.11f + 0.7f);
            m_oceanFilterL.setLowPass(140.0f + 950.0f * swellL * swellL, 1.15f);
            m_oceanFilterR.setLowPass(140.0f + 950.0f * swellR * swellR, 1.15f);

            // Wind: drifting howling formant
            const float windSweepL = 0.5f + 0.5f * std::sin(float(t) * TwoPi * 0.07f + 0.3f * std::sin(float(t) * 0.19f));
            const float windSweepR = 0.5f + 0.5f * std::sin(float(t) * TwoPi * 0.065f + 1.1f);
            m_windFilterL.setBandPass(180.0f + 420.0f * windSweepL, 3.0f);
            m_windFilterR.setBandPass(190.0f + 420.0f * windSweepR, 3.0f);

            // Stream babble modulation
            const float streamMod = 0.5f + 0.5f * std::sin(float(t) * TwoPi * 1.7f);
            m_streamFilter1L.setBandPass(520.0f + 260.0f * streamMod, 2.0f);
            m_streamFilter1R.setBandPass(560.0f + 240.0f * (1.0f - streamMod), 2.0f);

            // Update modulation level indicators for UI
            for (int c = 0; c < ChannelCount; ++c) {
                if (waveMod) {
                    const float lfo = 0.76f + 0.24f * std::sin(float(t) * TwoPi * lfoFreqs[c] + float(c) * 1.37f);
                    m_modLevels[c].store(lfo, std::memory_order_relaxed);
                } else {
                    m_modLevels[c].store(1.0f, std::memory_order_relaxed);
                }
            }
        }

        if (m_currentMasterGain < 1e-5f) {
            stereoOut[f * 2] = 0.0f;
            stereoOut[f * 2 + 1] = 0.0f;
            continue;
        }

        float mixL = 0.0f;
        float mixR = 0.0f;

        auto chanGain = [&](int idx) -> float {
            const float v = m_currentVolumes[idx];
            if (v < 1e-4f)
                return 0.0f;
            return v * m_modLevels[idx].load(std::memory_order_relaxed);
        };

        // 0: White Noise (softened high end for pleasant listening)
        if (const float g = chanGain(WhiteNoise); g > 0.0f) {
            mixL += nextWhite() * 0.25f * g;
            mixR += nextWhite() * 0.25f * g;
        }

        // 1: Pink Noise (-3 dB/oct)
        if (const float g = chanGain(PinkNoise); g > 0.0f) {
            mixL += m_pinkL.step(nextWhite()) * 0.45f * g;
            mixR += m_pinkR.step(nextWhite()) * 0.45f * g;
        }

        // 2: Brown Noise (-6 dB/oct leaky integrator)
        if (const float g = chanGain(BrownNoise); g > 0.0f) {
            m_brownL = std::clamp((m_brownL + 0.025f * nextWhite()) * 0.996f, -1.0f, 1.0f);
            m_brownR = std::clamp((m_brownR + 0.025f * nextWhite()) * 0.996f, -1.0f, 1.0f);
            mixL += m_brownL * 0.65f * g;
            mixR += m_brownR * 0.65f * g;
        }

        // 3: Gentle Rain
        if (const float g = chanGain(GentleRain); g > 0.0f) {
            const float wL = nextWhite();
            const float wR = nextWhite();
            const float bedL = m_rainFilterL.process(m_rainPinkL.step(wL));
            const float bedR = m_rainFilterR.process(m_rainPinkR.step(wR));
            if (std::abs(wL) > 0.9965f)
                m_rainDropL = 0.65f;
            if (std::abs(wR) > 0.9965f)
                m_rainDropR = 0.65f;
            m_rainDropL *= 0.985f;
            m_rainDropR *= 0.985f;
            const float dropL = m_rainDropFilterL.process(wL * m_rainDropL);
            const float dropR = m_rainDropFilterR.process(wR * m_rainDropR);
            mixL += (bedL * 0.45f + dropL * 0.35f) * g;
            mixR += (bedR * 0.45f + dropR * 0.35f) * g;
        }

        // 4: Heavy Rain
        if (const float g = chanGain(HeavyRain); g > 0.0f) {
            const float wL = nextWhite();
            const float wR = nextWhite();
            const float bedL = m_heavyRainFilterL.process(m_heavyRainPinkL.step(wL) + wL * 0.15f);
            const float bedR = m_heavyRainFilterR.process(m_heavyRainPinkR.step(wR) + wR * 0.15f);
            if (std::abs(wL) > 0.991f)
                m_heavyDropL = 0.85f;
            if (std::abs(wR) > 0.991f)
                m_heavyDropR = 0.85f;
            m_heavyDropL *= 0.978f;
            m_heavyDropR *= 0.978f;
            mixL += (bedL * 0.50f + wL * m_heavyDropL * 0.18f) * g;
            mixR += (bedR * 0.50f + wR * m_heavyDropR * 0.18f) * g;
        }

        // 5: Distant Thunder
        if (const float g = chanGain(DistantThunder); g > 0.0f) {
            const float wL = nextWhite();
            const float wR = nextWhite();
            m_thunderBrownL = std::clamp((m_thunderBrownL + 0.03f * wL) * 0.995f, -1.0f, 1.0f);
            m_thunderBrownR = std::clamp((m_thunderBrownR + 0.03f * wR) * 0.995f, -1.0f, 1.0f);
            if (std::abs(wL) > 0.99993f) {
                m_thunderTarget = 1.0f;
            }
            m_thunderTarget *= 0.99995f;
            m_thunderEnvL += (m_thunderTarget - m_thunderEnvL) * 0.00025f;
            m_thunderEnvR += (m_thunderEnvL - m_thunderEnvR) * 0.00015f; // Stereo distance lag
            const float rumbleBase = 0.25f + 0.75f * m_thunderEnvL;
            const float rL = m_thunderFilterL.process(m_thunderBrownL) * rumbleBase;
            const float rR = m_thunderFilterR.process(m_thunderBrownR) * (0.25f + 0.75f * m_thunderEnvR);
            mixL += rL * 0.90f * g;
            mixR += rR * 0.90f * g;
        }

        // 6: Ocean Waves
        if (const float g = chanGain(OceanWaves); g > 0.0f) {
            const float swellL = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(float(t) * TwoPi * 0.11f));
            const float swellR = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(float(t) * TwoPi * 0.11f + 0.7f));
            const float surfL = m_oceanFilterL.process(m_oceanPinkL.step(nextWhite())) * swellL;
            const float surfR = m_oceanFilterR.process(m_oceanPinkR.step(nextWhite())) * swellR;
            mixL += surfL * 0.70f * g;
            mixR += surfR * 0.70f * g;
        }

        // 7: Flowing Stream
        if (const float g = chanGain(FlowingStream); g > 0.0f) {
            const float pL = m_streamPinkL.step(nextWhite());
            const float pR = m_streamPinkR.step(nextWhite());
            const float ripple = 0.6f + 0.4f * std::sin(float(t) * TwoPi * 4.3f + pL * 2.0f);
            const float sL = m_streamFilter1L.process(pL) + m_streamFilter2L.process(pL) * ripple;
            const float sR = m_streamFilter1R.process(pR) + m_streamFilter2R.process(pR) * ripple;
            mixL += sL * 0.65f * g;
            mixR += sR * 0.65f * g;
        }

        // 8: Wind
        if (const float g = chanGain(Wind); g > 0.0f) {
            const float gust = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(float(t) * TwoPi * 0.08f));
            const float wL = m_windFilterL.process(m_windPinkL.step(nextWhite())) * gust;
            const float wR = m_windFilterR.process(m_windPinkR.step(nextWhite())) * gust;
            mixL += wL * 0.90f * g;
            mixR += wR * 0.90f * g;
        }

        // 9: Crackling Campfire
        if (const float g = chanGain(Campfire); g > 0.0f) {
            const float wL = nextWhite();
            const float wR = nextWhite();
            m_campfireBrownL = std::clamp((m_campfireBrownL + 0.02f * wL) * 0.994f, -1.0f, 1.0f);
            m_campfireBrownR = std::clamp((m_campfireBrownR + 0.02f * wR) * 0.994f, -1.0f, 1.0f);
            if (std::abs(wL) > 0.9975f)
                m_campfireCrackleL = 1.0f;
            if (std::abs(wR) > 0.9975f)
                m_campfireCrackleR = 1.0f;
            m_campfireCrackleL *= 0.965f;
            m_campfireCrackleR *= 0.965f;
            const float crackL = m_campfireCrackFilterL.process(wL * m_campfireCrackleL);
            const float crackR = m_campfireCrackFilterR.process(wR * m_campfireCrackleR);
            mixL += (m_campfireBrownL * 0.35f + crackL * 0.55f) * g;
            mixR += (m_campfireBrownR * 0.35f + crackR * 0.55f) * g;
        }

        // 10: Forest Night (Crickets + soft night air)
        if (const float g = chanGain(ForestNight); g > 0.0f) {
            m_forestPhase1 += 3850.0 / double(SampleRate);
            if (m_forestPhase1 >= 1.0)
                m_forestPhase1 -= 1.0;
            m_forestPhase2 += 4120.0 / double(SampleRate);
            if (m_forestPhase2 >= 1.0)
                m_forestPhase2 -= 1.0;

            const float pulse1 = std::max(0.0f, std::sin(float(t) * TwoPi * 3.2f)) *
                                 std::max(0.0f, std::sin(float(t) * TwoPi * 24.0f));
            const float pulse2 = std::max(0.0f, std::sin(float(t) * TwoPi * 2.7f + 1.5f)) *
                                 std::max(0.0f, std::sin(float(t) * TwoPi * 21.0f));
            const float chirpL = std::sin(float(m_forestPhase1) * TwoPi) * pulse1 * 0.12f;
            const float chirpR = std::sin(float(m_forestPhase2) * TwoPi) * pulse2 * 0.12f;
            const float breezeL = nextWhite() * 0.04f;
            const float breezeR = nextWhite() * 0.04f;
            mixL += (chirpL + chirpR * 0.3f + breezeL) * g;
            mixR += (chirpR + chirpL * 0.3f + breezeR) * g;
        }

        // 11: Box Fan
        if (const float g = chanGain(BoxFan); g > 0.0f) {
            m_fanPhase += 58.0 / double(SampleRate);
            if (m_fanPhase >= 1.0)
                m_fanPhase -= 1.0;
            const float hum = std::sin(float(m_fanPhase) * TwoPi) * 0.14f +
                              std::sin(float(m_fanPhase) * TwoPi * 2.0f) * 0.07f;
            const float airL = m_fanFilterL.process(nextWhite()) * 0.45f;
            const float airR = m_fanFilterR.process(nextWhite()) * 0.45f;
            mixL += (hum + airL) * g;
            mixR += (hum + airR) * g;
        }

        // 12: Train Tracks
        if (const float g = chanGain(TrainTracks); g > 0.0f) {
            const float wL = nextWhite();
            const float wR = nextWhite();
            m_trainBrownL = std::clamp((m_trainBrownL + 0.025f * wL) * 0.995f, -1.0f, 1.0f);
            m_trainBrownR = std::clamp((m_trainBrownR + 0.025f * wR) * 0.995f, -1.0f, 1.0f);
            // Rhythmic double rail-joint clack every 1.6 seconds
            const float cycle = std::fmod(float(t), 1.6f);
            float clack = 0.0f;
            if (cycle < 0.045f) {
                clack = std::exp(-cycle * 90.0f);
            } else if (cycle > 0.18f && cycle < 0.225f) {
                clack = std::exp(-(cycle - 0.18f) * 90.0f) * 0.85f;
            }
            mixL += (m_trainBrownL * 0.45f + wL * clack * 0.22f) * g;
            mixR += (m_trainBrownR * 0.45f + wR * clack * 0.22f) * g;
        }

        // 13: Cafe Hum
        if (const float g = chanGain(CafeHum); g > 0.0f) {
            const float pL = m_cafePinkL.step(nextWhite());
            const float pR = m_cafePinkR.step(nextWhite());
            const float syllableModL = 0.55f + 0.45f * std::sin(float(t) * TwoPi * 3.8f + std::sin(float(t) * 1.9f));
            const float syllableModR = 0.55f + 0.45f * std::sin(float(t) * TwoPi * 4.2f + 1.2f);
            const float cL = (m_cafeFilter1L.process(pL) + m_cafeFilter2L.process(pL) * 0.6f) * syllableModL;
            const float cR = (m_cafeFilter1R.process(pR) + m_cafeFilter2R.process(pR) * 0.6f) * syllableModR;
            mixL += cL * 0.70f * g;
            mixR += cR * 0.70f * g;
        }

        // 14: Binaural Alpha Focus (10 Hz: 200 Hz L / 210 Hz R)
        if (const float g = chanGain(BinauralAlpha); g > 0.0f) {
            m_alphaPhaseL += 200.0 / double(SampleRate);
            if (m_alphaPhaseL >= 1.0)
                m_alphaPhaseL -= 1.0;
            m_alphaPhaseR += 210.0 / double(SampleRate);
            if (m_alphaPhaseR >= 1.0)
                m_alphaPhaseR -= 1.0;
            mixL += std::sin(float(m_alphaPhaseL) * TwoPi) * 0.22f * g;
            mixR += std::sin(float(m_alphaPhaseR) * TwoPi) * 0.22f * g;
        }

        // 15: Binaural Theta Meditation (6 Hz: 180 Hz L / 186 Hz R)
        if (const float g = chanGain(BinauralTheta); g > 0.0f) {
            m_thetaPhaseL += 180.0 / double(SampleRate);
            if (m_thetaPhaseL >= 1.0)
                m_thetaPhaseL -= 1.0;
            m_thetaPhaseR += 186.0 / double(SampleRate);
            if (m_thetaPhaseR >= 1.0)
                m_thetaPhaseR -= 1.0;
            mixL += std::sin(float(m_thetaPhaseL) * TwoPi) * 0.22f * g;
            mixR += std::sin(float(m_thetaPhaseR) * TwoPi) * 0.22f * g;
        }

        // 16: Binaural Delta Sleep (2.5 Hz: 140 Hz L / 142.5 Hz R)
        if (const float g = chanGain(BinauralDelta); g > 0.0f) {
            m_deltaPhaseL += 140.0 / double(SampleRate);
            if (m_deltaPhaseL >= 1.0)
                m_deltaPhaseL -= 1.0;
            m_deltaPhaseR += 142.5 / double(SampleRate);
            if (m_deltaPhaseR >= 1.0)
                m_deltaPhaseR -= 1.0;
            mixL += std::sin(float(m_deltaPhaseL) * TwoPi) * 0.24f * g;
            mixR += std::sin(float(m_deltaPhaseR) * TwoPi) * 0.24f * g;
        }

        // Soft tanh limiter on master output to prevent clipping when many channels are layered
        stereoOut[f * 2] = std::tanh(mixL * m_currentMasterGain);
        stereoOut[f * 2 + 1] = std::tanh(mixR * m_currentMasterGain);
    }
}
