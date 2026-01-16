import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Rectangle {
    id: root
    radius: Theme.radiusMd
    color: Theme.panel2
    border.color: Theme.border
    border.width: 1

    property string message: ""
    property bool error: false
    property int duration: 2400
    property int toastId: 0

    signal dismissRequested(int id)

    implicitHeight: Math.max(52, messageText.implicitHeight + 20)

    Timer {
        interval: root.duration
        running: true
        repeat: false
        onTriggered: root.dismissRequested(root.toastId)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Rectangle {
            width: 4
            radius: 2
            color: root.error ? Theme.danger : Theme.accent
            Layout.fillHeight: true
        }

        Label {
            id: messageText
            Layout.fillWidth: true
            text: root.message
            color: Theme.text
            wrapMode: Text.Wrap
        }

        ToolButton {
            flat: true
            onClicked: root.dismissRequested(root.toastId)

            contentItem: Label {
                text: "x"
                color: Theme.text
                font.pointSize: 11
                font.bold: true
            }
        }
    }
}
