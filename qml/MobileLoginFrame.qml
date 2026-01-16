// MobileLoginFrame.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Utils"
import chat 1.0

Item {
    anchors.fill: parent
    property bool controllerReady: Controller !== null

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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        anchors.bottomMargin: 28
        spacing: 12

        Image {
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            fillMode: Image.PreserveAspectFit
            source: "qrc:/qt/qml/chat/images/logo.png"
        }

        Label {
            text: qsTr("Welcome back")
            color: Theme.text
            font.pointSize: 16
            font.weight: Font.DemiBold
            Layout.alignment: Qt.AlignCenter
        }

        Label {
            text: qsTr("Sign in to access your account")
            color: Theme.muted
            font.pointSize: 11
            Layout.alignment: Qt.AlignCenter
        }

        Item { Layout.preferredHeight: 12 }

        CustomInput {
            id: usernameInput
            Layout.fillWidth: true
                    fieldText: Theme.text
                    fieldMuted: Theme.muted
            placeholderText: qsTr("Enter your username")
            iconSource: "qrc:/qt/qml/chat/images/message-icon.png"
        }

        CustomInput {
            id: passwordInput
            Layout.fillWidth: true
                    fieldText: Theme.text
                    fieldMuted: Theme.muted
            placeholderText: qsTr("Enter your password")
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

            CustomCheckBox {
                font.pointSize: 10
                text: qsTr("Remember me")
            }

            Item { Layout.fillWidth: true }

            Label {
                text: qsTr("Forgot Password?")
                font.pointSize: 9
                color: Theme.muted
            }
        }

        ConnectionSettings {
            Layout.fillWidth: true
            compact: true
        }

        Item { Layout.fillHeight: true }

        CustomButton {
            Layout.fillWidth: true
            text: qsTr("Login  ->")
            enabled: controllerReady && Controller.connected
            onClicked: Controller.login(usernameInput.text, passwordInput.text)
        }

        Label {
            Layout.alignment: Qt.AlignCenter
            text: qsTr("New member? <b>Register Now</b>")
            font.pointSize: 9
            color: Theme.text
            textFormat: Text.RichText

            MouseArea {
                anchors.fill: parent
                onClicked: Controller.showRegister()
            }
        }
    }
}
