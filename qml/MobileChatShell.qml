// MobileChatShell.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

FocusScope {
    id: root
    anchors.fill: parent
    focus: true
    Keys.priority: Keys.BeforeItem
    Component.onCompleted: root.forceActiveFocus()

    property int pageIndex: 0

    onPageIndexChanged: {
        root.forceActiveFocus()
    }

    Keys.onBackPressed: function(event) {
        if (pageIndex === 1) {
            pageIndex = 0
            event.accepted = true
        }
    }

    Keys.onReleased: function(event) {
        if (event.key === Qt.Key_Back && pageIndex === 1) {
            pageIndex = 0
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        Rectangle {
            anchors.fill: parent
            visible: Theme.gradientOn && Theme.hasGradient
            opacity: 0.35
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

            property real pageT: root.pageIndex
            Behavior on pageT {
                NumberAnimation { duration: Theme.animSlow; easing.type: Easing.OutCubic }
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
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: Theme.panel2
                        border.color: Theme.border
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Label {
                                text: "Huxley"
                                color: Theme.text
                                font.bold: true
                                elide: Label.ElideRight
                            }

                            Item { Layout.fillWidth: true }

                            ToolButton {
                                icon.source: "../images/settings-icon.png"
                                icon.width: 18
                                icon.height: 18
                                flat: true
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
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: Theme.panel2
                        border.color: Theme.border
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            ToolButton {
                                text: "<-"
                                flat: true
                                onClicked: root.pageIndex = 0
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: Controller.hasPeer ? Controller.currentPeer : "Chat"
                                    color: Theme.text
                                    font.bold: true
                                    elide: Label.ElideRight
                                }

                                RowLayout {
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

                            ToolButton {
                                icon.source: "../images/settings-icon.png"
                                icon.width: 18
                                icon.height: 18
                                flat: true
                                onClicked: settingsDrawer.open()
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
