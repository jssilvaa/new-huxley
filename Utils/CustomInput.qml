import QtQuick
import QtQuick.Controls
import chat 1.0

TextField {
    id: control

    // set image & icons here
    property string iconSource: ""
    property bool showIcon: iconSource != ""
    property alias maximumLength: control.maximumLength
    property color fieldBg: Theme.panel2
    property color fieldText: Theme.onPanel2
    property color fieldMuted: Theme.mutedText(Theme.panel2)

    // textfield inherits text, echomode, placeholdertext, enabled, readonly
    // this is already exposed. no need to do it twice
    implicitHeight: 40
    font.pointSize: 12

    leftPadding: 16
    rightPadding: showIcon ? 44 : 16

    placeholderTextColor: fieldMuted
    color: fieldText
    selectionColor: Theme.accent
    selectedTextColor: Theme.onAccent

    background: Rectangle {
        anchors.fill: parent
        color: fieldBg
        border.color: Theme.border
        border.width: 1
        radius: 20
    }

    Image {
        visible: showIcon
        source: iconSource

        anchors {
            right: parent.right
            rightMargin: 15
            verticalCenter: parent.verticalCenter
        }
        width: 20
        height: 20
        fillMode: Image.PreserveAspectFit
    }
}
