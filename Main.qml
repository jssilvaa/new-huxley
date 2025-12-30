// Main.qml
import QtQuick
import QtQuick.Controls

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
        LoginFrame { }
    }

    Component {
        id: chatShell
        ChatShell { }
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
