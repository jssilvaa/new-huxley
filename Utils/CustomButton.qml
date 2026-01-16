import QtQuick
import QtQuick.Controls
import chat 1.0

Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 10
    horizontalPadding: padding + 6
    spacing: 8

    hoverEnabled: true

    property color buttonBg: {
        const base = control.enabled ? Theme.accent : Theme.border
        if (control.down) return Qt.darker(base, 1.15)
        if (control.hovered) return Qt.darker(base, 1.08)
        return base
    }
    property color buttonGlow: Qt.lighter(buttonBg, 1.12)
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
            visible: source !== "" && control.display !== AbstractButton.TextOnly
            width: 20
            height: 20
            fillMode: Image.PreserveAspectFit
        }

        Label {
            text: control.text
            visible: control.display !== AbstractButton.IconOnly
            font.pointSize: 12
            font.weight: Font.DemiBold
            color: control.buttonText
        }
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 42
        visible: !control.flat || control.down || control.checked || control.highlighted
        color: control.flat ? "transparent" : control.buttonBg
        border.color: control.visualFocus ? Theme.accent : Qt.darker(control.buttonBg, 1.4)
        border.width: control.visualFocus ? 2 : 1
        radius: 12

        Behavior on color { ColorAnimation { duration: Theme.animFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.animFast } }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            visible: !control.flat
            opacity: 0.25
            gradient: Gradient {
                GradientStop { position: 0.0; color: control.buttonGlow }
                GradientStop { position: 1.0; color: Qt.darker(control.buttonBg, 1.15) }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "#ffffff"
            opacity: control.hovered && !control.down ? 0.08 : 0.0
            Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "#000000"
            opacity: control.down ? 0.12 : 0.0
            Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
        }
    }
}
