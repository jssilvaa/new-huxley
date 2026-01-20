// InputBar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Utils"
import chat 1.0

Rectangle {
    color: Theme.panel2
    border.color: Theme.border
    border.width: 1

    property bool isAndroid: Qt.platform.os === "android"
    property bool showBar: Controller.authenticated && Controller.hasPeer

    visible: opacity > 0.01
    opacity: showBar ? 1.0 : 0.0
    enabled: showBar

    Behavior on opacity {
        NumberAnimation { duration: Theme.animSlow; easing.type: Easing.OutCubic }
    }

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
            if (!isAndroid && input.enabled)
                Qt.callLater(() => input.forceActiveFocus())
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        CustomInput {
            id: input
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            fieldBg: Theme.surface
            fieldText: Theme.text
            fieldMuted: Theme.muted

            placeholderText: !Controller.authenticated
                ? "Log in to start chatting"
                : !Controller.hasPeer
                    ? "Select a contact first"
                    : "Type a message"

            enabled: Controller.authenticated && Controller.hasPeer
        }

        CustomButton {
            id: sendButton
            text: "Send"
            icon.source: isAndroid ? "../images/message-icon.png" : ""
            display: isAndroid ? AbstractButton.IconOnly : AbstractButton.TextOnly
            Layout.preferredWidth: isAndroid ? 48 : implicitWidth
            Layout.preferredHeight: isAndroid ? 48 : implicitHeight
            Layout.alignment: Qt.AlignVCenter
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
