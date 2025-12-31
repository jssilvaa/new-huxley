// HeaderBar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Rectangle {
    signal settingsRequested()

    color: Theme.panel2
    border.color: Theme.border
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            text: Controller.authenticated ? "Huxley" : "Huxley (guest)"
            color: Theme.text
            font.bold: true
        }

        Item { Layout.fillWidth: true }

        Label {
            text: Controller.connected ? "● Connected" : "● Disconnected"
            color: Controller.connected ? Theme.accent : Theme.danger
            font.pointSize: 10

            Behavior on color {
                ColorAnimation { duration: Theme.animFast }
            }
        }

        Button {
            text: "Refresh"
            enabled: Controller.authenticated
            onClicked: Controller.refreshUsers()
        }

        Button {
            text: "⚙"
            onClicked: settingsRequested()
        }
    }
}
