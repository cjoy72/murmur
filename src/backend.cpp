#include "backend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QSaveFile>
#include <algorithm>
#include <cmath>

Backend::Backend(const QString &stateFilePath, bool enableAudioOutput, QObject *parent)
    : QObject(parent),
      m_stateFilePath(stateFilePath.isEmpty() ? QDir::homePath() + "/.config/murmur/state.json" : stateFilePath),
      m_engine(enableAudioOutput, this) {
    initChannels();
    initDefaultPresets();

    m_secondTimer.setInterval(1000);
    connect(&m_secondTimer, &QTimer::timeout, this, [this] { tickTimer(1); });

    m_statusTimer.setSingleShot(true);
    m_statusTimer.setInterval(2600);
    connect(&m_statusTimer, &QTimer::timeout, this, [this] {
        if (!m_statusMessage.isEmpty()) {
            m_statusMessage.clear();
            emit statusMessageChanged();
        }
    });

    loadState();

    // If no channels were active in saved state, load Preset 1 (Rainy Study) ready to play
    if (activeCount() == 0) {
        loadPreset(1);
        setPlaying(false);
        m_statusMessage.clear();
    }
}

void Backend::initChannels() {
    m_channels = {
        {"white", QT_TR_NOOP("White Noise"), QT_TR_NOOP("Full spectrum static"), "noises", "wave", 0.0, 0.45},
        {"pink", QT_TR_NOOP("Pink Noise"), QT_TR_NOOP("Balanced -3 dB/oct"), "noises", "equalizer", 0.0, 0.50},
        {"brown", QT_TR_NOOP("Brown Noise"), QT_TR_NOOP("Deep warm -6 dB/oct"), "noises", "bass", 0.0, 0.65},
        {"rain", QT_TR_NOOP("Gentle Rain"), QT_TR_NOOP("Soft window droplets"), "weather", "rain", 0.0, 0.60},
        {"heavy_rain", QT_TR_NOOP("Heavy Rain"), QT_TR_NOOP("Dense storm downpour"), "weather", "storm", 0.0, 0.55},
        {"thunder", QT_TR_NOOP("Distant Thunder"), QT_TR_NOOP("Low rolling rumbles"), "weather", "bolt", 0.0, 0.40},
        {"ocean", QT_TR_NOOP("Ocean Waves"), QT_TR_NOOP("Rhythmic coastal surf"), "weather", "ocean", 0.0, 0.55},
        {"stream", QT_TR_NOOP("Flowing Stream"), QT_TR_NOOP("Babbling mountain creek"), "weather", "stream", 0.0, 0.55},
        {"wind", QT_TR_NOOP("Wind"), QT_TR_NOOP("Sweeping valley breeze"), "weather", "wind", 0.0, 0.45},
        {"campfire", QT_TR_NOOP("Crackling Campfire"), QT_TR_NOOP("Warm hearth & embers"), "cozy", "fire", 0.0, 0.60},
        {"forest", QT_TR_NOOP("Forest Night"), QT_TR_NOOP("Crickets & night air"), "cozy", "forest", 0.0, 0.50},
        {"fan", QT_TR_NOOP("Box Fan"), QT_TR_NOOP("Steady bedroom hum"), "cozy", "fan", 0.0, 0.50},
        {"train", QT_TR_NOOP("Train Tracks"), QT_TR_NOOP("Rhythmic sleeper coach"), "cozy", "train", 0.0, 0.55},
        {"cafe", QT_TR_NOOP("Cafe Hum"), QT_TR_NOOP("Warm coffee shop murmur"), "cozy", "cafe", 0.0, 0.45},
        {"alpha", QT_TR_NOOP("Alpha Focus"), QT_TR_NOOP("10 Hz relaxed alertness"), "binaural", "alpha", 0.0, 0.35},
        {"theta", QT_TR_NOOP("Theta Rest"), QT_TR_NOOP("6 Hz deep meditation"), "binaural", "theta", 0.0, 0.35},
        {"delta", QT_TR_NOOP("Delta Sleep"), QT_TR_NOOP("2.5 Hz restorative sleep"), "binaural", "delta", 0.0, 0.35},
    };
}

