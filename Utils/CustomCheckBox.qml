import QtQuick
import QtQuick.Controls
import chat 1.0

CheckBox {
    id: control
    hoverEnabled: true

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        radius: 6

        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        color: control.checked ? Theme.accent : Theme.panel2
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.border

        Behavior on color { ColorAnimation { duration: Theme.animFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.animFast } }

        Item {
            anchors.centerIn: parent
            width: 12
            height: 12
            visible: control.checkState === Qt.Checked

            Rectangle {
                width: 3
                height: 7
                radius: 1
                color: Theme.onAccent
                anchors.centerIn: parent
                rotation: 45
                x: -2
                y: 2
            }

            Rectangle {
                width: 3
                height: 12
                radius: 1
                color: Theme.onAccent
                anchors.centerIn: parent
                rotation: -45
                x: 2
                y: -2
            }
        }

        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 16
            height: 3
            color: Theme.onPanel2
            visible: control.checkState === Qt.PartiallyChecked
        }
    }

    contentItem: Label {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: Theme.text
    }
}
