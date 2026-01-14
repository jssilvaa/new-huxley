// Main.qml
import QtQuick
import QtQuick.Controls
import chat 1.0

Window {
    id: root
    width: 1200
    height: 800
    minimumWidth: 850
    minimumHeight: 450
    visible: true
    title: qsTr("Huxley Chat")
    color: Theme.bg

    property bool controllerReady: Controller !== null
    property string page: (controllerReady && Controller.authenticated) ? "chat"
                       : (controllerReady && Controller.registering)   ? "register"
                       : "login"

    // intro only once (first time app shows)
    property real introT: 0
    Component.onCompleted: introAnim.start()

    NumberAnimation {
        id: introAnim
        target: root
        property: "introT"
        from: 0
        to: 1
        duration: Theme.reducedMotion ? 0 : Theme.animMed
        easing.type: Easing.OutCubic
    }

    // --- Pages kept alive so transitions work ---
    Item {
        id: pages
        anchors.fill: parent

        // LOGIN
        LoginFrame {
            id: loginPage
            anchors.fill: parent

            opacity: root.introT * (root.page === "login" ? 1 : 0)
            visible: opacity > 0.01
            enabled: root.page === "login"

            // a tiny “pop” / drift
            scale: root.page === "login" ? 1.0 : 0.985
            y:     root.page === "login" ? 0   : 10

            Behavior on opacity { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
            Behavior on scale   { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
            Behavior on y       { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
        }

        // REGISTER
        RegisterFrame {
            id: regPage
            anchors.fill: parent

            opacity: root.introT * (root.page === "register" ? 1 : 0)
            visible: opacity > 0.01
            enabled: root.page === "register"

            scale: root.page === "register" ? 1.0 : 0.985
            y:     root.page === "register" ? 0   : 10

            Behavior on opacity { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
            Behavior on scale   { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
            Behavior on y       { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
        }

        // CHAT (slide in slightly from right)
        ChatShell {
            id: chatPage
            anchors.fill: parent

            opacity: root.introT * (root.page === "chat" ? 1 : 0)
            visible: opacity > 0.01
            enabled: root.page === "chat"

            x: root.page === "chat" ? 0 : 18
            Behavior on opacity { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
            Behavior on x       { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
        }
    }

    // toasts unchanged…
    ToastHost {
        id: toasts
        parent: root.contentItem
        anchors.fill: parent
        Component.onCompleted: toasts.show("Toast System Online", false)
    }

    Connections {
        target: Controller
        function onToast(msg) { toasts.show(msg, false) }
        function onError(msg) { toasts.show(msg, true) }
    }

    Action { shortcut: "Ctrl+w"; onTriggered: Qt.quit() }
}