void Backend::initDefaultPresets() {
    auto makeVols = [](std::initializer_list<std::pair<int, double>> pairs) {
        QVector<double> v(SoundEngine::ChannelCount, 0.0);
        for (const auto &p : pairs) {
            if (p.first >= 0 && p.first < SoundEngine::ChannelCount)
                v[p.first] = p.second;
        }
        return v;
    };

    m_presets = {
        {1, QT_TR_NOOP("Rainy Study"), makeVols({{SoundEngine::GentleRain, 0.60}, {SoundEngine::DistantThunder, 0.35}, {SoundEngine::PinkNoise, 0.20}}), false},
        {2, QT_TR_NOOP("Deep Brown"), makeVols({{SoundEngine::BrownNoise, 0.65}, {SoundEngine::BoxFan, 0.30}, {SoundEngine::BinauralAlpha, 0.25}}), false},
        {3, QT_TR_NOOP("Coastal Fire"), makeVols({{SoundEngine::OceanWaves, 0.55}, {SoundEngine::Campfire, 0.60}, {SoundEngine::Wind, 0.25}}), false},
        {4, QT_TR_NOOP("Night Train"), makeVols({{SoundEngine::TrainTracks, 0.60}, {SoundEngine::GentleRain, 0.45}, {SoundEngine::BrownNoise, 0.30}}), false},
        {5, QT_TR_NOOP("Forest Creek"), makeVols({{SoundEngine::FlowingStream, 0.55}, {SoundEngine::ForestNight, 0.50}, {SoundEngine::Wind, 0.25}}), false},
        {6, QT_TR_NOOP("Cozy Cafe"), makeVols({{SoundEngine::CafeHum, 0.50}, {SoundEngine::GentleRain, 0.50}, {SoundEngine::Campfire, 0.30}}), false},
        {7, QT_TR_NOOP("Storm Cabin"), makeVols({{SoundEngine::HeavyRain, 0.65}, {SoundEngine::DistantThunder, 0.55}, {SoundEngine::Campfire, 0.50}}), false},
        {8, QT_TR_NOOP("Theta Zen"), makeVols({{SoundEngine::BinauralTheta, 0.40}, {SoundEngine::FlowingStream, 0.45}, {SoundEngine::PinkNoise, 0.20}}), false},
        {9, QT_TR_NOOP("Delta Sleep"), makeVols({{SoundEngine::BinauralDelta, 0.40}, {SoundEngine::BrownNoise, 0.55}, {SoundEngine::OceanWaves, 0.35}}), false},
    };
}

QVariantList Backend::channels() const {
    QVariantList list;
    list.reserve(m_channels.size());
    for (int i = 0; i < m_channels.size(); ++i) {
        const auto &c = m_channels[i];
        QVariantMap map;
        map["index"] = i;
        map["id"] = c.id;
        map["name"] = c.name;
        map["subtitle"] = c.subtitle;
        map["category"] = c.category;
        map["icon"] = c.icon;
        map["volume"] = c.volume;
        map["active"] = c.volume > 0.001;
        list.append(map);
    }
    return list;
}

int Backend::activeCount() const {
    int count = 0;
    for (const auto &c : m_channels) {
        if (c.volume > 0.001)
            ++count;
    }
    return count;
}

void Backend::setPlaying(bool playing) {
    if (m_playing == playing)
        return;
    m_playing = playing;
    m_engine.setPlaying(playing);
    emit playingChanged();
}

void Backend::setMasterVolume(double volume) {
    const double clamped = std::clamp(volume, 0.0, 1.0);
    if (qFuzzyCompare(m_masterVolume, clamped))
        return;
    m_masterVolume = clamped;
    m_engine.setMasterVolume(static_cast<float>(m_masterVolume));
    emit masterVolumeChanged();
    saveState();
}

void Backend::setMuted(bool muted) {
    if (m_muted == muted)
        return;
    m_muted = muted;
    m_engine.setMuted(m_muted);
    emit mutedChanged();
    showStatus(m_muted ? tr("Muted") : tr("Unmuted"));
}

