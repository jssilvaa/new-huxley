// qml/Theme.qml
pragma Singleton
import QtQuick
import QtCore   // Settings (Qt 6.5+)

QtObject {
    id: root

    // ---- persisted settings ----
    property var store: Settings {
        category: "ui"
        property bool dark: true
        property int accentIndex: 0

        // theme system
        property string presetId: "classic"
        property bool gradientOn: true
        property bool decorationsOn: false
        property bool reducedMotion: false
    }

    // expose as readonly state (bindings read these)
    readonly property bool dark: store.dark
    readonly property int accentIndex: store.accentIndex
    readonly property string presetId: store.presetId
    readonly property bool gradientOn: store.gradientOn
    readonly property bool decorationsOn: store.decorationsOn
    readonly property bool reducedMotion: store.reducedMotion

    // ---- mutators ----
    function setDark(v) { store.dark = v }
    function setAccent(i) { store.accentIndex = Math.max(0, Math.min(i, accents.length - 1)) }
    function setPreset(id) { store.presetId = id }
    function setGradientOn(v) { store.gradientOn = v }
    function setDecorationsOn(v) { store.decorationsOn = v }
    function setReducedMotion(v) { store.reducedMotion = v }

    // ---- accents ----
    readonly property var accents: ["#4caf50", "#8e7dff", "#ff9800", "#00bcd4", "#e91e63"]
    readonly property color accent: accents[accentIndex]
    readonly property color danger: dark ? "#ff5252" : "#d32f2f"

    // ---- presets catalog, small simple & stylish ----
    // Each preset can override any token; anything missing falls back to Classic.
    readonly property var presets: ({
        classic: {
            name: "Classic",
            blurb: "Neutral, readable.",
            gradA: "#141c24",
            gradB: "#0f0f0f"
        },
        midnight: {
            name: "Midnight",
            blurb: "Cool deep blues.",
            // dark
            bgDark: "#0b1020",
            panelDark: "#101a2e",
            panel2Dark: "#121f36",
            surfaceDark: "#162544",
            borderDark: "#203257",
            bubbleOwnDark: "#1a3a5a",
            gradADark: "#1b2a4a",
            gradBDark: "#0b1020",

            // light
            bgLight: "#eef2ff",
            panelLight: "#ffffff",
            panel2Light: "#f7f8ff",
            surfaceLight: "#edf0ff",
            borderLight: "#d7dcf5",
            bubbleOwnLight: "#dbe8ff",
            bubblePeerLight: "#ffffff",
            gradALight: "#dfe6ff",
            gradBLight: "#ffffff"
        },
        synthwave: {
            name: "Synthwave",
            blurb: "Neon vibes.",
            // dark
            bgDark: "#0a0620",
            panelDark: "#120a2d",
            panel2Dark: "#160c36",
            surfaceDark: "#1c1046",
            borderDark: "#2b1a6a",
            bubbleOwnDark: "#2a1b6a",
            bubblePeerDark: "#140a2e",
            gradADark: "#2b1a6a",
            gradBDark: "#0a0620",

            // light
            bgLight: "#fbf7ff",
            panelLight: "#ffffff",
            panel2Light: "#f6efff",
            surfaceLight: "#efe5ff",
            borderLight: "#e0cffd",
            bubbleOwnLight: "#efe0ff",
            bubblePeerLight: "#ffffff",
            gradALight: "#f1e5ff",
            gradBLight: "#ffffff"
        },
        hearts: {
            name: "Hearts",
            blurb: "Unprofessional. Perfect.",
            bgDark: "#0f0b10",
            panelDark: "#171118",
            panel2Dark: "#1b141c",
            surfaceDark: "#231a24",
            borderDark: "#2c2230",
            bubbleOwnDark: "#3a1f2a",
            bubblePeerDark: "#1b141c",
            gradA: "#3a1f2a",
            gradB: "#0f0b10",
            pattern: "qrc:/qt/qml/chat/images/patterns/hearts.svg"
        }
    })

    // helper: resolve a key from preset or fallback
    function preset() {
        return presets[presetId] || presets.classic
    }
    function pick(key, fallbackValue) {
        const p = preset()
        return (p[key] !== undefined) ? p[key] : fallbackValue
    }

    // ---- tokens (fallback = Classic values) ----
    readonly property color bg:      dark ? pick("bgDark",      "#0f0f0f") : pick("bgLight",      "#f5f5f5")
    readonly property color panel:   dark ? pick("panelDark",   "#141414") : pick("panelLight",   "#ffffff")
    readonly property color panel2:  dark ? pick("panel2Dark",  "#161616") : pick("panel2Light",  "#fafafa")
    readonly property color surface: dark ? pick("surfaceDark", "#1b1b1b") : pick("surfaceLight", "#f0f0f0")
    readonly property color border:  dark ? pick("borderDark",  "#222222") : pick("borderLight",  "#e0e0e0")

    readonly property color text:  dark ? pick("textDark",  "#eaeaea") : pick("textLight",  "#1a1a1a")
    readonly property color muted: dark ? pick("mutedDark", "#9aa0a6") : pick("mutedLight", "#777777")

    readonly property color bubblePeer: dark ? pick("bubblePeerDark", "#1b1b1b") : pick("bubblePeerLight", "#ffffff")
    readonly property color bubbleOwn:  dark ? pick("bubbleOwnDark",  "#1f3b2d") : pick("bubbleOwnLight",  "#dff5e6")

    // gradient tokens
    readonly property bool hasGradient: preset().gradA !== undefined && preset().gradB !== undefined
    readonly property color gradA: dark ? pick("gradADark", pick("gradA", "#141c24"))
                                    : pick("gradALight", pick("gradA", "#eef2ff"))
    readonly property color gradB: dark ? pick("gradBDark", pick("gradB", "#0f0f0f"))
                                    : pick("gradBLight", pick("gradB", "#ffffff"))

    // decorations token
    readonly property url pattern: pick("pattern", "")
    readonly property bool hasPattern: pattern !== ""

    // metrics
    readonly property int radiusSm: 10
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16

    // animation policy
    readonly property int animFast: reducedMotion ? 0 : 120
    readonly property int animMed:  reducedMotion ? 0 : 180
}
