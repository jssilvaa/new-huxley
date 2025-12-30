// ChatView.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0f0f0f"

    // Empty state
    Label {
        anchors.centerIn: parent
        text: "Select a chat to start messaging"
        color: "#666"
        visible: Controller.hasPeer === false
    }

    ListView {
        id: chat
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        spacing: 10
        model: Controller.chat

        // keep the view glued to bottom when new messages arrive,
        // but don't fight the user if they scroll up
        property bool stickToBottom: true

        onMovementStarted: stickToBottom = (contentY >= (contentHeight - height - 2))
        onMovementEnded:   stickToBottom = (contentY >= (contentHeight - height - 2))

        Component.onCompleted: positionViewAtEnd()
        onCountChanged: {
            if (count > 0 && stickToBottom)
                positionViewAtEnd()
        }

        delegate: Item {
            id: row
            width: chat.width
            // Height is bubble height only. Spacing handled by ListView.spacing
            height: bubble.implicitHeight

            // Use a row container so right/left alignment is stable
            RowLayout {
                anchors.fill: parent

                Item { Layout.fillWidth: true; visible: isOwn }  // left spacer for own messages

                Rectangle {
                    id: bubble
                    radius: 10
                    color: isOwn ? "#1f3b2d" : "#1b1b1b"
                    border.color: "#2a2a2a"
                    border.width: 1

                    Layout.alignment: isOwn ? Qt.AlignRight : Qt.AlignLeft
                    // IMPORTANT: cap width, don't force it
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

                        // Optional: sender line for non-own messages (WhatsApp group style)
                        // Comment out if you want 1:1-only feel.
                        Label {
                            visible: !isOwn
                            text: sender
                            color: "#9aa0a6"
                            font.pointSize: 9
                            elide: Label.ElideRight
                        }

                        Text {
                            text: content
                            color: "#eaeaea"
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                            // ensure wrapping uses the bubble width
                            width: Math.min(implicitWidth, bubble.Layout.maximumWidth - 20)
                        }

                        // Optional timestamp (if you have it)
                        Label {
                            visible: timestamp && timestamp.length > 0
                            text: timestamp
                            color: "#8a8a8a"
                            font.pointSize: 8
                            horizontalAlignment: Text.AlignRight
                            Layout.fillWidth: true
                        }
                    }
                }

                Item { Layout.fillWidth: true; visible: !isOwn } // right spacer for peer messages
            }
        }

        Connections { 
            target: Controller 
            function onCurrentPeerChanged() { 
                chat.stickToBottom = true;
                Qt.callLater(function() { chat.positionViewAtEnd(); });
            }
        }
    }
}
