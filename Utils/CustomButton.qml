import QtQuick
import QtQuick.Controls
import chat 1.0

Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    horizontalPadding: padding + 2
    spacing: 6

    property color buttonBg: {
        const base = control.enabled ? Theme.accent : Theme.border
        if (control.down) return Qt.darker(base, 1.15)
        if (control.hovered) return Qt.darker(base, 1.08)
        return base
    }
    property color buttonText: {
        if (!control.enabled) return Theme.muted
        if (control.flat && !control.down && !control.checked && !control.highlighted) return Theme.text
        return Theme.onAccent
    }

    contentItem: Row {
        spacing: control.spacing
        anchors.centerIn: parent

        Image {
            source: control.icon.source
            visible: source !== ""
            width: 20
            height: 20
            fillMode: Image.PreserveAspectFit
        }

        Label {
            text: control.text
            font.pointSize: 12
            color: control.buttonText
        }
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 40
        visible: !control.flat || control.down || control.checked || control.highlighted
        color: control.buttonBg
        border.color: control.visualFocus ? Theme.accent : Theme.border
        border.width: control.visualFocus ? 2 :
                      Qt.styleHints.accessibility.contrastPreference === Qt.HighContrast ? 1 : 0
        radius: 10
    }
}
