// ChatView.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Rectangle {
    id: root
    color: Theme.bg

    // Empty states
    Column {
        anchors.centerIn: parent
        spacing: 8
        visible: !Controller.hasPeer

        Label {
            text: "No messages yet. Say hello 👋"
            color: Theme.muted
            horizontalAlignment: Text.AlignHCenter
        }
    }

    ListView {
        id: chat
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        spacing: 10
        model: Controller.chat

        boundsBehavior: Flickable.StopAtBounds

        // Keep glued to bottom only if user is at bottom.
        property bool stickToBottom: true
        function atBottom() {
            return contentY >= (contentHeight - height - 2)
        }

        onMovementStarted: stickToBottom = atBottom()
        onMovementEnded:   stickToBottom = atBottom()

        Component.onCompleted: Qt.callLater(positionViewAtEnd)

        onCountChanged: {
            if (count > 0 && stickToBottom)
                Qt.callLater(positionViewAtEnd)
        }

        // Correct place for “arrival” animations
        add: Transition {
            NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: Theme.animFast }
        }

        delegate: Item {
            id: row
            width: chat.width
            height: bubble.implicitHeight

            RowLayout {
                anchors.fill: parent

                Item { Layout.fillWidth: true; visible: isOwn }  // left spacer

                Rectangle {
                    id: bubble
                    radius: Theme.radiusSm
                    color: isOwn ? Theme.bubbleOwn : Theme.bubblePeer
                    border.color: Theme.border
                    border.width: 1

                    Layout.alignment: isOwn ? Qt.AlignRight : Qt.AlignLeft
                    Layout.maximumWidth: Math.floor(chat.width * 0.72)

                    implicitWidth: bubbleContent.implicitWidth + 24
                    implicitHeight: bubbleContent.implicitHeight + 18

                    ColumnLayout {
                        id: bubbleContent
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 10
                        spacing: 4

                        Label {
                            visible: !isOwn
                            text: sender
                            color: Theme.muted
                            font.pointSize: 9
                            elide: Label.ElideRight
                        }

                        Text {
                            text: content
                            color: Theme.text
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                            width: Math.min(implicitWidth, bubble.Layout.maximumWidth - 20)
                        }

                        Label {
                            visible: timestamp && timestamp.length > 0
                            text: timestamp
                            color: Theme.muted
                            font.pointSize: 8
                            horizontalAlignment: Text.AlignRight
                            Layout.fillWidth: true
                        }
                    }
                }

                Item { Layout.fillWidth: true; visible: !isOwn } // right spacer
            }
        }

        Connections {
            target: Controller
            function onCurrentPeerChanged() {
                chat.stickToBottom = true
                Qt.callLater(function() { chat.positionViewAtEnd(); })
            }
        }

        ScrollBar.vertical: ScrollBar { }
    }
}
