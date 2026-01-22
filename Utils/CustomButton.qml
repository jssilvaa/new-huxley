import QtQuick
import QtQuick.Controls
import chat 1.0

Button {
    id: control

    property bool isAndroid: Qt.platform.os === "android"
    property bool iconOnly: control.display === AbstractButton.IconOnly
    readonly property bool hasIcon: control.icon.source !== "" && control.display !== AbstractButton.TextOnly
    readonly property bool hasText: control.text.length > 0 && control.display !== AbstractButton.IconOnly
    property bool useHaptics: isAndroid
    property int textPointSize: isAndroid ? 14 : 12
    property real contentOffsetX: 0

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: isAndroid ? (iconOnly ? 12 : 14) : (iconOnly ? 8 : 10)
    horizontalPadding: iconOnly ? padding : padding + (isAndroid ? 12 : 6)
    spacing: iconOnly ? 0 : (isAndroid ? 10 : 8)

    hoverEnabled: !isAndroid

    property color buttonBg: {
        const base = control.enabled ? Theme.accent : Theme.border
        if (control.down) return Qt.darker(base, 1.15)
        if (control.hovered) return Qt.darker(base, 1.08)
        return base
    }
    property color buttonGlow: Qt.lighter(buttonBg, isAndroid ? 1.18 : 1.12)
    property color buttonShade: Qt.darker(buttonBg, isAndroid ? 1.2 : 1.12)
    property color buttonText: {
        if (!control.enabled) return Theme.muted
        if (control.flat && !control.down && !control.checked && !control.highlighted) return Theme.text
        return Theme.onAccent
    }

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight

        Row {
            id: contentRow
            spacing: control.hasIcon && control.hasText ? control.spacing : 0
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: control.contentOffsetX

            Image {
                source: control.icon.source
                visible: control.hasIcon
                width: control.hasIcon
                    ? (control.icon.width > 0 ? control.icon.width : (isAndroid ? 22 : 20))
                    : 0
                height: control.hasIcon
                    ? (control.icon.height > 0 ? control.icon.height : (isAndroid ? 22 : 20))
                    : 0
                fillMode: Image.PreserveAspectFit
            }

            Label {
                text: control.text
                visible: control.hasText
                width: control.hasText ? implicitWidth : 0
                height: control.hasText ? implicitHeight : 0
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pointSize: control.textPointSize
                font.weight: Font.DemiBold
                font.letterSpacing: isAndroid ? 0.2 : 0.0
                color: control.buttonText
            }
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
        implicitHeight: isAndroid ? 52 : 42
        visible: isAndroid || !control.flat || control.down || control.checked || control.highlighted
        color: control.flat ? "transparent" : control.buttonBg
        border.color: control.visualFocus ? Theme.accent : Qt.darker(control.buttonBg, isAndroid ? 1.25 : 1.35)
        border.width: control.flat && !control.down && !control.checked && !control.highlighted ? 0 : (control.visualFocus ? 2 : 1)
        radius: iconOnly ? height / 2 : (isAndroid ? 20 : 12)
        clip: true

        Behavior on color { ColorAnimation { duration: Theme.animFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.animFast } }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            visible: !control.flat
            opacity: isAndroid ? 0.32 : 0.25
            gradient: Gradient {
                GradientStop { position: 0.0; color: control.buttonGlow }
                GradientStop { position: 1.0; color: control.buttonShade }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height * (isAndroid ? 0.45 : 0.4)
            radius: parent.radius
            visible: !control.flat
            color: "#ffffff"
            opacity: control.enabled ? (isAndroid ? 0.16 : 0.12) : 0.0
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
