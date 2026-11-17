import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1100
    height: 760
    minimumWidth: 780
    minimumHeight: 560
    visible: true
    title: qsTr("Murmur")
    color: "#0e0e10"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    property real wavePhase: 0.0

    Timer {
        interval: 60
        running: backend.playing && backend.waveModulation
        repeat: true
        onTriggered: win.wavePhase = (win.wavePhase + 0.08) % 6.28318
    }

    // Keyboard shortcuts
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; enabled: win.helpVisible; onActivated: win.helpVisible = false }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.primaryAction() }
    Shortcut { sequence: "M"; context: Qt.ApplicationShortcut; onActivated: backend.toggleMute() }
    Shortcut { sequence: "W"; context: Qt.ApplicationShortcut; onActivated: backend.toggleWaveModulation() }
    Shortcut { sequence: "T"; context: Qt.ApplicationShortcut; onActivated: backend.cycleSleepTimer() }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: backend.resetMix() }
    Shortcut { sequence: "0"; context: Qt.ApplicationShortcut; onActivated: backend.resetMix() }
    Shortcut { sequence: "C"; context: Qt.ApplicationShortcut; onActivated: backend.randomizeMix() }
    Shortcut { sequence: "S"; context: Qt.ApplicationShortcut; onActivated: backend.saveCurrentToActiveOrNextPreset() }
    Shortcut { sequence: "Up"; context: Qt.ApplicationShortcut; onActivated: backend.adjustMasterVolume(0.05) }
    Shortcut { sequence: "Down"; context: Qt.ApplicationShortcut; onActivated: backend.adjustMasterVolume(-0.05) }
    Shortcut { sequence: "+"; context: Qt.ApplicationShortcut; onActivated: backend.adjustMasterVolume(0.05) }
    Shortcut { sequence: "-"; context: Qt.ApplicationShortcut; onActivated: backend.adjustMasterVolume(-0.05) }

    Shortcut { sequence: "1"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(1) }
    Shortcut { sequence: "2"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(2) }
    Shortcut { sequence: "3"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(3) }
    Shortcut { sequence: "4"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(4) }
    Shortcut { sequence: "5"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(5) }
    Shortcut { sequence: "6"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(6) }
    Shortcut { sequence: "7"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(7) }
    Shortcut { sequence: "8"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(8) }
    Shortcut { sequence: "9"; context: Qt.ApplicationShortcut; onActivated: backend.loadPreset(9) }

    Shortcut { sequence: "Shift+1"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(1) }
    Shortcut { sequence: "Shift+2"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(2) }
    Shortcut { sequence: "Shift+3"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(3) }
    Shortcut { sequence: "Shift+4"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(4) }
    Shortcut { sequence: "Shift+5"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(5) }
    Shortcut { sequence: "Shift+6"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(6) }
    Shortcut { sequence: "Shift+7"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(7) }
    Shortcut { sequence: "Shift+8"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(8) }
    Shortcut { sequence: "Shift+9"; context: Qt.ApplicationShortcut; onActivated: backend.savePreset(9) }

    function formatPercent(val) {
        void(translator.language)
        return Math.round(val * 100).toLocaleString(Qt.locale(), 'f', 0) + "%"
    }

    function formatSlot(slot) {
        void(translator.language)
        return Number(slot).toLocaleString(Qt.locale(), 'f', 0)
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            // Audio device error banner (shown only if audio initialization failed)
            Rectangle {
                Layout.fillWidth: true
                visible: backend.audioError.length > 0
                height: visible ? 40 : 0
                radius: theme.radius
                color: "#3b181c"
                border.color: "#ff5555"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10
                    Text {
                        text: "!"
                        color: "#ff6b6b"
                        font.pixelSize: 15
                        font.bold: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.audioError
                        color: "#ffd6d6"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }
                }
            }

            // Top Control Header
            Rectangle {
                Layout.fillWidth: true
                height: 64
                radius: theme.radius
                color: "#16161a"
                border.color: "#26262e"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 14

                    // App title & active count badge
                    RowLayout {
                        spacing: 10

                        Rectangle {
                            width: 34
                            height: 34
                            radius: Math.min(4, theme.radius)
                            color: backend.playing ? theme.accent : "#23232b"

                            Canvas {
                                anchors.fill: parent
                                anchors.margins: 7
                                property string fgColor: backend.playing ? theme.accentForeground : theme.accent
                                onFgColorChanged: requestPaint()
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.reset()
                                    ctx.strokeStyle = fgColor
                                    ctx.lineWidth = 2.2
                                    ctx.lineCap = "round"
                                    var w = width, h = height
                                    var bars = [0.35, 0.75, 1.0, 0.65, 0.4]
                                    for (var i = 0; i < bars.length; ++i) {
                                        var x = (i + 0.5) * (w / bars.length)
                                        var bh = bars[i] * h
                                        ctx.beginPath()
                                        ctx.moveTo(x, (h - bh) / 2)
                                        ctx.lineTo(x, (h + bh) / 2)
                                        ctx.stroke()
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 1
                            Text {
                                text: qsTr("Murmur")
                                color: "#f4f4f6"
                                font.pixelSize: 16
                                font.bold: true
                            }
                            Text {
                                text: qsTr("%n active", "", backend.activeCount)
                                color: backend.activeCount > 0 ? theme.accent : "#7c7c88"
                                font.pixelSize: 11
                            }
                        }
                    }

                    // Play / Pause Primary Button (Space)
                    Rectangle {
                        id: playBtn
                        Layout.preferredWidth: playRow.implicitWidth + 26
                        height: 38
                        radius: theme.radius
                        color: backend.playing ? theme.accent : "#22222a"
                        border.color: backend.playing ? theme.accent : "#343440"
                        border.width: 1

                        RowLayout {
                            id: playRow
                            anchors.centerIn: parent
                            spacing: 8
                            Text {
                                text: backend.playing ? "❚❚" : "▶"
                                color: backend.playing ? theme.accentForeground : "#f0f0f4"
                                font.pixelSize: 12
                            }
                            Text {
                                text: backend.playing ? qsTr("Pause") : qsTr("Play")
                                color: backend.playing ? theme.accentForeground : "#f0f0f4"
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.primaryAction()
                        }
                    }

                    // Master Volume + Mute
                    RowLayout {
                        spacing: 8
                        Layout.preferredWidth: 210

                        Rectangle {
                            width: 32
                            height: 32
                            radius: Math.min(4, theme.radius)
                            color: backend.muted ? "#3a1d22" : "#22222a"
                            border.color: backend.muted ? "#ff5555" : "#30303a"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: backend.muted ? "✕" : "♪"
                                color: backend.muted ? "#ff6b6b" : "#d8d8e0"
                                font.pixelSize: 13
                                font.bold: true
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.toggleMute()
                            }
                        }

                        Slider {
                            id: masterSlider
                            Layout.fillWidth: true
                            from: 0.0
                            to: 1.0
                            value: backend.masterVolume
                            onMoved: backend.masterVolume = value
                        }

                        Text {
                            Layout.preferredWidth: 38
                            horizontalAlignment: Text.AlignRight
                            text: win.formatPercent(backend.masterVolume)
                            color: backend.muted ? "#777784" : "#d0d0da"
                            font.pixelSize: 12
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Organic Wave Modulation Toggle (W)
                    Rectangle {
                        Layout.preferredWidth: waveRow.implicitWidth + 22
                        height: 36
                        radius: theme.radius
                        color: backend.waveModulation ? Qt.rgba(Qt.color(theme.accent).r, Qt.color(theme.accent).g, Qt.color(theme.accent).b, 0.16) : "#202028"
                        border.color: backend.waveModulation ? theme.accent : "#2e2e38"
                        border.width: 1

                        RowLayout {
                            id: waveRow
                            anchors.centerIn: parent
                            spacing: 7

                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: backend.waveModulation ? theme.accent : "#666672"
                                scale: backend.playing && backend.waveModulation ? (0.85 + 0.25 * Math.sin(win.wavePhase)) : 1.0
                            }
                            Text {
                                text: qsTr("Wave")
                                color: backend.waveModulation ? "#f4f4f6" : "#9a9aa6"
                                font.pixelSize: 12
                                font.bold: backend.waveModulation
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.toggleWaveModulation()
                        }
                    }

                    // Sleep Timer Selector (T)
                    Rectangle {
                        Layout.preferredWidth: timerRow.implicitWidth + 22
                        height: 36
                        radius: theme.radius
                        color: backend.timerRemainingSeconds > 0 ? Qt.rgba(Qt.color(theme.accent).r, Qt.color(theme.accent).g, Qt.color(theme.accent).b, 0.16) : "#202028"
                        border.color: backend.timerRemainingSeconds > 0 ? theme.accent : "#2e2e38"
                        border.width: 1

                        RowLayout {
                            id: timerRow
                            anchors.centerIn: parent
                            spacing: 6
                            Text {
                                text: "◷"
                                color: backend.timerRemainingSeconds > 0 ? theme.accent : "#9a9aa6"
                                font.pixelSize: 13
                            }
                            Text {
                                text: backend.timerRemainingSeconds > 0
                                      ? (qsTr("Timer") + ": " + backend.timerLabel)
                                      : (qsTr("Timer") + ": " + qsTr("Off"))
                                color: backend.timerRemainingSeconds > 0 ? "#f4f4f6" : "#9a9aa6"
                                font.pixelSize: 12
                                font.bold: backend.timerRemainingSeconds > 0
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.cycleSleepTimer()
                        }
                    }

                    // Shuffle Mix Button (C)
                    Rectangle {
                        Layout.preferredWidth: shuffleText.implicitWidth + 20
                        height: 36
                        radius: theme.radius
                        color: "#202028"
                        border.color: "#2e2e38"
                        border.width: 1

                        Text {
                            id: shuffleText
                            anchors.centerIn: parent
                            text: qsTr("Shuffle")
                            color: "#c8c8d4"
                            font.pixelSize: 12
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.randomizeMix()
                        }
                    }

                    // Reset Mix Button (R)
                    Rectangle {
                        Layout.preferredWidth: resetText.implicitWidth + 20
                        height: 36
                        radius: theme.radius
                        color: "#202028"
                        border.color: "#2e2e38"
                        border.width: 1

                        Text {
                            id: resetText
                            anchors.centerIn: parent
                            text: qsTr("Reset")
                            color: "#c8c8d4"
                            font.pixelSize: 12
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.resetMix()
                        }
                    }

                    // Shortcuts Help Button (?)
                    Rectangle {
                        width: 36
                        height: 36
                        radius: theme.radius
                        color: win.helpVisible ? theme.accent : "#202028"
                        border.color: win.helpVisible ? theme.accent : "#2e2e38"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "?"
                            color: win.helpVisible ? theme.accentForeground : "#c8c8d4"
                            font.pixelSize: 14
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: win.helpVisible = !win.helpVisible
                        }
                    }
                }
            }

            // Presets Row (1 - 9)
            Rectangle {
                Layout.fillWidth: true
                height: 44
                radius: theme.radius
                color: "#141418"
                border.color: "#222229"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 6

                    Text {
                        text: qsTr("Presets")
                        color: "#8a8a98"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    // Horizontal list of 9 presets
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Repeater {
                            model: backend.presets
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 30
                                radius: Math.min(4, theme.radius)
                                color: modelData.active
                                       ? Qt.rgba(Qt.color(theme.accent).r, Qt.color(theme.accent).g, Qt.color(theme.accent).b, 0.20)
                                       : "#1d1d24"
                                border.color: modelData.active ? theme.accent : "#2b2b35"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 5
                                    anchors.rightMargin: 5
                                    spacing: 4

                                    Rectangle {
                                        width: 15
                                        height: 15
                                        radius: Math.min(3, theme.radius)
                                        color: modelData.active ? theme.accent : "#2a2a34"
                                        Text {
                                            anchors.centerIn: parent
                                            text: win.formatSlot(modelData.slot)
                                            color: modelData.active ? theme.accentForeground : "#9898a6"
                                            font.pixelSize: 9
                                            font.bold: true
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: qsTr(modelData.name)
                                        color: modelData.active ? "#f6f6f8" : "#c0c0cc"
                                        font.pixelSize: 11
                                        font.bold: modelData.active
                                        elide: Text.ElideRight
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: function(mouse) {
                                        if (mouse.button === Qt.RightButton) {
                                            backend.savePreset(modelData.slot)
                                        } else {
                                            backend.loadPreset(modelData.slot)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Save current mix button (S)
                    Rectangle {
                        Layout.preferredWidth: saveMixLabel.implicitWidth + 18
                        height: 32
                        radius: Math.min(4, theme.radius)
                        color: "#22222b"
                        border.color: "#343442"
                        border.width: 1

                        Text {
                            id: saveMixLabel
                            anchors.centerIn: parent
                            text: qsTr("Save Mix")
                            color: theme.accent
                            font.pixelSize: 11
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.saveCurrentToActiveOrNextPreset()
                        }
                    }
                }
            }

            // Category Filter Bar + Status Notification
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: [
                        { key: "all", label: QT_TR_NOOP("All Channels") },
                        { key: "noises", label: QT_TR_NOOP("Color Noises") },
                        { key: "weather", label: QT_TR_NOOP("Weather & Water") },
                        { key: "cozy", label: QT_TR_NOOP("Cozy & Ambient") },
                        { key: "binaural", label: QT_TR_NOOP("Binaural Beats") }
                    ]
                    delegate: Rectangle {
                        Layout.preferredWidth: catLabel.implicitWidth + 24
                        height: 32
                        radius: theme.radius
                        property bool selected: backend.selectedCategory === modelData.key
                        color: selected ? theme.accent : "#18181e"
                        border.color: selected ? theme.accent : "#282832"
                        border.width: 1

                        Text {
                            id: catLabel
                            anchors.centerIn: parent
                            text: qsTr(modelData.label)
                            color: parent.selected ? theme.accentForeground : "#b4b4c2"
                            font.pixelSize: 12
                            font.bold: parent.selected
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.selectedCategory = modelData.key
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Transient status feedback pill
                Rectangle {
                    visible: backend.statusMessage.length > 0
                    Layout.preferredWidth: statusLabel.implicitWidth + 20
                    height: 30
                    radius: theme.radius
                    color: "#1e1e26"
                    border.color: theme.accent
                    border.width: 1

                    Text {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: backend.statusMessage
                        color: "#ededf2"
                        font.pixelSize: 12
                    }
                }
            }

            // Soundscape Channels Grid
            Flickable {
                id: gridFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: width
                contentHeight: soundGrid.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                GridLayout {
                    id: soundGrid
                    width: gridFlick.width
                    columns: Math.max(2, Math.min(4, Math.floor(width / 230)))
                    columnSpacing: 10
                    rowSpacing: 10

                    Repeater {
                        model: backend.channels
                        delegate: Rectangle {
                            id: card
                            readonly property bool matchesCategory: backend.selectedCategory === "all" || backend.selectedCategory === modelData.category
                            visible: matchesCategory
                            Layout.fillWidth: true
                            Layout.preferredHeight: 96
                            radius: theme.radius
                            color: modelData.active
                                   ? Qt.rgba(Qt.color(theme.accent).r, Qt.color(theme.accent).g, Qt.color(theme.accent).b, 0.13)
                                   : "#16161b"
                            border.color: modelData.active ? theme.accent : "#262630"
                            border.width: modelData.active ? 1.5 : 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 11
                                spacing: 4

                                // Top part of card: click anywhere here to toggle channel
                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true

                                    RowLayout {
                                        anchors.fill: parent
                                        spacing: 11

                                        // Channel Icon Badge
                                        Rectangle {
                                            width: 38
                                            height: 38
                                            radius: Math.min(4, theme.radius)
                                            color: modelData.active ? theme.accent : "#22222b"

                                            Canvas {
                                                anchors.fill: parent
                                                anchors.margins: 8
                                                property string iconType: modelData.icon
                                                property string strokeCol: modelData.active ? theme.accentForeground : "#9e9eae"
                                                onStrokeColChanged: requestPaint()
                                                onIconTypeChanged: requestPaint()
                                                onPaint: {
                                                    var ctx = getContext("2d")
                                                    ctx.reset()
                                                    ctx.strokeStyle = strokeCol
                                                    ctx.fillStyle = strokeCol
                                                    ctx.lineWidth = 2.0
                                                    ctx.lineCap = "round"
                                                    ctx.lineJoin = "round"
                                                    var w = width, h = height

                                                    if (iconType === "wave" || iconType === "equalizer" || iconType === "bass") {
                                                        var heights = iconType === "wave" ? [0.5, 0.9, 0.6, 0.95, 0.55]
                                                                    : iconType === "equalizer" ? [0.9, 0.7, 0.55, 0.4, 0.25]
                                                                    : [1.0, 0.85, 0.5, 0.3, 0.15]
                                                        for (var i = 0; i < 5; ++i) {
                                                            var x = (i + 0.5) * (w / 5)
                                                            var bh = heights[i] * h
                                                            ctx.beginPath()
                                                            ctx.moveTo(x, (h - bh) / 2)
                                                            ctx.lineTo(x, (h + bh) / 2)
                                                            ctx.stroke()
                                                        }
                                                    } else if (iconType === "rain" || iconType === "storm") {
                                                        ctx.beginPath()
                                                        ctx.arc(w * 0.5, h * 0.38, w * 0.26, Math.PI, 0, false)
                                                        ctx.lineTo(w * 0.78, h * 0.45)
                                                        ctx.lineTo(w * 0.22, h * 0.45)
                                                        ctx.closePath()
                                                        ctx.stroke()
                                                        var drops = iconType === "storm" ? [0.25, 0.5, 0.75] : [0.35, 0.65]
                                                        for (var d = 0; d < drops.length; ++d) {
                                                            ctx.beginPath()
                                                            ctx.moveTo(w * drops[d], h * 0.60)
                                                            ctx.lineTo(w * (drops[d] - 0.08), h * 0.88)
                                                            ctx.stroke()
                                                        }
                                                    } else if (iconType === "bolt") {
                                                        ctx.beginPath()
                                                        ctx.moveTo(w * 0.58, h * 0.10)
                                                        ctx.lineTo(w * 0.28, h * 0.52)
                                                        ctx.lineTo(w * 0.52, h * 0.52)
                                                        ctx.lineTo(w * 0.40, h * 0.90)
                                                        ctx.stroke()
                                                    } else if (iconType === "ocean" || iconType === "stream" || iconType === "wind") {
                                                        var rows = iconType === "ocean" ? 2 : 3
                                                        for (var r = 0; r < rows; ++r) {
                                                            var y = h * (0.30 + r * 0.25)
                                                            ctx.beginPath()
                                                            ctx.moveTo(w * 0.1, y)
                                                            ctx.bezierCurveTo(w * 0.35, y - h * 0.18, w * 0.65, y + h * 0.18, w * 0.9, y)
                                                            ctx.stroke()
                                                        }
                                                    } else if (iconType === "fire") {
                                                        ctx.beginPath()
                                                        ctx.moveTo(w * 0.5, h * 0.12)
                                                        ctx.quadraticCurveTo(w * 0.85, h * 0.45, w * 0.72, h * 0.78)
                                                        ctx.quadraticCurveTo(w * 0.5, h * 0.95, w * 0.28, h * 0.78)
                                                        ctx.quadraticCurveTo(w * 0.15, h * 0.45, w * 0.5, h * 0.12)
                                                        ctx.stroke()
                                                    } else if (iconType === "forest") {
                                                        ctx.beginPath()
                                                        ctx.moveTo(w * 0.5, h * 0.12)
                                                        ctx.lineTo(w * 0.82, h * 0.68)
                                                        ctx.lineTo(w * 0.18, h * 0.68)
                                                        ctx.closePath()
                                                        ctx.stroke()
                                                        ctx.beginPath()
                                                        ctx.moveTo(w * 0.5, h * 0.68)
                                                        ctx.lineTo(w * 0.5, h * 0.90)
                                                        ctx.stroke()
                                                    } else if (iconType === "fan") {
                                                        ctx.beginPath()
                                                        ctx.arc(w * 0.5, h * 0.5, w * 0.38, 0, Math.PI * 2)
                                                        ctx.stroke()
                                                        ctx.beginPath()
                                                        ctx.moveTo(w * 0.2, h * 0.5)
                                                        ctx.lineTo(w * 0.8, h * 0.5)
                                                        ctx.moveTo(w * 0.5, h * 0.2)
                                                        ctx.lineTo(w * 0.5, h * 0.8)
                                                        ctx.stroke()
                                                    } else if (iconType === "train") {
                                                        ctx.strokeRect(w * 0.22, h * 0.15, w * 0.56, h * 0.55)
                                                        ctx.beginPath()
                                                        ctx.moveTo(w * 0.15, h * 0.88)
                                                        ctx.lineTo(w * 0.85, h * 0.88)
                                                        ctx.stroke()
                                                    } else if (iconType === "cafe") {
                                                        ctx.strokeRect(w * 0.20, h * 0.35, w * 0.48, h * 0.45)
                                                        ctx.beginPath()
                                                        ctx.arc(w * 0.68, h * 0.57, w * 0.14, -Math.PI / 2, Math.PI / 2)
                                                        ctx.stroke()
                                                    } else {
                                                        // Binaural concentric rings
                                                        ctx.beginPath()
                                                        ctx.arc(w * 0.38, h * 0.5, w * 0.26, 0, Math.PI * 2)
                                                        ctx.stroke()
                                                        ctx.beginPath()
                                                        ctx.arc(w * 0.62, h * 0.5, w * 0.26, 0, Math.PI * 2)
                                                        ctx.stroke()
                                                    }
                                                }
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2
                                            Text {
                                                Layout.fillWidth: true
                                                text: qsTr(modelData.name)
                                                color: modelData.active ? "#f6f6f8" : "#d0d0dc"
                                                font.pixelSize: 14
                                                font.bold: true
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: qsTr(modelData.subtitle)
                                                color: modelData.active ? "#b4b4c4" : "#787886"
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                            }
                                        }

                                        // Active breathing indicator pill
                                        Rectangle {
                                            width: 10
                                            height: 10
                                            radius: 5
                                            color: modelData.active ? theme.accent : "#2c2c36"
                                            opacity: modelData.active && backend.playing && backend.waveModulation
                                                     ? (0.55 + 0.45 * Math.sin(win.wavePhase + modelData.index * 0.9))
                                                     : 1.0
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: backend.toggleChannel(modelData.index)
                                    }
                                }

                                // Channel Volume Slider Row
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Slider {
                                        id: chanSlider
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 24
                                        from: 0.0
                                        to: 1.0
                                        value: modelData.volume
                                        onMoved: backend.setChannelVolume(modelData.index, value)
                                    }

                                    Text {
                                        Layout.preferredWidth: 36
                                        horizontalAlignment: Text.AlignRight
                                        text: win.formatPercent(modelData.volume)
                                        color: modelData.active ? theme.accent : "#6c6c7a"
                                        font.pixelSize: 11
                                        font.bold: modelData.active
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Keyboard Shortcuts Modal Overlay (?)
        Rectangle {
            anchors.fill: parent
            visible: win.helpVisible
            color: "#cc08080a"
            z: 20

            MouseArea {
                anchors.fill: parent
                onClicked: win.helpVisible = false
            }

            Rectangle {
                width: Math.min(parent.width - 60, 520)
                height: helpCol.implicitHeight + 44
                anchors.centerIn: parent
                radius: theme.radius
                color: "#16161c"
                border.color: theme.accent
                border.width: 1

                MouseArea { anchors.fill: parent }

                ColumnLayout {
                    id: helpCol
                    anchors.fill: parent
                    anchors.margins: 22
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Keyboard Shortcuts")
                            color: "#f4f4f8"
                            font.pixelSize: 16
                            font.bold: true
                        }
                        Text {
                            text: "Esc / ?"
                            color: theme.accent
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    Repeater {
                        model: [
                            { key: "Space", desc: QT_TR_NOOP("Play / Pause active soundscape") },
                            { key: "1 - 9", desc: QT_TR_NOOP("Load preset mix 1 through 9") },
                            { key: "Shift+1 - 9", desc: QT_TR_NOOP("Save current mix into preset slot") },
                            { key: "S", desc: QT_TR_NOOP("Save current mix to preset") },
                            { key: "W", desc: QT_TR_NOOP("Toggle organic wave modulation") },
                            { key: "T", desc: QT_TR_NOOP("Cycle sleep timer (15m - 120m)") },
                            { key: "M", desc: QT_TR_NOOP("Mute / unmute master audio") },
                            { key: "Up / Down", desc: QT_TR_NOOP("Adjust master volume") },
                            { key: "C", desc: QT_TR_NOOP("Shuffle random ambient mix") },
                            { key: "R / 0", desc: QT_TR_NOOP("Reset and clear all channels") },
                            { key: "?", desc: QT_TR_NOOP("Show or hide keyboard shortcuts") },
                            { key: "Q", desc: QT_TR_NOOP("Quit Murmur") }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Rectangle {
                                Layout.preferredWidth: 110
                                height: 26
                                radius: Math.min(4, theme.radius)
                                color: "#23232c"
                                border.color: "#343442"
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.key
                                    color: theme.accent
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: qsTr(modelData.desc)
                                color: "#d4d4e0"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }
}
