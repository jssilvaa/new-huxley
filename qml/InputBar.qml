// InputBar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Rectangle {
    color: Theme.panel2
    border.color: Theme.border
    border.width: 1

    enabled: Controller.authenticated && Controller.hasPeer

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        TextField {
            id: input
            Layout.fillWidth: true

            placeholderText: !Controller.authenticated
                ? "Log in to start chatting"
                : !Controller.hasPeer
                    ? "Select a contact first"
                    : "Type a message"

            enabled: Controller.authenticated && Controller.hasPeer
        }

        Button {
            text: "Send"
            enabled: input.text.length > 0 && input.enabled

            opacity: enabled ? 1.0 : 0.4
            Behavior on opacity {
                NumberAnimation { duration: Theme.animFast }
            }

            onClicked: {
                Controller.sendMessage(input.text)
                input.clear()
            }
        }
    }
}
