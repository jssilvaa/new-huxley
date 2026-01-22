// Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import chat 1.0

Window {
    id: root
    property bool isMobile: Qt.platform.os === "android"
    property var mobileChatShell: null

    width: isMobile ? Screen.width : 1200
    height: isMobile ? Screen.height : 800
    minimumWidth: isMobile ? 0 : 850
    minimumHeight: isMobile ? 0 : 450
    visibility: isMobile ? Window.FullScreen : Window.Windowed
    visible: true
    title: qsTr("Huxley Chat")
    color: Theme.bg

    property bool controllerReady: Controller !== null
    property string page: (controllerReady && Controller.authenticated) ? "chat"
                       : (controllerReady && Controller.registering)   ? "register"
                       : "login"
    function handleAndroidBack() {
        if (!isMobile) return false

        if (page === "register") {
            Controller.showLogin()
            return true
        }

        if (page === "chat" && mobileChatShell && mobileChatShell.pageIndex === 1) {
            mobileChatShell.pageIndex = 0
            return true
        }

        if (page === "chat") return true
        if (page === "login") return true

        return false
    }
    onClosing: function(close) {
        if (root.isMobile) {
            if (root.handleAndroidBack()) close.accepted = false
            return
        }

        // Desktop: stop network/timers before QML teardown completes.
        if (Controller && Controller.shutdown)
            Controller.shutdown()
    }

    FocusScope {
        anchors.fill: parent
        focus: true
        Keys.priority: Keys.BeforeItem
        Keys.onBackPressed: function(event) {
            event.accepted = root.handleAndroidBack()
        }
    }

    Shortcut {
        enabled: root.isMobile
        sequences: [StandardKey.Back]
        context: Qt.ApplicationShortcut
        onActivated: root.handleAndroidBack()
    }

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

    Component { id: desktopLoginComponent; LoginFrame { anchors.fill: parent } }
    Component { id: mobileLoginComponent; MobileLoginFrame { anchors.fill: parent } }
    Component { id: desktopRegisterComponent; RegisterFrame { anchors.fill: parent } }
    Component { id: mobileRegisterComponent; MobileRegisterFrame { anchors.fill: parent } }
    Component { id: desktopChatComponent; ChatShell { anchors.fill: parent } }
    Component { id: mobileChatComponent; MobileChatShell { anchors.fill: parent } }

    // --- Pages kept alive so transitions work ---
    Item {
        id: pages
        anchors.fill: parent

        // LOGIN
        Item {
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

            Loader {
                anchors.fill: parent
                sourceComponent: root.isMobile ? mobileLoginComponent : desktopLoginComponent
            }
        }

        // REGISTER
        Item {
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

            Loader {
                anchors.fill: parent
                sourceComponent: root.isMobile ? mobileRegisterComponent : desktopRegisterComponent
            }
        }

        // CHAT (slide in slightly from right)
        Item {
            id: chatPage
            anchors.fill: parent

            opacity: root.introT * (root.page === "chat" ? 1 : 0)
            visible: opacity > 0.01
            enabled: root.page === "chat"

            x: root.page === "chat" ? 0 : 18
            Behavior on opacity { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }
            Behavior on x       { NumberAnimation { duration: Theme.reducedMotion ? 0 : Theme.animMed; easing.type: Easing.OutCubic } }

            Loader {
                anchors.fill: parent
                sourceComponent: root.isMobile ? mobileChatComponent : desktopChatComponent
                onLoaded: {
                    root.mobileChatShell = root.isMobile ? item : null
                }
            }
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

    // Avoid quitting directly from a shortcut handler (can cause shutdown re-entrancy).
    // Close the window instead; the app will exit when the last window closes.
    Action { shortcut: "Ctrl+w"; onTriggered: Qt.callLater(() => root.close()) }
}
