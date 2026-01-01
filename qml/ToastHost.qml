import QtQuick
import chat 1.0

Item {
    id: host
    anchors.fill: parent
    z: 1000

    property var queue: []
    property bool showing: false

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
    }

    function show(msg, isError=false) {
        queue.push({ msg: msg, error: isError })
        if (!showing)
            next()
    }

    function next() {
        if (queue.length === 0) {
            showing = false
            return
        }

        showing = true
        const item = queue.shift()

        toast.message = item.msg
        toast.error   = item.error

        toast.opacity = 1
        toast.y = parent.height - toast.height - 24

        Qt.callLater(() => {
            Qt.createQmlObject(`
                import QtQuick
                Timer {
                    interval: 2000
                    running: true
                    repeat: false
                    onTriggered: {
                        toast.opacity = 0
                        toast.y = parent.height
                        Qt.callLater(host.next)
                        destroy()
                    }
                }`, host)
        })
    }
}
