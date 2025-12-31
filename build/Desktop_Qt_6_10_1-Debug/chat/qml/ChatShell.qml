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
                    color: Theme.bg

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
