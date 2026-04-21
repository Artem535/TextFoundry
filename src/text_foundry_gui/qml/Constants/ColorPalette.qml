pragma Singleton
import QtQuick

Item {
    readonly property bool darkTheme: true

    readonly property color primary: "#6f6bff"
    readonly property color primaryDark: "#5a56ea"
    readonly property color primaryLight: "#8a86ff"
    readonly property color onPrimary: "#0f1020"

    readonly property color background: "#25262e"
    readonly property color onBackground: "#f2f4ff"

    readonly property color surface: "#30313b"
    readonly property color surfaceAlt: "#3a3b46"
    readonly property color onSurface: "#f2f4ff"
    readonly property color onSurfaceMuted: "#b6b9c9"

    readonly property color border: "#434553"
    readonly property color borderStrong: "#56586a"

    readonly property color button: "#3a3b46"
    readonly property color onButton: "#f2f4ff"

    readonly property color selection: primary
    readonly property color onSelection: "#0f1020"
    readonly property color warning: "#d9a441"
    readonly property color danger: "#e56a6a"

    readonly property color headerBackground: surface
    readonly property color footerBackground: surface
    readonly property color fieldBackground: "#2a2b34"
    readonly property color fieldText: onSurface
    readonly property color placeholderText: "#8f93a5"
    readonly property color textPrimary: onSurface
    readonly property color textSecondary: onSurfaceMuted
}
