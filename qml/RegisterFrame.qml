// RegisterFrame.qml (desktop rectangular)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Utils"
import chat 1.0

Item {
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

    Rectangle { anchors.fill: parent; color: Theme.bg }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 980)
        height: Math.min(parent.height - 80, 600)
        radius: 18
        color: Theme.surface
        border.color: Theme.border
        border.width: 1

        Rectangle {
            anchors.fill: parent
            radius: panel.radius
            visible: Theme.gradientOn && Theme.hasGradient
            opacity: 0.18
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.gradA }
                GradientStop { position: 1; color: Theme.gradB }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 26
            spacing: 26

            // LEFT: form
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 560
                spacing: 12

                Label {
                    text: qsTr("Create account")
                    color: Theme.text
                    font.pointSize: 18
                    font.weight: Font.DemiBold
                }

                Label {
                    text: qsTr("Pick a username and a strong password.")
                    color: Theme.muted
                    font.pointSize: 11
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

                Item { Layout.fillHeight: true }

                ConnectionSettings {
                    Layout.fillWidth: true
                    compact: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomButton {
                        Layout.fillWidth: true
                        text: "Register"
                        enabled: Controller.connected && user.text.length > 0 && passOk && matchOk
                        onClicked: tryRegister()
                    }

                    CustomButton {
                        Layout.preferredWidth: 200
                        text: "Back to login"
                        onClicked: Controller.showLogin()
                    }
                }
            }

            // RIGHT: rules block
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 320
                radius: 14
                color: Theme.panel
                border.color: Theme.border
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 8

                    Label { text: "Password rules"; color: Theme.text; font.bold: true; font.pointSize: 12 }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; opacity: 0.8 }

                    Label { text: "• 8–24 characters";                 color: ruleColor(ruleLen);     font.pointSize: 10 }
                    Label { text: "• 1 lowercase (a–z)";              color: ruleColor(ruleLower);   font.pointSize: 10 }
                    Label { text: "• 1 uppercase (A–Z)";              color: ruleColor(ruleUpper);   font.pointSize: 10 }
                    Label { text: "• 1 number (0–9)";                 color: ruleColor(ruleDigit);   font.pointSize: 10 }
                    Label { text: "• 1 special character (!@#…)";     color: ruleColor(ruleSpecial); font.pointSize: 10 }
                    Label {
                        text: "• passwords match"
                        color: ruleColor(matchOk)
                        font.pointSize: 10
                        visible: confirm.text.length > 0 || attempted
                    }

                    Item { Layout.fillHeight: true }

                    Label {
                        text: Controller.connected ? "Status: Online" : "Status: Offline"
                        color: Controller.connected ? Theme.accent : Theme.muted
                        font.pointSize: 10
                    }
                }
            }
        }
    }
}
