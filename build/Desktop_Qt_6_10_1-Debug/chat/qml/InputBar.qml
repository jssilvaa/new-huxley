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

    opacity: enabled ? 1.0 : 0.6
    Behavior on opacity { NumberAnimation { duration: Theme.animFast } }

    Keys.onReturnPressed: function(event) {
        if (event.modifiers & Qt.ShiftModifier) { // still not working, input bar does not extend?
            input.insert("\n")
            event.accepted = true
        } else if (enabled && input.text.length > 0) {
            Controller.sendMessage(input.text)
            event.accepted = true
        }
    }

    Keys.onEnterPressed: function(event) {
        Keys.onReturnPressed(event)
    }

    Connections {
        target: Controller
        function onCurrentPeerChanged() {
            Qt.callLater(() => input.forceActiveFocus())
        }
    }

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
            }

            Connections {
                target: Controller.messageService
                function onSendMessageResponse() {
                    input.clear()
                }
            }
        }
    }
}
