// LoginFrame.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Utils"
import chat 1.0

Item {
    anchors.fill: parent

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: 420
        height: 720
        radius: 24
        color: "#fafafa"

        // ease in and out animation on register and back
        // OpacityAnimator {
        //     target: card
        //     from: 0
        //     to: 1
        //     duration: 250
        //     running: true
        //     easing.type: Easing.InOutQuad
        // }

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
                text: qsTr("Welcome back")
                color: "#1a1a1a"
                font.pointSize: 15
                font.weight: Font.DemiBold
                Layout.alignment: Qt.AlignCenter
            }

            Label {
                text: qsTr("Sign in to access your account")
                color: "#777"
                font.pointSize: 12
                font.weight: Font.Light
                Layout.alignment: Qt.AlignCenter
            }

            Item { Layout.preferredHeight: 32 }

            CustomInput {
                id: usernameInput
                Layout.fillWidth: true
                placeholderText: qsTr("Enter your username")
                iconSource: "qrc:/qt/qml/chat/images/message-icon.png"
            }

            Item { Layout.preferredHeight: 16 }

            CustomInput {
                id: passwordInput
                Layout.fillWidth: true
                placeholderText: qsTr("Enter your password")
                echoMode: TextInput.Password
                iconSource: "qrc:/qt/qml/chat/images/lock.png"
                maximumLength: 24

                Keys.onReturnPressed: function(event) {
                    if (Controller.connected) {
                        Controller.login(usernameInput.text, passwordInput.text)
                        event.accepted = true
                    }
                }
            }

            Item { Layout.preferredHeight: 16 }

            RowLayout {
                Layout.fillWidth: true

                CustomCheckBox {
                    font.pointSize: 10
                    text: qsTr("Remember me")
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: qsTr("Forgot Password?")
                    font.pointSize: 9
                    color: "#000"
                }
            }

            Item { Layout.fillHeight: true }

            CustomButton {
                Layout.fillWidth: true
                text: qsTr("Login  →")
                enabled: Controller.connected
                onClicked: Controller.login(usernameInput.text, passwordInput.text)
            }

            Item { Layout.preferredHeight: 10 }

            Label {
                Layout.alignment: Qt.AlignCenter
                text: qsTr("New member? <b>Register Now</b>")
                font.pointSize: 8
                color: "#1e1e1e"
                textFormat: Text.RichText

                MouseArea {
                    anchors.fill: parent 
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Controller.showRegister()
                }
            }
        }
    }
}