void Backend::setWaveModulation(bool enabled) {
    if (m_waveModulation == enabled)
        return;
    m_waveModulation = enabled;
    m_engine.setWaveModulation(m_waveModulation);
    emit waveModulationChanged();
    saveState();
    showStatus(m_waveModulation ? tr("Organic wave modulation enabled") : tr("Organic wave modulation disabled"));
}

void Backend::setSelectedCategory(const QString &category) {
    if (m_selectedCategory == category)
        return;
    m_selectedCategory = category;
    emit selectedCategoryChanged();
}

QString Backend::timerLabel() const {
    if (m_timerRemainingSeconds <= 0)
        return tr("Off");
    const int mins = m_timerRemainingSeconds / 60;
    const int secs = m_timerRemainingSeconds % 60;
    return QString("%1:%2").arg(mins, 2, 10, QLatin1Char('0')).arg(secs, 2, 10, QLatin1Char('0'));
}

QVariantList Backend::presets() const {
    QVariantList list;
    list.reserve(m_presets.size());
    for (const auto &p : m_presets) {
        QVariantMap map;
        map["slot"] = p.slot;
        map["name"] = p.name;
        map["isCustom"] = p.isCustom;
        map["active"] = (m_activePreset == p.slot);
        list.append(map);
    }
    return list;
}

void Backend::primaryAction() {
    if (!m_playing && activeCount() == 0) {
        loadPreset(1);
    }
    setPlaying(!m_playing);
}

void Backend::toggleChannel(int index) {
    if (index < 0 || index >= m_channels.size())
        return;
    auto &c = m_channels[index];
    if (c.volume > 0.001) {
        c.lastNonZeroVolume = c.volume;
        c.volume = 0.0;
    } else {
        c.volume = std::clamp(c.lastNonZeroVolume, 0.15, 1.0);
        if (!m_playing)
            setPlaying(true);
    }
    m_engine.setChannelVolume(index, static_cast<float>(c.volume));
    m_activePreset = 0;
    emit channelsChanged();
    emit presetsChanged();
    saveState();
}

void Backend::setChannelVolume(int index, double volume) {
    if (index < 0 || index >= m_channels.size())
        return;
    const double clamped = std::clamp(volume, 0.0, 1.0);
    auto &c = m_channels[index];
    if (qFuzzyCompare(c.volume, clamped))
        return;
    c.volume = clamped;
    if (clamped > 0.05)
        c.lastNonZeroVolume = clamped;
    m_engine.setChannelVolume(index, static_cast<float>(clamped));
    if (clamped > 0.001 && !m_playing)
        setPlaying(true);
    m_activePreset = 0;
    emit channelsChanged();
    emit presetsChanged();
    saveState();
}

double Backend::channelVolume(int index) const {
    if (index < 0 || index >= m_channels.size())
        return 0.0;
    return m_channels[index].volume;
}

void Backend::adjustMasterVolume(double delta) {
    setMasterVolume(m_masterVolume + delta);
}

void Backend::toggleMute() {
    setMuted(!m_muted);
}

void Backend::toggleWaveModulation() {
    setWaveModulation(!m_waveModulation);
}

void Backend::resetMix() {
    for (int i = 0; i < m_channels.size(); ++i) {
        m_channels[i].volume = 0.0;
        m_engine.setChannelVolume(i, 0.0f);
    }
    m_activePreset = 0;
    setPlaying(false);
    emit channelsChanged();
    emit presetsChanged();
    saveState();
    showStatus(tr("Mix cleared"));
}

void Backend::randomizeMix() {
    for (int i = 0; i < m_channels.size(); ++i) {
        m_channels[i].volume = 0.0;
        m_engine.setChannelVolume(i, 0.0f);
    }
    // Pick 3 distinct channels for an instant balanced soundscape
    QVector<int> pool;
    for (int i = 0; i < m_channels.size(); ++i)
        pool.append(i);
    for (int k = 0; k < 3 && !pool.isEmpty(); ++k) {
        const int pickIdx = QRandomGenerator::global()->bounded(pool.size());
        const int chan = pool.takeAt(pickIdx);
        const double vol = 0.30 + (QRandomGenerator::global()->bounded(40) / 100.0);
        m_channels[chan].volume = vol;
        m_channels[chan].lastNonZeroVolume = vol;
        m_engine.setChannelVolume(chan, static_cast<float>(vol));
    }
    m_activePreset = 0;
    setPlaying(true);
    emit channelsChanged();
    emit presetsChanged();
    saveState();
    showStatus(tr("Shuffled ambient mix"));
}

