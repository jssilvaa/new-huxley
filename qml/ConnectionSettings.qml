// ConnectionSettings.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml
import chat 1.0

ColumnLayout {
    id: root
    property bool compact: false
    spacing: compact ? 6 : 8
    Layout.fillWidth: true

    function applyConnection() {
        if (hostField.text !== Controller.serverHost) {
            Controller.serverHost = hostField.text
        }

        const parsed = parseInt(portField.text, 10)
        if (!isNaN(parsed)) {
            Controller.serverPort = parsed
        }

        Controller.reconnect()
    }

    Label {
        text: "Connection"
        color: Theme.muted
        font.pointSize: compact ? 9 : 10
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        TextField {
            id: hostField
            Layout.fillWidth: true
            placeholderText: "Host"
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

            Binding {
                target: hostField
                property: "text"
                value: Controller.serverHost
                when: !hostField.activeFocus
            }

            onAccepted: root.applyConnection()
        }

        TextField {
            id: portField
            Layout.preferredWidth: compact ? 84 : 110
            placeholderText: "Port"
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 1; top: 65535 }

            Binding {
                target: portField
                property: "text"
                value: String(Controller.serverPort)
                when: !portField.activeFocus
            }

            onAccepted: root.applyConnection()
        }

        Button {
            text: Controller.connected ? "Reconnect" : "Connect"
            onClicked: root.applyConnection()
        }
    }

    Label {
        text: Controller.connected ? "Status: Online" : "Status: Offline"
        color: Controller.connected ? Theme.accent : Theme.muted
        font.pointSize: compact ? 9 : 10
    }
}
