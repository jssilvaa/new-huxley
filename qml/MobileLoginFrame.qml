// MobileLoginFrame.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../Utils"
import chat 1.0

Item {
    id: root
    anchors.fill: parent
    property bool controllerReady: Controller !== null

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
        anchors.centerIn: safeArea
        width: Math.min(safeArea.width, 420)
        height: Math.min(safeArea.height, 640)
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

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
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
                text: qsTr("Login")
                textPointSize: 16
                contentOffsetX: -24
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
}
