import QtQuick
import QtQuick.Controls
import chat 1.0

Button {
    id: control

    property bool isAndroid: Qt.platform.os === "android"
    property bool iconOnly: control.display === AbstractButton.IconOnly
    property bool useHaptics: isAndroid

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: isAndroid ? (iconOnly ? 10 : 12) : (iconOnly ? 8 : 10)
    horizontalPadding: iconOnly ? padding : padding + (isAndroid ? 10 : 6)
    spacing: iconOnly ? 0 : (isAndroid ? 10 : 8)

    hoverEnabled: !isAndroid

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
            width: control.icon.width > 0 ? control.icon.width : (isAndroid ? 22 : 20)
            height: control.icon.height > 0 ? control.icon.height : (isAndroid ? 22 : 20)
            fillMode: Image.PreserveAspectFit
        }

        Label {
            text: control.text
            visible: control.display !== AbstractButton.IconOnly
            font.pointSize: isAndroid ? 13 : 12
            font.weight: Font.DemiBold
            color: control.buttonText
        }
    }

    onPressed: {
        if (useHaptics && Qt.vibrate)
            Qt.vibrate(8)
        if (isAndroid)
            rippleAnim.restart()
    }

    background: Rectangle {
        implicitWidth: iconOnly ? implicitHeight : 100
        implicitHeight: isAndroid ? 50 : 42
        visible: isAndroid || !control.flat || control.down || control.checked || control.highlighted
        color: control.flat ? "transparent" : control.buttonBg
        border.color: control.visualFocus ? Theme.accent : Qt.darker(control.buttonBg, 1.4)
        border.width: control.flat && !control.down && !control.checked && !control.highlighted ? 0 : (control.visualFocus ? 2 : 1)
        radius: iconOnly ? height / 2 : (isAndroid ? 16 : 12)
        clip: true

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
            id: ripple
            anchors.centerIn: parent
            width: parent.width
            height: parent.height
            radius: iconOnly ? width / 2 : parent.radius
            color: "#ffffff"
            opacity: 0
            scale: 0.6
            visible: control.isAndroid
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

    ParallelAnimation {
        id: rippleAnim
        PropertyAnimation {
            target: ripple
            property: "opacity"
            from: 0.18
            to: 0.0
            duration: Theme.animFast + 120
        }
        PropertyAnimation {
            target: ripple
            property: "scale"
            from: 0.6
            to: 1.1
            duration: Theme.animFast + 120
            easing.type: Easing.OutCubic
        }
    }
}