void Backend::cycleSleepTimer() {
    static const QVector<int> steps = {0, 15, 30, 45, 60, 90, 120};
    const int currentMins = (m_timerTotalSeconds + 59) / 60;
    int nextMins = 15;
    for (int i = 0; i < steps.size(); ++i) {
        if (currentMins == steps[i]) {
            nextMins = steps[(i + 1) % steps.size()];
            break;
        }
    }
    setSleepTimerMinutes(nextMins);
}

void Backend::setSleepTimerMinutes(int minutes) {
    if (minutes <= 0) {
        setSleepTimerSeconds(0, 60);
        showStatus(tr("Sleep timer off"));
    } else {
        setSleepTimerSeconds(minutes * 60, 60);
        showStatus(tr("Sleep timer set to %1 min").arg(minutes));
    }
}

void Backend::setSleepTimerSeconds(int seconds, int fadeWindowSeconds) {
    m_timerFadeWindowSeconds = std::max(1, fadeWindowSeconds);
    if (seconds <= 0) {
        m_timerRemainingSeconds = 0;
        m_timerTotalSeconds = 0;
        m_fadeMultiplier = 1.0;
        m_engine.setFadeMultiplier(1.0f);
        m_secondTimer.stop();
    } else {
        m_timerRemainingSeconds = seconds;
        m_timerTotalSeconds = seconds;
        m_fadeMultiplier = 1.0;
        m_engine.setFadeMultiplier(1.0f);
        m_secondTimer.start();
    }
    emit timerChanged();
}

void Backend::tickTimer(int elapsedSeconds) {
    if (m_timerRemainingSeconds <= 0)
        return;

    m_timerRemainingSeconds = std::max(0, m_timerRemainingSeconds - std::max(1, elapsedSeconds));
    if (m_timerRemainingSeconds == 0) {
        m_fadeMultiplier = 0.0;
        m_engine.setFadeMultiplier(0.0f);
        m_secondTimer.stop();
        setPlaying(false);
        // Restore multiplier to 1.0 after pausing so manual resume plays at normal volume
        m_timerTotalSeconds = 0;
        m_fadeMultiplier = 1.0;
        m_engine.setFadeMultiplier(1.0f);
        showStatus(tr("Sleep timer finished"));
    } else if (m_timerRemainingSeconds <= m_timerFadeWindowSeconds) {
        m_fadeMultiplier = static_cast<double>(m_timerRemainingSeconds) / static_cast<double>(m_timerFadeWindowSeconds);
        m_engine.setFadeMultiplier(static_cast<float>(m_fadeMultiplier));
    } else {
        m_fadeMultiplier = 1.0;
        m_engine.setFadeMultiplier(1.0f);
    }
    emit timerChanged();
}

void Backend::loadPreset(int slot) {
    if (slot < 1 || slot > m_presets.size())
        return;
    const auto &preset = m_presets[slot - 1];
    for (int i = 0; i < m_channels.size() && i < preset.volumes.size(); ++i) {
        m_channels[i].volume = preset.volumes[i];
        if (preset.volumes[i] > 0.05)
            m_channels[i].lastNonZeroVolume = preset.volumes[i];
        m_engine.setChannelVolume(i, static_cast<float>(preset.volumes[i]));
    }
    m_activePreset = slot;
    setPlaying(true);
    emit channelsChanged();
    emit presetsChanged();
    saveState();
    showStatus(tr("Loaded preset %1: %2").arg(slot).arg(tr(preset.name.toUtf8().constData())));
}

