// qml/SettingsDrawer.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Drawer {
    id: root
    width: 360
    edge: Qt.RightEdge
    modal: false
    interactive: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.panel
        border.color: Theme.border
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        Label {
            text: "Settings"
            font.bold: true
            color: Theme.text
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; opacity: 0.8 }

        // --- Appearance toggles ---
        CustomCheckBox {
            text: "Dark Mode"
            checked: Theme.dark
            onToggled: Theme.setDark(checked)
        }

        CustomCheckBox {
            text: "Gradient background"
            checked: Theme.gradientOn
            enabled: Theme.hasGradient
            opacity: enabled ? 1 : 0.5
            onToggled: Theme.setGradientOn(checked)
        }

        CustomCheckBox {
            text: "Decorations"
            checked: Theme.decorationsOn
            enabled: Theme.hasPattern
            opacity: enabled ? 1 : 0.5
            onToggled: Theme.setDecorationsOn(checked)
        }

        CustomCheckBox {
            text: "Reduced motion"
            checked: Theme.reducedMotion
            onToggled: Theme.setReducedMotion(checked)
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; opacity: 0.8 }

        // --- Presets ---
        Label { text: "Themes"; color: Theme.muted; font.pointSize: 10 }

        Flow {
            Layout.fillWidth: true
            spacing: 10

            // We can’t iterate Theme.presets (object) directly; list them explicitly here.
            // when adding more, make sure you don't forget to add them here too
            Repeater {
                model: [
                    { id: "classic",   name: "Classic",   a: "#141c24", b: "#0f0f0f" },
                    { id: "midnight",  name: "Midnight",  a: "#1b2a4a", b: "#0b1020" },
                    { id: "synthwave", name: "Synthwave", a: "#2b1a6a", b: "#0a0620" },
                    { id: "hearts",    name: "Hearts",    a: "#3a1f2a", b: "#0f0b10" }
                ]

                delegate: Rectangle {
                    width: (root.width - 14*2 - 10) / 2
                    height: 74
                    radius: Theme.radiusMd
                    border.width: (Theme.presetId === modelData.id) ? 2 : 1
                    border.color: (Theme.presetId === modelData.id) ? Theme.accent : Theme.border
                    color: Theme.surface

                    // preview gradient
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        opacity: 0.55
                        gradient: Gradient {
                            GradientStop { position: 0; color: modelData.a }
                            GradientStop { position: 1; color: modelData.b }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: Theme.setPreset(modelData.id)
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 10
                        spacing: 2

                        Label {
                            text: modelData.name
                            color: Theme.text
                            font.bold: true
                            elide: Label.ElideRight
                        }

                        Label {
                            text: (Theme.presetId === modelData.id) ? "Selected" : ""
                            color: Theme.muted
                            font.pointSize: 9
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; opacity: 0.8 }

        // --- Accent ---
        Label { text: "Accent"; color: Theme.muted; font.pointSize: 10 }

        RowLayout {
            spacing: 10
            Repeater {
                model: Theme.accents
                delegate: Rectangle {
                    width: 22; height: 22; radius: 11
                    color: modelData
                    border.width: (index === Theme.accentIndex) ? 2 : 1
                    border.color: (index === Theme.accentIndex) ? Theme.text : Theme.border

                    MouseArea {
                        anchors.fill: parent
                        onClicked: Theme.setAccent(index)
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
