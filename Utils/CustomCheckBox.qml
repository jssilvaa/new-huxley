import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl
import chat 1.0

CheckBox {
    id: control

    // keep in sync with CheckDelegate.qml (shared CheckIndicator.qml was removed for performance reasons)
    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16

        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        color: control.checked ? Theme.accent : Theme.panel2
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.accent : Theme.border

        ColorImage {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            color: Theme.onAccent
            source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
            visible: control.checkState === Qt.Checked

            width: 18
            height: 18
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

    contentItem: CheckLabel {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: Theme.text
    }
}
