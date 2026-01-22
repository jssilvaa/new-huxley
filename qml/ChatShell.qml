import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Item {
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        // background layers
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

        // root split layout
        RowLayout {
            anchors.fill: parent
            spacing: 0

            // left contacts pane
            ContactsFrame {
                Layout.preferredWidth: 340
                Layout.minimumWidth: 240
                Layout.fillHeight: true
            }

            // right chat pane
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // chat header only
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
