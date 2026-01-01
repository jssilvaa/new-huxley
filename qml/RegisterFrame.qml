// RegisterFrame.qml
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
        // before attempt: muted; after attempt: red if bad; always green if good
        if (ok) return "#2e7d32"
        return attempted ? "#c62828" : "#777"
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
        anchors.centerIn: parent
        width: 420
        height: 720
        radius: 24
        color: "#fafafa"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            anchors.bottomMargin: 48
            spacing: 0

            Image {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                fillMode: Image.PreserveAspectFit
                source: "qrc:/qt/qml/chat/images/logo.png"
            }

            Item { Layout.preferredHeight: 32 }

            Label {
                text: qsTr("Create account")
                color: "#1a1a1a"
                font.pointSize: 15
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignCenter
            }

            Label {
                text: qsTr("Insert your details to register")
                color: "#777"
                font.pointSize: 12
                font.weight: Font.Light
                Layout.alignment: Qt.AlignCenter
            }

            Item { Layout.preferredHeight: 32 }

            CustomInput {
                id: user
                Layout.fillWidth: true
                placeholderText: "Username"
                iconSource: "qrc:/qt/qml/chat/images/message-icon.png"
            }

            Item { Layout.preferredHeight: 16 }

            CustomInput {
                id: pass
                Layout.fillWidth: true
                placeholderText: "Password"
                echoMode: TextInput.Password
                iconSource: "qrc:/qt/qml/chat/images/lock.png"
                maximumLength: 24
            }

            Item { Layout.preferredHeight: 16 }


            CustomInput {
                id: confirm
                Layout.fillWidth: true
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

            Item { Layout.preferredHeight: 16 }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                spacing: 4

                Label { text: "Password requirements:"; color: "#777"; font.pointSize: 9 }

                Label { text: "• at least 8 characters";                 color: ruleColor(ruleLen);     font.pointSize: 9 }
                Label { text: "• 1 lowercase (a–z)";                      color: ruleColor(ruleLower);   font.pointSize: 9 }
                Label { text: "• 1 uppercase (A–Z)";                      color: ruleColor(ruleUpper);   font.pointSize: 9 }
                Label { text: "• 1 number (0–9)";                         color: ruleColor(ruleDigit);   font.pointSize: 9 }
                Label { text: "• 1 special character (!@#…)";             color: ruleColor(ruleSpecial); font.pointSize: 9 }

                Label {
                    text: "• passwords match"
                    color: ruleColor(matchOk)
                    font.pointSize: 9
                    visible: confirm.text.length > 0 || attempted
                }
            }

            Item { Layout.fillHeight: true }

            CustomButton {
                Layout.fillWidth: true
                text: "Register"
                enabled: Controller.connected && user.text.length > 0 && passOk && matchOk
                onClicked: tryRegister()
            }
            
            Item { Layout.preferredHeight: 10 }

            CustomButton {
                Layout.fillWidth: true
                text: "← Back to login"
                onClicked: Controller.showLogin()
            }
        }
    }
}
