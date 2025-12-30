// ChatShell.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: "#111111"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            HeaderBar {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
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
                    color: "#0f0f0f"

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
