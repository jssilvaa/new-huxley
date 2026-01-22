import QtQuick
import QtQuick.Controls
import chat 1.0

Label {
    id: root

    // background color for contrast
    property color background: Theme.surface

    // semantic role
    property string role: "primary" // primary muted accent

    color: {
        if (role === "muted")
            return Theme.mutedText(background)
        if (role === "accent")
            return Theme.accent

        return Theme.contrastText(background)
    }
}
