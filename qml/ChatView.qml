// ChatView.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import chat 1.0

Rectangle {
    id: root
    color: "transparent"
    property int bottomInset: 0
    onBottomInsetChanged: chat.keepBottomVisible()

    Layout.minimumWidth: Qt.platform.os === "android" ? 0 : 500
    Layout.minimumHeight: Qt.platform.os === "android" ? 0 : 300

    function formatMessageTime(ts) {
        if (!ts || ts.length === 0) return ""

        const d = new Date(ts)
        if (isNaN(d.getTime())) return ts

        return Qt.formatTime(d, "hh:mm")
    }

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
        reuseItems: true
        cacheBuffer: Math.max(0, height * 2)
        section.property: "dayLabel"
        section.criteria: ViewSection.FullString
        section.labelPositioning: ViewSection.InlineLabels
        section.delegate: Item {
            width: chat.width
            height: dayPill.visible ? dayPill.implicitHeight + 6 : 0
            visible: dayPill.visible

            Rectangle {
                id: dayPill
                anchors.centerIn: parent
                radius: 999
                color: Theme.surface
                border.color: Theme.border
                border.width: 1
                visible: section && section.length > 0

                implicitWidth: dayLabelText.implicitWidth + 20
                implicitHeight: dayLabelText.implicitHeight + 6

                Label {
                    id: dayLabelText
                    anchors.centerIn: parent
                    text: section
                    color: Theme.muted
                    font.pointSize: 9
                }
            }
        }

        property bool ready: false
        property bool isResetting: false
        property bool pendingPositionEnd: false
        interactive: !isResetting
        visible: ready 
        opacity: ready ? 1.0 : 0.0
        Behavior on opacity {
            NumberAnimation {
                duration: chat.isResetting ? 0 : Theme.animMed + 120
                easing.type: Easing.OutCubic
            }
        }

        Connections {
            target: Controller
            function onCurrentPeerChanged() {
                chat.ready = false 
                chat.stickToBottom = true
                chat.isResetting = true
            }
        }

        // temp placeholder logic
        Connections {
            target: Controller 
            function onClearChat() {
                chat.isResetting = true
                chat.pendingPositionEnd = true
            }
            function onShowChat() {
                chat.ready = false
                Qt.callLater(() => {
                    chat.forceLayout()
                    if (Controller.chat && Controller.chat.refreshDaySeparators) {
                        Controller.chat.refreshDaySeparators()
                    }
                    if (chat.pendingPositionEnd) {
                        chat.positionViewAtEnd()
                        chat.pendingPositionEnd = false
                    }
                    chat.isResetting = false
                    chat.ready = true
                })
            }
        }

        Connections {
            target: Controller
            function onMessageSubmitted() {
                chat.stickToBottom = true
                if (chat.isResetting || !chat.ready) {
                    chat.pendingPositionEnd = true
                    return
                }
                Qt.callLater(() => chat.positionViewAtEnd())
            }
        }

        boundsBehavior: Flickable.StopAtBounds
        Behavior on contentY {
            enabled: chat.isResetting
            NumberAnimation {
                duration: Theme.reducedMotion ? 0 : Theme.animMed + 160
                easing.type: Easing.OutCubic
            }
        }

        // Keep glued to bottom only if user is at bottom.
        property bool stickToBottom: true
        function atBottom() {
            return contentY >= (contentHeight - height - 2)
        }

        onMovementStarted: stickToBottom = atBottom()
        onMovementEnded:   stickToBottom = atBottom()

        Component.onCompleted: {
            ready = false  
            Qt.callLater(function() {
                positionViewAtEnd()
                ready = true
            })    
        } 

        onCountChanged: {
            if (isResetting) return
            if (count > 0 && stickToBottom) {
                Qt.callLater(function() {
                    positionViewAtEnd()
                    ready = true 
                })
            } else if (count === 0) {
                ready = true 
            }
        }
        function keepBottomVisible() {
            if (isResetting || !ready || !stickToBottom) return
            if (moving || flicking || dragging) return
            Qt.callLater(() => positionViewAtEnd())
        }

        onHeightChanged: keepBottomVisible()
        onContentHeightChanged: keepBottomVisible()

        footer: Item {
            width: chat.width
            height: root.bottomInset
            visible: height > 0
        }


        header: Item {
            width: chat.width
            height: headerText.visible ? headerText.implicitHeight + 16 : 0
            visible: Controller.hasPeer

            Label {
                id: headerText
                anchors.centerIn: parent
                width: Math.min(chat.width * 0.8, 460)
                text: Controller.hasPeer
                      ? "This is the beggining of your conversation with " + Controller.currentPeer
                      : ""
                color: Theme.muted
                font.pointSize: 9
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                visible: Controller.hasPeer
            }
        }
        
        add: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: chat.isResetting ? 0 : Theme.animFast }
                NumberAnimation { properties: "y"; from: y + 6; to: y; duration: chat.isResetting ? 0 : Theme.animFast }
            }
        }

        remove: Transition {
            ParallelAnimation {
                NumberAnimation { properties: "opacity"; from: 1; to: 0; duration: chat.isResetting ? 0 : Theme.animFast }
                NumberAnimation { properties: "y"; from: y; to: y - 6; duration: chat.isResetting ? 0 : Theme.animFast }
            }
        }

        delegate: Item {
            id: row
            width: chat.width
            implicitHeight: contentColumn.implicitHeight
            height: implicitHeight

            Column {
                id: contentColumn
                width: parent.width
                spacing: 6

                RowLayout {
                    width: parent.width

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
                                text: formatMessageTime(timestamp)
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
        }

        ScrollBar.vertical: ScrollBar {
            policy: Qt.platform.os === "android" ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded
        }
    }
}
