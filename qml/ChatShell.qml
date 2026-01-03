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

        // put these near the top of the background Rectangle (behind everything)
        Rectangle {
            anchors.fill: parent
            visible: Theme.gradientOn && Theme.hasGradient
            opacity: 0.35
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

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            HeaderBar {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                onSettingsRequested: settingsDrawer.open()
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                ContactsFrame {
                    Layout.preferredWidth: 340
                    Layout.fillHeight: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "transparent"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        ChatView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
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
}
