#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>

#include "backend.h"
#include "soundengine.h"
#include "theme.h"
#include "translator.h"

class MurmurTests : public QObject {
    Q_OBJECT

private slots:
    void primaryActionStartsAndPausesPlayback() {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), false);
        QVERIFY(!backend.isPlaying());
        QVERIFY(backend.activeCount() > 0);

        backend.primaryAction();
        QVERIFY(backend.isPlaying());

        backend.primaryAction();
        QVERIFY(!backend.isPlaying());
    }

    void soundEngineSynthesizesEveryChannel() {
        SoundEngine engine(false);
        engine.setPlaying(true);
        engine.setMasterVolume(0.9f);
        engine.setWaveModulation(false);

        QVector<float> buffer(2048 * 2, 0.0f);

        for (int ch = 0; ch < SoundEngine::ChannelCount; ++ch) {
            for (int i = 0; i < SoundEngine::ChannelCount; ++i)
                engine.setChannelVolume(i, 0.0f);

            engine.setChannelVolume(ch, 0.85f);
            // Warm up smoothing + synthesis state
            engine.renderFrames(buffer.data(), 2048);
            engine.renderFrames(buffer.data(), 2048);

            double energyL = 0.0;
            double energyR = 0.0;
            float maxAbs = 0.0f;
            for (int f = 0; f < 2048; ++f) {
                const float sL = buffer[f * 2];
                const float sR = buffer[f * 2 + 1];
                energyL += double(sL) * double(sL);
                energyR += double(sR) * double(sR);
                maxAbs = std::max(maxAbs, std::max(std::abs(sL), std::abs(sR)));
            }
            QVERIFY2(energyL > 1e-4, qPrintable(QString("Channel %1 left energy too low").arg(ch)));
            QVERIFY2(energyR > 1e-4, qPrintable(QString("Channel %1 right energy too low").arg(ch)));
            QVERIFY2(maxAbs <= 1.0f, qPrintable(QString("Channel %1 exceeded [-1, 1]").arg(ch)));
        }
    }

    void organicWaveModulationVariesGain() {
        SoundEngine engine(false);
        engine.setPlaying(true);
        engine.setWaveModulation(true);
        engine.setChannelVolume(SoundEngine::PinkNoise, 0.7f);

        QVector<float> buffer(4096 * 2, 0.0f);
        engine.renderFrames(buffer.data(), 4096);
        const float modStart = engine.channelModulationLevel(SoundEngine::PinkNoise);

        // Advance roughly 4 seconds (4 * 48000 samples)
        for (int i = 0; i < 48; ++i)
            engine.renderFrames(buffer.data(), 4000);
        const float modLater = engine.channelModulationLevel(SoundEngine::PinkNoise);

        QVERIFY(std::abs(modLater - modStart) > 0.05f);
    }

    void presetsAndPersistenceRoundTrip() {
        QTemporaryDir dir;
        const QString stateFile = dir.filePath("state.json");

        {
            Backend backend(stateFile, false);
            backend.resetMix();
            QCOMPARE(backend.activeCount(), 0);

            backend.setChannelVolume(SoundEngine::BrownNoise, 0.75);
            backend.setChannelVolume(SoundEngine::Campfire, 0.50);
            backend.setMasterVolume(0.65);
            backend.savePreset(4, "Late Coding");
            QCOMPARE(backend.activePreset(), 4);
        }

        {
            Backend reloaded(stateFile, false);
            QCOMPARE(reloaded.activePreset(), 4);
            QVERIFY(std::abs(reloaded.masterVolume() - 0.65) < 0.001);
            QVERIFY(std::abs(reloaded.channelVolume(SoundEngine::BrownNoise) - 0.75) < 0.001);
            QVERIFY(std::abs(reloaded.channelVolume(SoundEngine::Campfire) - 0.50) < 0.001);

            reloaded.loadPreset(2);
            QCOMPARE(reloaded.activePreset(), 2);
            QVERIFY(reloaded.channelVolume(SoundEngine::Campfire) < 0.001);

            reloaded.loadPreset(4);
            QVERIFY(std::abs(reloaded.channelVolume(SoundEngine::Campfire) - 0.50) < 0.001);
        }
    }

    void sleepTimerFadesOutAndStopsPlayback() {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), false);
        backend.loadPreset(1);
        QVERIFY(backend.isPlaying());

        backend.setSleepTimerSeconds(10, 4);
        QCOMPARE(backend.timerRemainingSeconds(), 10);
        QCOMPARE(backend.fadeMultiplier(), 1.0);

        backend.tickTimer(6);
        QCOMPARE(backend.timerRemainingSeconds(), 4);
        QCOMPARE(backend.fadeMultiplier(), 1.0);

        backend.tickTimer(2);
        QCOMPARE(backend.timerRemainingSeconds(), 2);
        QVERIFY(std::abs(backend.fadeMultiplier() - 0.5) < 0.01);
        QVERIFY(backend.isPlaying());

        backend.tickTimer(2);
        QCOMPARE(backend.timerRemainingSeconds(), 0);
        QVERIFY(!backend.isPlaying());
    }

    void themeFallsBack() {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
    }

    void themeReadsAccent() {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("accent = \"#112233\"\n");
        colors.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeFollowsHyprlandRounding() {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json"), command = dir.filePath("hyprctl");
        auto writeResp = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(writeResp(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(writeResp(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(writeResp(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish() {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile() {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})");
        write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms() {
        QTemporaryDir dir;
        write(dir.path() + "/language", "ru\n");
        write(dir.path() + "/en.json", R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})");
        write(dir.path() + "/ru.json", R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
    }

private:
    static void write(const QString &path, const QByteArray &contents) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(MurmurTests)
#include "murmur_tests.moc"
