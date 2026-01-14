import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import chat 1.0

T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    horizontalPadding: padding + 2
    spacing: 6

    icon.width: 24
    icon.height: 24
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

    icon.color: buttonText

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font.pointSize: 12
        color: control.buttonText
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
