// ContactsFrame.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#141414"
    border.color: "#222222"
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
                width: ListView.view.width
                height: 56
                color: ListView.isCurrentItem ? "#202020" : "transparent"

                // selector 
                Rectangle {
                    width: 3
                    height: parent.height
                    color: ListView.isCurrentItem ? "#4caf50" : "transparent"
                    anchors.left: parent.left
                    anchors.top: parent.top
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: online ? "#4caf50" : "#888"
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Label {
                        text: username
                        color: "#eaeaea"
                        elide: Label.ElideRight
                        Layout.fillWidth: true
                        font.bold: online
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        list.currentIndex = index
                        Controller.selectPeer(username)
                    }
                }

                visible: search.text.length === 0
                         || username.toLowerCase().indexOf(search.text.toLowerCase()) >= 0
            }
        }
    }
}
