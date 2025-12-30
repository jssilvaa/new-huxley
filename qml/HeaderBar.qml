// HeaderBar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#161616"
    border.color: "#222222"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            text: Controller.authenticated ? "Huxley" : "Huxley (guest)"
            color: "#eaeaea"
            font.bold: true
        }

        Item { Layout.fillWidth: true }

        Label {
            text: Controller.connected ? "● Connected" : "● Disconnected"
            color: Controller.connected ? "#4caf50" : "#ff5252"
            font.pointSize: 10
        }

        Button {
            text: "Refresh"
            enabled: Controller.authenticated
            onClicked: Controller.refreshUsers()
        }
    }
}
