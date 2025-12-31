// SettingsDrawer.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Drawer {
    id: drawer
    edge: Qt.RightEdge
    modal: true
    width: 320

    background: Rectangle {
        color: Theme.panel
        border.color: Theme.border
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        Label {
            text: "Settings"
            color: Theme.text
            font.bold: true
            font.pointSize: 13
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Dark mode"; color: Theme.text; Layout.fillWidth: true }
            Switch {
                checked: Theme.dark
                onToggled: Theme.setDark(checked)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Label { text: "Accent"; color: Theme.text; font.bold: true }

            RowLayout {
                spacing: 10
                Repeater {
                    model: Theme.accents.length
                    delegate: Rectangle {
                        width: 26; height: 26; radius: 13
                        color: Theme.accents[index]
                        border.width: Theme.accentIndex === index ? 3 : 1
                        border.color: Theme.accentIndex === index ? Theme.text : Theme.border

                        MouseArea {
                            anchors.fill: parent
                            onClicked: Theme.setAccent(index)
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Button {
            text: "Close"
            onClicked: drawer.close()
        }
    }
}
