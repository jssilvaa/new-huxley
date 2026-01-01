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

        Loader {
            id: titleLoader
            active: true
            sourceComponent: titleComponent
            opacity: 1

            Behavior on opacity {
                NumberAnimation { duration: Theme.animFast }
            }

            Connections {
                target: Controller
                function onCurrentPeerChanged() {
                    titleLoader.opacity = 0
                    Qt.callLater(() => titleLoader.opacity = 1)
                }
            }
        }

        Component {
            id: titleComponent
            Label {
                text: !Controller.authenticated ? "Huxley (guest)"
                      : Controller.hasPeer ? Controller.currentPeer
                      : "Huxley"
                color: Theme.text
                font.bold: true
                elide: Label.ElideRight
            }
        }

        Row {
            spacing: 6
            visible: Controller.hasPeer

            Rectangle {
                width: 8; height: 8; radius: 4
                color: Controller.currentPeerOnline ? Theme.accent : Theme.muted // stub
            }

            Label {
                text: Controller.currentPeerOnline ? "online" : "offline" // stub
                color: Theme.muted
                font.pointSize: 9
            }
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
            opacity: enabled ? 1.0 : 0.4
            onClicked: Controller.refreshUsers()
        }

        Button {
            text: "⚙"
            onClicked: settingsRequested()
        }
    }
}
