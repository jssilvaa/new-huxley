// LoginFrame.qml (desktop rectangular)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Utils"
import chat 1.0

Item {
    anchors.fill: parent
    property bool controllerReady: Controller !== null

    // background (use Theme if you want)
    Rectangle {
        anchors.fill: parent
        color: Theme.bg
    }

    // centered auth panel
    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 980)
        height: Math.min(parent.height - 80, 560)
        radius: 18
        color: Theme.surface
        border.color: Theme.border
        border.width: 1

        // subtle decorations
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
                    text: qsTr("Welcome back")
                    color: Theme.text
                    font.pointSize: 18
                    font.weight: Font.DemiBold
                }

                Label {
                    text: qsTr("Sign in to access your account")
                    color: Theme.muted
                    font.pointSize: 11
                }

                Item { Layout.preferredHeight: 10 }

                CustomInput {
                    id: usernameInput
                    Layout.fillWidth: true
                    fieldText: Theme.text
                    fieldMuted: Theme.muted
                    placeholderText: qsTr("Username")
                    iconSource: "qrc:/qt/qml/chat/images/message-icon.png"
                }

                CustomInput {
                    id: passwordInput
                    Layout.fillWidth: true
                    fieldText: Theme.text
                    fieldMuted: Theme.muted
                    placeholderText: qsTr("Password")
                    echoMode: TextInput.Password
                    iconSource: "qrc:/qt/qml/chat/images/lock.png"
                    maximumLength: 24

                    Keys.onReturnPressed: function(event) {
                        if (controllerReady && Controller.connected) {
                            Controller.login(usernameInput.text, passwordInput.text)
                            event.accepted = true
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    CustomCheckBox {
                        text: qsTr("Remember me")
                        font.pointSize: 10
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: qsTr("Forgot password?")
                        color: Theme.muted
                        font.pointSize: 10
                    }
                }

                Item { Layout.fillHeight: true }

                ConnectionSettings {
                    Layout.fillWidth: true
                    compact: true
                }

                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("Login")
                    textPointSize: 13
                    enabled: controllerReady && Controller.connected
                    onClicked: Controller.login(usernameInput.text, passwordInput.text)
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("New here? <b>Create an account</b>")
                    font.pointSize: 10
                    color: Theme.text
                    textFormat: Text.RichText
                    opacity: 0.9

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Controller.showRegister()
                    }
                }
            }

            // RIGHT: “discord-like” info block
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 320
                radius: 14
                color: Theme.panel
                border.color: Theme.border
                border.width: 1

                Column {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10

                    Image {
                        width: 256; height: 128
                        fillMode: Image.PreserveAspectFit
                        source: "qrc:/qt/qml/chat/images/logo.png"
                        opacity: 0.95
                    }

                    Label {
                        text: qsTr("Huxley Chat")
                        color: Theme.text
                        font.pointSize: 16
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Fast, minimal, and encrypted.\nPick a contact and start talking.")
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                        font.pointSize: 10
                    }

                    Item { height: 8 }

                    Rectangle { width: parent.width; height: 1; color: Theme.border; opacity: 0.8 }

                    Label {
                        text: (controllerReady && Controller.connected)
                              ? qsTr("Status: Online")
                              : qsTr("Status: Offline")
                        color: (controllerReady && Controller.connected) ? Theme.accent : Theme.muted
                        font.pointSize: 10
                    }
                }
            }
        }
    }
}
