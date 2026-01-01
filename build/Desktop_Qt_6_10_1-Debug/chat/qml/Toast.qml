import QtQuick
import QtQuick.Controls
import chat 1.0

Rectangle {
    id: root
    radius: Theme.radiusMd
    color: Theme.panel2
    border.color: Theme.border
    border.width: 1

    property string message: ""
    property bool error: false

    implicitWidth: Math.min(parent.width * 0.6, text.implicitWidth + 32)
    implicitHeight: text.implicitHeight + 20

    opacity: 0
    y: parent.height

    Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
    Behavior on y       { NumberAnimation { duration: Theme.animFast } }

    Text {
        id: text
        anchors.centerIn: parent
        text: root.message
        color: error ? Theme.danger : Theme.text
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }
}
