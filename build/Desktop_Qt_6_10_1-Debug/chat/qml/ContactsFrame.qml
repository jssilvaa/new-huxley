// ContactsFrame.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Rectangle {
    color: Theme.contactsPanel
    border.color: Theme.contactsBorder
    border.width: 1

    function formatTime(ts) {
        if (!ts || ts.length === 0) return ""

        const d = new Date(ts)
        if (isNaN(d.getTime())) return ""

        const now = new Date()
        if (d.toDateString() === now.toDateString())
            return Qt.formatTime(d, "hh:mm")

        const y = new Date(now)
        y.setDate(now.getDate() - 1)
        if (d.toDateString() === y.toDateString())
            return "Yesterday"

        return Qt.formatDate(d, "dd/MM/yyyy")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        anchors.margins: 10

        Label {
            text: "Chats"
            color: Theme.contactsText
            font.bold: true
        }

        TextField {
            id: search
            Layout.fillWidth: true
            placeholderText: "Search"
            onTextChanged: Controller.contactsProxy.filterText = text 
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: Controller.focusContacts
            activeFocusOnTab: true
            model: Controller.contactsProxy

            function syncCurrentIndex() {
                if (!Controller.hasPeer) {
                    currentIndex = -1
                    return
                }

                const idx = Controller.contactsProxy.rowForUser(Controller.currentPeer)
                if (idx >= 0 && idx !== currentIndex) {
                    currentIndex = idx
                }
            }

            Component.onCompleted: {
                if (Controller.focus) {
                    forceActiveFocus()
                }
                syncCurrentIndex()
            }

            Connections {
                target: Controller
                function onFocusContactsChanged() {
                    if (Controller.focusContacts) {
                        Qt.callLater(() => list.forceActiveFocus())
                    }
                }
                function onCurrentPeerChanged() {
                    list.syncCurrentIndex()
                }
            }

            Connections {
                target: Controller.contactsProxy
                function onLayoutChanged() {
                    list.syncCurrentIndex()
                }
                function onModelReset() {
                    list.syncCurrentIndex()
                }
                function onRowsMoved() {
                    list.syncCurrentIndex()
                }
                function onRowsInserted() {
                    list.syncCurrentIndex()
                }
            }

            Keys.onPressed: function(e) {
                switch (e.key) {
                case Qt.Key_Up:
                    currentIndex = Math.max(0, currentIndex - 1)
                    e.accepted = true
                    break

                case Qt.Key_Down:
                    currentIndex = Math.min(count - 1, currentIndex + 1)
                    e.accepted = true
                    break

                case Qt.Key_Return:
                case Qt.Key_Enter:
                    if (currentItem) {
                        Controller.selectPeer(currentItem.username)
                        Controller.focusContacts = false
                    }
                    e.accepted = true
                    break

                case Qt.Key_Escape:
                    currentIndex = -1
                    Controller.clearPeer()
                    e.accepted = true
                    break
                }
            }

            delegate: Rectangle {
                id: row
                width: ListView.view.width
                height: 56
                radius: Theme.radiusSm

                property bool selected: ListView.isCurrentItem
                property bool hovered: mouse.containsMouse
                property bool pressed: mouse.pressed

                color: selected ? Theme.contactsSurface : "transparent"

                // need to fix this later 
                border.color: selected ? Theme.accent : "transparent"
                border.width: selected ? 1 : 0

                scale: pressed ? 0.985 : 1.0
                Behavior on scale { NumberAnimation { duration: Theme.animFast } }
                Behavior on color { ColorAnimation { duration: Theme.animFast } }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true

                    onClicked: {
                        list.currentIndex = index
                        Controller.selectPeer(username)
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    // online dot
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: online ? Theme.accent : Theme.contactsMuted
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // main text column
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: username
                            color: Theme.contactsText
                            elide: Label.ElideRight
                            font.pointSize: 11
                            font.bold: selected
                        }

                        Label {
                            text: lastMessage
                            color: Theme.contactsMuted
                            font.pointSize: 9
                            elide: Label.ElideRight
                            visible: lastMessage && lastMessage.length > 0
                        }
                    }

                    // right side info: timestamp and badge
                    ColumnLayout {
                        spacing: 4
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 60

                        Label {
                            text: formatTime(lastTimestamp)
                            font.pointSize: 9
                            color: Theme.contactsMuted
                            horizontalAlignment: Text.AlignRight
                            Layout.fillWidth: true
                            visible: lastTimestamp && lastTimestamp.length > 0
                        }

                        Rectangle {
                            visible: unread > 0
                            width: unread < 10 ? 18 : 22
                            height: 18
                            radius: 9
                            color: Theme.accent
                            Layout.alignment: Qt.AlignRight

                            // animations
                            Behavior on width { NumberAnimation { duration: Theme.animFast } }
                            Behavior on opacity { NumberAnimation { duration: Theme.animFast } }

                            Label {
                                anchors.centerIn: parent
                                text: unread > 99 ? "99+" : unread
                                color: Theme.onAccent
                                font.pointSize: 9
                            }
                        }
                    }

                    // keeps right edge consistent, little guy > icon
                    Label {
                        text: "›"
                        color: Theme.contactsMuted
                        visible: hovered || selected
                    }
                }

                // pressed overlay
                Rectangle {
                    anchors.fill: parent
                    radius: row.radius
                    color: Theme.contactsBorder
                    opacity: pressed ? 0.10 : 0.0
                    Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
                }

                // hover overlay
                Rectangle {
                    anchors.fill: parent
                    radius: row.radius
                    color: Theme.contactsPanel2
                    opacity: hovered && !selected ? 0.25 : 0.0
                    border.color: Theme.contactsBorder
                    border.width: hovered && !selected ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
                }
            }
        }
    }
}
