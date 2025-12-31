// ContactsFrame.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Rectangle {
    color: Theme.panel
    border.color: Theme.border
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        anchors.margins: 10

        Label {
            text: "Chats"
            color: "#eaeaea"
            font.bold: true
        }

        TextField {
            id: search
            Layout.fillWidth: true
            placeholderText: "Search"
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: Controller.contacts

            delegate: Rectangle {
                id: row
                width: ListView.view.width
                height: 56
                radius: Theme.radiusSm

                property bool selected: ListView.isCurrentItem
                property bool hovered: mouse.containsMouse
                property bool pressed: mouse.pressed

                color: selected ? Theme.surface : "transparent"

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

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: online ? Theme.accent : Theme.muted
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Label {
                        text: username
                        color: Theme.text
                        elide: Label.ElideRight
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font.pointSize: 11
                        font.bold: selected
                    }

                    // keeps right edge consistent, little guy > icon
                    Label {
                        text: "›"
                        color: Theme.muted
                        visible: hovered || selected
                    }
                }

                // pressed overlay
                Rectangle {
                    anchors.fill: parent
                    radius: row.radius
                    color: Theme.border
                    opacity: pressed ? 0.10 : 0.0
                    Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
                }

                // hover overlay
                Rectangle {
                    anchors.fill: parent
                    radius: row.radius
                    color: Theme.panel2
                    opacity: hovered && !selected ? 0.25 : 0.0
                    border.color: Theme.border
                    border.width: hovered && !selected ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
                }
            }
        }
    }
}
