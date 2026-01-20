// ChatShell.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Item {
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        // background layers (behind everything)
        Rectangle {
            anchors.fill: parent
            visible: Theme.gradientOn && Theme.hasGradient
            opacity: Theme.gradientOpacity
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.gradA }
                GradientStop { position: 1; color: Theme.gradB }
            }
        }

        Image {
            anchors.fill: parent
            visible: Theme.decorationsOn && Theme.hasPattern
            source: Theme.pattern
            fillMode: Image.Tile
            opacity: 0.10
        }

        SettingsDrawer { id: settingsDrawer; height: parent.height }

        // --- NEW ROOT LAYOUT: split left (contacts) / right (chat pane)
        RowLayout {
            anchors.fill: parent
            spacing: 0

            // LEFT: contacts full height
            ContactsFrame {
                Layout.preferredWidth: 340
                Layout.minimumWidth: 240
                Layout.fillHeight: true
            }

            // RIGHT: chat pane (header + chat + input)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Header only for chat pane now
                    HeaderBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        onSettingsRequested: settingsDrawer.open()
                    }

                    ChatView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 500
                    }

                    InputBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 64
                    }
                }
            }
        }
    }
}
