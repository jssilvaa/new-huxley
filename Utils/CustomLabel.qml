import QtQuick
import QtQuick.Controls
import chat 1.0

Label {
    id: root

    // Background color this text sits on
    // REQUIRED for correctness
    property color background: Theme.surface

    // semantic role
    property string role: "primary" // "primary" | "muted" | "accent"

    function luminance(c) {
        // sRGB luminance
        return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
    }

    function contrastText(bg) {
        return luminance(bg) > 0.55 ? "#111111" : "#f5f5f5"
    }

    color: {
        const base = contrastText(background)

        if (role === "muted")
            return Qt.lighter(base, 1.4)
        if (role === "accent")
            return Theme.accent

        return base
    }
}