void Backend::savePreset(int slot, const QString &customName) {
    if (slot < 1 || slot > m_presets.size())
        return;
    auto &preset = m_presets[slot - 1];
    preset.volumes.resize(m_channels.size());
    for (int i = 0; i < m_channels.size(); ++i) {
        preset.volumes[i] = m_channels[i].volume;
    }
    preset.isCustom = true;
    if (!customName.trimmed().isEmpty()) {
        preset.name = customName.trimmed();
    } else if (!preset.name.startsWith(QLatin1String("Mix "))) {
        preset.name = tr("Custom %1").arg(slot);
    }
    m_activePreset = slot;
    emit presetsChanged();
    saveState();
    showStatus(tr("Saved current mix to preset %1").arg(slot));
}

void Backend::saveCurrentToActiveOrNextPreset() {
    int targetSlot = m_activePreset > 0 ? m_activePreset : 1;
    for (const auto &p : m_presets) {
        if (!p.isCustom) {
            targetSlot = p.slot;
            break;
        }
    }
    savePreset(targetSlot);
}

void Backend::loadState() {
    QFile file(m_stateFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    if (root.isEmpty())
        return;

    if (root.contains("masterVolume")) {
        m_masterVolume = std::clamp(root.value("masterVolume").toDouble(0.80), 0.0, 1.0);
        m_engine.setMasterVolume(static_cast<float>(m_masterVolume));
    }
    if (root.contains("waveModulation")) {
        m_waveModulation = root.value("waveModulation").toBool(true);
        m_engine.setWaveModulation(m_waveModulation);
    }
    if (root.contains("activePreset")) {
        m_activePreset = root.value("activePreset").toInt(0);
    }

    const QJsonObject chanObj = root.value("channels").toObject();
    for (int i = 0; i < m_channels.size(); ++i) {
        const QString id = m_channels[i].id;
        if (chanObj.contains(id)) {
            const double v = std::clamp(chanObj.value(id).toDouble(0.0), 0.0, 1.0);
            m_channels[i].volume = v;
            if (v > 0.05)
                m_channels[i].lastNonZeroVolume = v;
            m_engine.setChannelVolume(i, static_cast<float>(v));
        }
    }

    const QJsonArray presetArr = root.value("presets").toArray();
    for (const auto &val : presetArr) {
        const QJsonObject pObj = val.toObject();
        const int slot = pObj.value("slot").toInt(0);
        if (slot < 1 || slot > m_presets.size())
            continue;
        auto &preset = m_presets[slot - 1];
        if (pObj.contains("name"))
            preset.name = pObj.value("name").toString();
        preset.isCustom = pObj.value("isCustom").toBool(false);
        const QJsonObject vols = pObj.value("volumes").toObject();
        if (!vols.isEmpty()) {
            for (int i = 0; i < m_channels.size(); ++i) {
                preset.volumes[i] = std::clamp(vols.value(m_channels[i].id).toDouble(0.0), 0.0, 1.0);
            }
        }
    }
}

void Backend::saveState() const {
    if (m_stateFilePath.isEmpty())
        return;

    const QFileInfo info(m_stateFilePath);
    QDir().mkpath(info.absolutePath());

    QJsonObject root;
    root["masterVolume"] = m_masterVolume;
    root["waveModulation"] = m_waveModulation;
    root["activePreset"] = m_activePreset;

    QJsonObject chanObj;
    for (const auto &c : m_channels) {
        chanObj[c.id] = c.volume;
    }
    root["channels"] = chanObj;

    QJsonArray presetArr;
    for (const auto &p : m_presets) {
        QJsonObject pObj;
        pObj["slot"] = p.slot;
        pObj["name"] = p.name;
        pObj["isCustom"] = p.isCustom;
        QJsonObject vols;
        for (int i = 0; i < m_channels.size() && i < p.volumes.size(); ++i) {
            vols[m_channels[i].id] = p.volumes[i];
        }
        pObj["volumes"] = vols;
        presetArr.append(pObj);
    }
    root["presets"] = presetArr;

    QSaveFile file(m_stateFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.commit();
    }
}

void Backend::showStatus(const QString &message) {
    m_statusMessage = message;
    emit statusMessageChanged();
    m_statusTimer.start();
}
