// MobileChatShell.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../Utils"
import chat 1.0

FocusScope {
    id: root
    anchors.fill: parent
    focus: true
    Keys.priority: Keys.BeforeItem
    Component.onCompleted: root.forceActiveFocus()

    property bool isAndroid: Qt.platform.os === "android"
    property int pageIndex: 0
    readonly property rect availableGeom: {
        if (Screen.availableGeometry && Screen.availableGeometry.width > 0)
            return Screen.availableGeometry
        const w = Screen.width > 0 ? Screen.width : root.width
        const h = Screen.height > 0 ? Screen.height : root.height
        return Qt.rect(0, 0, w, h)
    }
    readonly property int safeTop: Math.max(0, availableGeom.y)
    readonly property int safeLeft: Math.max(0, availableGeom.x)
    readonly property int safeRight: Math.max(0, Screen.width - (availableGeom.x + availableGeom.width))
    readonly property int safeBottom: Math.max(0, Screen.height - (availableGeom.y + availableGeom.height))
    readonly property int safePad: isAndroid ? 8 : 6
    readonly property rect keyboardRect: Qt.inputMethod.keyboardRectangle
    readonly property int keyboardInset: isAndroid && Qt.inputMethod.visible
        && keyboardRect.height > 0 && keyboardRect.y > 0
        ? Math.max(0, root.height - keyboardRect.y)
        : 0
    readonly property int contentTopInset: safeTop + safePad
    readonly property int contentBottomInset: safePad + safeBottom + keyboardInset
    readonly property int headerMargin: 10
    readonly property int headerButtonSize: 48
    readonly property int headerIconSize: 20

    onPageIndexChanged: {
        root.forceActiveFocus()
    }

    // Back handling is centralized in Main.qml.

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        Rectangle {
            anchors.fill: parent
            visible: Theme.gradientOn && Theme.hasGradient
            opacity: Theme.gradientOpacity
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.gradA }
                GradientStop { position: 1; color: Theme.gradB }
            }
        }

        Image {
            anchors.fill: parent
            visible: Theme.decorationsOn && Theme.hasPattern
            source: Theme.pattern
            fillMode: Image.Tile
            opacity: 0.10
        }

        SettingsDrawer {
            id: settingsDrawer
            height: parent.height
        }

        Item {
            id: pages
            anchors.fill: parent
            clip: true

            property real pageT: 0
            property bool dragging: false
            property real dragStartT: 0

            Binding {
                target: pages
                property: "pageT"
                value: root.pageIndex
                when: !pages.dragging
            }
            Behavior on pageT {
                enabled: !pages.dragging
                NumberAnimation { duration: Theme.animSlow; easing.type: Easing.OutCubic }
            }

            DragHandler {
                id: backSwipe
                target: null
                enabled: root.isAndroid && root.pageIndex === 1
                acceptedDevices: PointerDevice.TouchScreen
                xAxis.enabled: true
                yAxis.enabled: false

                onActiveChanged: {
                    if (active) {
                        pages.dragging = true
                        pages.dragStartT = pages.pageT
                    } else if (pages.dragging) {
                        root.pageIndex = pages.pageT < 0.6 ? 0 : 1
                        pages.dragging = false
                    }
                }
                onTranslationChanged: {
                    if (!active)
                        return
                    const next = Math.max(0, Math.min(1, pages.dragStartT - translation.x / pages.width))
                    pages.pageT = next
                }
            }

            Item {
                id: contactsPage
                width: parent.width
                height: parent.height
                x: -pages.pageT * width
                opacity: 1.0 - pages.pageT
                enabled: root.pageIndex === 0

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: contentTopInset
                    anchors.bottomMargin: contentBottomInset
                    anchors.leftMargin: safeLeft
                    anchors.rightMargin: safeRight
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: Theme.panel2
                        border.color: Theme.border
                        border.width: 1

                        Item {
                            anchors.fill: parent
                            anchors.margins: root.headerMargin

                            Item {
                                id: contactsHeaderSpacer
                                width: root.headerButtonSize
                                height: root.headerButtonSize
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Label {
                                text: "Huxley"
                                color: Theme.text
                                font.bold: true
                                elide: Label.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                anchors.left: contactsHeaderSpacer.right
                                anchors.right: contactsSettings.left
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            CustomButton {
                                id: contactsSettings
                                icon.source: "../images/settings-icon.png"
                                icon.width: root.headerIconSize
                                icon.height: root.headerIconSize
                                display: AbstractButton.IconOnly
                                flat: true
                                width: root.headerButtonSize
                                height: root.headerButtonSize
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked: settingsDrawer.open()
                            }
                        }
                    }

                    ContactsFrame {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        onContactActivated: root.pageIndex = 1
                    }
                }
            }

            Item {
                id: chatPage
                width: parent.width
                height: parent.height
                x: (1.0 - pages.pageT) * width
                opacity: pages.pageT
                enabled: root.pageIndex === 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: contentTopInset
                    anchors.bottomMargin: contentBottomInset
                    anchors.leftMargin: safeLeft
                    anchors.rightMargin: safeRight
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: Theme.panel2
                        border.color: Theme.border
                        border.width: 1

                        Item {
                            anchors.fill: parent
                            anchors.margins: root.headerMargin

                            CustomButton {
                                id: backButton
                                icon.source: "../images/back-icon.png"
                                icon.width: root.headerIconSize
                                icon.height: root.headerIconSize
                                display: AbstractButton.IconOnly
                                flat: true
                                width: root.headerButtonSize
                                height: root.headerButtonSize
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked: root.pageIndex = 0
                            }

                            CustomButton {
                                id: chatSettings
                                icon.source: "../images/settings-icon.png"
                                icon.width: root.headerIconSize
                                icon.height: root.headerIconSize
                                display: AbstractButton.IconOnly
                                flat: true
                                width: root.headerButtonSize
                                height: root.headerButtonSize
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked: settingsDrawer.open()
                            }

                            Item {
                                anchors.left: backButton.right
                                anchors.right: chatSettings.left
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                height: parent.height

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    width: parent.width
                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true
                                        text: Controller.hasPeer ? Controller.currentPeer : "Chat"
                                        color: Theme.text
                                        font.bold: true
                                        elide: Label.ElideRight
                                        horizontalAlignment: Text.AlignHCenter
                                    }

                                    RowLayout {
                                        Layout.alignment: Qt.AlignHCenter
                                        spacing: 6
                                        visible: Controller.hasPeer

                                        Rectangle {
                                            width: 8
                                            height: 8
                                            radius: 4
                                            color: Controller.currentPeerOnline ? Theme.accent : Theme.muted
                                        }

                                        Label {
                                            text: Controller.currentPeerOnline ? "online" : "offline"
                                            color: Theme.muted
                                            font.pointSize: 9
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ChatView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    InputBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 64
                    }
                }
            }
        }
    }

    Connections {
        target: Controller
        function onCurrentPeerChanged() {
            if (Controller.hasPeer) {
                root.pageIndex = 1
            } else {
                root.pageIndex = 0
            }
        }
    }
}
