// MobileRegisterFrame.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../Utils"
import chat 1.0

Item {
    id: root
    anchors.fill: parent

    property bool attempted: false

    function hasLower(s)   { return /[a-z]/.test(s) }
    function hasUpper(s)   { return /[A-Z]/.test(s) }
    function hasDigit(s)   { return /\d/.test(s) }
    function hasSpecial(s) { return /[^A-Za-z0-9]/.test(s) }

    property bool ruleLen:     pass.text.length >= 8 && pass.text.length <= 24
    property bool ruleLower:   hasLower(pass.text)
    property bool ruleUpper:   hasUpper(pass.text)
    property bool ruleDigit:   hasDigit(pass.text)
    property bool ruleSpecial: hasSpecial(pass.text)

    property bool passOk: ruleLen && ruleLower && ruleUpper && ruleDigit && ruleSpecial
    property bool matchOk: (confirm.text.length > 0) && (pass.text === confirm.text)

    function ruleColor(ok) {
        if (ok) return "#2e7d32"
        return attempted ? "#c62828" : Theme.muted
    }

    function tryRegister() {
        attempted = true
        if (!Controller.connected) return
        if (user.text.length === 0) return
        if (!passOk) return
        if (!matchOk) return
        Controller.registerUser(user.text, pass.text)
    }

    readonly property rect availableGeom: {
        if (Screen.availableGeometry && Screen.availableGeometry.width > 0)
            return Screen.availableGeometry
        const w = Screen.width > 0 ? Screen.width : root.width
        const h = Screen.height > 0 ? Screen.height : root.height
        return Qt.rect(0, 0, w, h)
    }
    readonly property int safeTop: Math.max(0, availableGeom.y)
    readonly property int safeLeft: Math.max(0, availableGeom.x)
    readonly property int safeRight: Math.max(0, Screen.width - (availableGeom.x + availableGeom.width))
    readonly property int safeBottom: Math.max(0, Screen.height - (availableGeom.y + availableGeom.height))

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        Rectangle {
            anchors.fill: parent
            visible: Theme.gradientOn && Theme.hasGradient
            opacity: Theme.gradientOpacity
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.gradA }
                GradientStop { position: 1; color: Theme.gradB }
            }
        }

        Image {
            anchors.fill: parent
            visible: Theme.decorationsOn && Theme.hasPattern
            source: Theme.pattern
            fillMode: Image.Tile
            opacity: 0.10
        }
    }

    Item {
        id: safeArea
        anchors.fill: parent
        anchors.topMargin: safeTop + 12
        anchors.bottomMargin: safeBottom + 12
        anchors.leftMargin: Math.max(16, safeLeft + 12)
        anchors.rightMargin: Math.max(16, safeRight + 12)
    }

    Rectangle {
        id: panel
        anchors.fill: safeArea
        radius: 24
        color: Theme.surface
        border.color: Theme.border
        border.width: 1
        clip: true

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            opacity: 0.08
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ffffff" }
                GradientStop { position: 1.0; color: "#000000" }
            }
        }

        ScrollView {
            id: scroll
            anchors.fill: parent
            clip: true
            ScrollBar.vertical.policy: Qt.platform.os === "android"
                ? ScrollBar.AlwaysOff
                : ScrollBar.AsNeeded

            ColumnLayout {
                width: scroll.width
                spacing: 12
                anchors.margins: 20
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top

                Image {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/qt/qml/chat/images/logo.png"
                }

                Label {
                    text: qsTr("Create account")
                    color: Theme.text
                    font.pointSize: 16
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignCenter
                }

                Label {
                    text: qsTr("Insert your details to register")
                    color: Theme.muted
                    font.pointSize: 11
                    Layout.alignment: Qt.AlignCenter
                }

                Item { Layout.preferredHeight: 10 }

                CustomInput {
                    id: user
                    Layout.fillWidth: true
                    fieldText: Theme.text
                    fieldMuted: Theme.muted
                    placeholderText: "Username"
                    iconSource: "qrc:/qt/qml/chat/images/message-icon.png"
                }

                CustomInput {
                    id: pass
                    Layout.fillWidth: true
                    fieldText: Theme.text
                    fieldMuted: Theme.muted
                    placeholderText: "Password"
                    echoMode: TextInput.Password
                    iconSource: "qrc:/qt/qml/chat/images/lock.png"
                    maximumLength: 24
                }

                CustomInput {
                    id: confirm
                    Layout.fillWidth: true
                    fieldText: Theme.text
                    fieldMuted: Theme.muted
                    placeholderText: "Confirm password"
                    echoMode: TextInput.Password
                    iconSource: "qrc:/qt/qml/chat/images/lock.png"
                    maximumLength: 24

                    Keys.onReturnPressed: function(event) {
                        tryRegister()
                        event.accepted = true
                    }
                    Keys.onEnterPressed: function(event) {
                        tryRegister()
                        event.accepted = true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label { text: "Password requirements:"; color: Theme.muted; font.pointSize: 9 }
                    Label { text: "- at least 8 characters";             color: ruleColor(ruleLen);     font.pointSize: 9 }
                    Label { text: "- 1 lowercase (a-z)";                 color: ruleColor(ruleLower);   font.pointSize: 9 }
                    Label { text: "- 1 uppercase (A-Z)";                 color: ruleColor(ruleUpper);   font.pointSize: 9 }
                    Label { text: "- 1 number (0-9)";                    color: ruleColor(ruleDigit);   font.pointSize: 9 }
                    Label { text: "- 1 special character (!@#...)";      color: ruleColor(ruleSpecial); font.pointSize: 9 }
                    Label {
                        text: "- passwords match"
                        color: ruleColor(matchOk)
                        font.pointSize: 9
                        visible: confirm.text.length > 0 || attempted
                    }
                }

                ConnectionSettings {
                    Layout.fillWidth: true
                    compact: true
                }

                CustomButton {
                    Layout.fillWidth: true
                    text: "Register"
                    enabled: Controller.connected && user.text.length > 0 && passOk && matchOk
                    onClicked: tryRegister()
                }

                CustomButton {
                    Layout.fillWidth: true
                    text: "<- Back to login"
                    onClicked: Controller.showLogin()
                }
            }
        }
    }
}
