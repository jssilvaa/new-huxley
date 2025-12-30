// InputBar.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#161616"
    border.color: "#222222"
    border.width: 1

    enabled: Controller.authenticated && Controller.hasPeer

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        TextField {
            id: input
            Layout.fillWidth: true
            placeholderText: "Type a message"
            enabled: Controller.authenticated
        }

        Button {
            text: "Send"
            enabled: parent.enabled && input.text.length > 0
            onClicked: {
                Controller.sendMessage(input.text)
                input.clear()
            }
        }
    }
}
