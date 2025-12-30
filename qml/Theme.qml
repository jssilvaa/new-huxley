// Theme.qml
pragma Singleton
import QtQuick
import Qt.labs.settings

QtObject {
    id: root

    // ---- persisted settings ----
    property bool dark: true
    property int accentIndex: 0

    Settings {
        id: store
        category: "ui"
        property bool dark: true
        property int accentIndex: 0
    }

    Component.onCompleted: {
        dark = store.dark
        accentIndex = store.accentIndex
    }

    function setDark(v) {
        dark = v
        store.dark = v
    }

    readonly property var accents: ["#4caf50", "#8e7dff", "#ff9800"]
    function setAccent(i) {
        accentIndex = Math.max(0, Math.min(i, accents.length - 1))
        store.accentIndex = accentIndex
    }

    // ---- tokens ----
    readonly property color accent: accents[accentIndex]
    readonly property color danger: dark ? "#ff5252" : "#d32f2f"

    readonly property color bg:      dark ? "#0f0f0f" : "#f5f5f5"
    readonly property color panel:   dark ? "#141414" : "#ffffff"
    readonly property color panel2:  dark ? "#161616" : "#fafafa"
    readonly property color surface: dark ? "#1b1b1b" : "#f0f0f0"
    readonly property color border:  dark ? "#222222" : "#e0e0e0"

    readonly property color text:    dark ? "#eaeaea" : "#1a1a1a"
    readonly property color muted:   dark ? "#9aa0a6" : "#777777"

    // chat bubbles
    readonly property color bubblePeer: dark ? "#1b1b1b" : "#ffffff"
    readonly property color bubbleOwn:  dark ? "#1f3b2d" : "#dff5e6"

    // metrics
    readonly property int radiusSm: 10
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16

    readonly property int animFast: 120
    readonly property int animMed:  180
}
