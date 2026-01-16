// MobileRegisterFrame.qml
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

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        Rectangle {
            anchors.fill: parent
            visible: Theme.gradientOn && Theme.hasGradient
            opacity: 0.35
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

    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 12
            anchors.margins: 24
            anchors.bottomMargin: 28

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
                Label { text: "- 1 lowercase (a-z)";                  color: ruleColor(ruleLower);   font.pointSize: 9 }
                Label { text: "- 1 uppercase (A-Z)";                  color: ruleColor(ruleUpper);   font.pointSize: 9 }
                Label { text: "- 1 number (0-9)";                     color: ruleColor(ruleDigit);   font.pointSize: 9 }
                Label { text: "- 1 special character (!@#...)";       color: ruleColor(ruleSpecial); font.pointSize: 9 }
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
