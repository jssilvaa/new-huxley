// Main.qml
import QtQuick
import QtQuick.Controls
import chat 1.0

Window {
    id: root
    width: 1200
    height: 800
    visible: true
    title: qsTr("Huxley Chat")

    // Desktop-first. Don’t overthink frameless right now.
    color: "#101010"

    Loader {
        id: rootLoader
        anchors.fill: parent
        sourceComponent: Controller.authenticated ? chatShell : loginShell
    }

    Component {
        id: loginShell
        LoginFrame {
            anchors.fill: parent
            visible: !Controller.authenticated
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.animMed } }    
         }
    }

    Component {
        id: chatShell
        ChatShell {
            anchors.fill: parent
            visible: Controller.authenticated
            opacity: visible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.animMed } }
         }
    }

    ToastHost {
        id: toasts
        parent: root.contentItem
        anchors.fill: parent

        Component.onCompleted: toasts.show("Toast System Online", false)
    }

    Connections {
        target: Controller
        function onToast(msg) {
            toasts.show(msg, false)
        }
        function onError(msg) {
            toasts.show(msg, true)
        }
    }

    // Keep logs centralized while building.
    Connections {
        target: Controller
        function onToast(msg) { console.log("[TOAST]", msg) }
        function onError(msg) { console.error("[ERROR]", msg) }
        function onConnectedChanged() { console.log("[STATE] connected =", Controller.connected) }
        function onAuthenticatedChanged() { console.log("[STATE] authenticated =", Controller.authenticated) }
    }

    // quit with ctrl w 
    Action {
        shortcut: "Ctrl+w"
        onTriggered: Qt.quit()
    }
}
