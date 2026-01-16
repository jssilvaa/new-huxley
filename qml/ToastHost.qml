import QtQuick
import QtQuick.Controls
import chat 1.0

Item {
    id: host
    anchors.fill: parent
    z: 1000

    property bool isMobile: Qt.platform.os === "android"
    property int nextId: 1

    ListModel { id: toastModel }

    function show(msg, isError=false, duration=2400) {
        toastModel.append({
            toastId: nextId++,
            message: msg,
            error: isError,
            duration: duration
        })
    }

    function dismiss(toastId) {
        for (let i = 0; i < toastModel.count; i++) {
            if (toastModel.get(i).toastId === toastId) {
                toastModel.remove(i)
                return
            }
        }
    }

    ListView {
        id: list
        model: toastModel
        spacing: 10
        interactive: false
        clip: true
        width: Math.min(parent.width * 0.9, 420)
        height: Math.min(contentHeight, parent.height - 32)

        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.top: host.isMobile ? undefined : parent.top
        anchors.topMargin: host.isMobile ? 0 : 16
        anchors.bottom: host.isMobile ? parent.bottom : undefined
        anchors.bottomMargin: host.isMobile ? 20 : 0

        verticalLayoutDirection: host.isMobile ? ListView.BottomToTop : ListView.TopToBottom

        delegate: Toast {
            width: list.width
            message: model.message
            error: model.error
            duration: model.duration
            toastId: model.toastId
            onDismissRequested: host.dismiss(toastId)
        }

        add: Transition {
            NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: Theme.animMed; easing.type: Easing.OutCubic }
            NumberAnimation { properties: "y"; from: host.isMobile ? y - 10 : y + 10; to: y; duration: Theme.animMed; easing.type: Easing.OutCubic }
        }

        remove: Transition {
            NumberAnimation { properties: "opacity"; from: 1; to: 0; duration: Theme.animFast; easing.type: Easing.InCubic }
        }

        displaced: Transition {
            NumberAnimation { properties: "y"; duration: Theme.animFast; easing.type: Easing.OutCubic }
        }
    }
}
