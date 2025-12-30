// Main.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import "Utils"

Window {
    width: 400
    height: 700
    visible: true
    title: qsTr("Login App")

    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    OpacityAnimator {
        from: 0
        to: 1
        duration: 250
        running: true
        easing.type: Easing.InOutQuad
    }

    Rectangle {
        id: card
        anchors.fill: parent
        color: "#fafafa"
        radius: 40


        ColumnLayout {
            anchors {
                fill: parent
                margins: 20
                bottomMargin: 48
            }

            spacing: 0

            Image {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                source: "images/logo.png"
            }



            Item {
                Layout.preferredHeight: 32
            }

            Label {
                text: qsTr("Welcome back")

                color: "#1a1a1a"
                font {
                    pointSize: 15
                    weight: Font.DemiBold
                }
                Layout.alignment: Qt.AlignCenter
            }


            Label {
                text: qsTr("Sign in to access your account")

                color: "#777"
                font {
                    pointSize: 12
                    weight: Font.Light
                }
                Layout.alignment: Qt.AlignCenter
            }

            Item {
                Layout.preferredHeight: 32
            }

            // Username Field
            CustomInput {
                id: usernameInput
                Layout.fillWidth: true
                placeholderText: qsTr("Enter your username")
                iconSource: "qrc:/qt/qml/chat/images/message-icon.png"
            }


            Item {
                Layout.preferredHeight: 16
            }


            // Password field
            CustomInput {
                id: passwordInput
                Layout.fillWidth: true
                placeholderText: qsTr("Enter your password")
                echoMode: TextInput.Password
                iconSource: "qrc:/qt/qml/chat/images/lock.png"
            }

            Item {
                Layout.preferredHeight: 16
            }

            RowLayout {
                Layout.fillWidth: true

                CustomCheckBox {
                    font.pointSize: 10
                    text: qsTr("Remember me")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Forgot Password?")
                    font.pointSize: 9
                    color: "#000"
                }
            }

            Item {
                Layout.fillHeight: true
            }

            CustomButton {
                Layout.fillWidth: true
                text: qsTr("Login  →")
                enabled: Controller.connected
                onClicked: Controller.login(usernameInput.text, passwordInput.text)
            }

            Item {
                Layout.preferredHeight: 10
            }

            Label {
                Layout.alignment: Qt.AlignCenter
                text: qsTr("New member? <b>Register Now</b>")
                font {
                    pointSize: 8
                }
                color: "#1e1e1e"
                textFormat: Text.RichText
            }

            CustomButton {
                Layout.fillWidth: true
                text: qsTr("Users")
                enabled: Controller.authenticated
                onClicked: Controller.refreshUsers()
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 12
                color: "#f0f0f0"
                visible: Controller.authenticated

                ListView {
                    anchors.fill: parent
                    anchors.margins: 8

                    model: Controller.contacts
                    clip: true

                    delegate: RowLayout {
                        width: ListView.view.width
                        spacing: 8

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: online ? "#4caf50" : "#aaa"
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Label {
                            text: username
                            font.bold: online
                            color: "#1a1a1a"
                        }
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: Controller.chat

                    delegate: Row {
                        width: parent.width
                        spacing: 8

                        Text {
                            text: sender + ":"
                            font.bold: isOwn
                        }

                        Text {
                            text: content
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }

    Action {
        shortcut: "Ctrl+w"
        onTriggered: Qt.quit()
    }

    Connections {
        target: Controller

        function onToast(msg) {
            console.log("[TOAST]", msg)
        }

        function onError(msg) {
            console.error("[ERROR]", msg)
        }

        function onConnectedChanged() {
            console.log("[STATE] connected =", Controller.connected)
        }

        function onAuthenticatedChanged() {
            console.log("[STATE] authenticated =", Controller.authenticated)
        }
    }

    Connections {
        target: Controller.messageService

        function onUsersReceived(users) {
            console.log("[MESSAGE SERVICE] Users received:", users)
        }

        function onHistoryReceived(peer, history) {
            console.log("[MESSAGE SERVICE] History received for", peer, ":", history)
        }

        function onIncomingMessage(message) {
            console.log("[MESSAGE SERVICE] Incoming message:", message)
        }
    }
}
