import QtQuick
import QtQuick.Controls

TextField {
    id: control

    // set image & icons here
    property string iconSource: ""
    property bool showIcon: iconSource != ""
    property alias maximumLength: control.maximumLength

    // textfield inherits text, echomode, placeholdertext, enabled, readonly
    // this is already exposed. no need to do it twice
    implicitHeight: 40
    font.pointSize: 12

    leftPadding: 16
    rightPadding: showIcon ? 44 : 16

    placeholderTextColor: "#999"
    color: "#1e1e1e"

    background: Rectangle {
        anchors.fill: parent
        color: "#e7e7e7"
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

