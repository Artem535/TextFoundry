pragma Singleton
import QtQuick
import QtQuick.Window

Item {
    readonly property bool isWindows: Qt.platform.os === "windows"
    readonly property bool forceDarkTheme: isWindows

    function luminance(c) {
        return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
    }

    function contrastDelta(a, b) {
        return Math.abs(luminance(a) - luminance(b))
    }

    SystemPalette {
        id: systemPalette
        colorGroup: SystemPalette.Active
    }

    readonly property bool useSystemPalette: !forceDarkTheme
                                           && contrastDelta(systemPalette.window, systemPalette.windowText) >= 0.45
                                           && contrastDelta(systemPalette.base, systemPalette.text) >= 0.35
                                           && contrastDelta(systemPalette.highlight, systemPalette.highlightedText) >= 0.30

    readonly property bool useSystemAccent: contrastDelta(systemPalette.highlight, systemPalette.highlightedText) >= 0.30

    readonly property bool darkTheme: forceDarkTheme
                                      || (useSystemPalette ? luminance(systemPalette.window) < 0.5 : true)

    readonly property color fallbackPrimary: "#6f6bff"
    readonly property color fallbackPrimaryDark: "#5a56ea"
    readonly property color fallbackPrimaryLight: "#8a86ff"
    readonly property color fallbackOnPrimary: "#0f1020"

    readonly property color fallbackBackground: "#25262e"
    readonly property color fallbackOnBackground: "#f2f4ff"

    readonly property color fallbackSurface: "#30313b"
    readonly property color fallbackSurfaceAlt: "#3a3b46"
    readonly property color fallbackOnSurface: "#f2f4ff"
    readonly property color fallbackOnSurfaceMuted: "#b6b9c9"

    readonly property color fallbackBorder: "#434553"
    readonly property color fallbackBorderStrong: "#56586a"

    readonly property color fallbackButton: "#3a3b46"
    readonly property color fallbackOnButton: "#f2f4ff"

    readonly property color fallbackWarning: "#d9a441"
    readonly property color fallbackDanger: "#e56a6a"
    readonly property color fallbackFieldBackground: "#2a2b34"
    readonly property color fallbackPlaceholderText: "#8f93a5"

    readonly property color primary: (useSystemPalette || useSystemAccent)
                                     ? (darkTheme ? Qt.lighter(systemPalette.highlight, 1.15)
                                                  : Qt.darker(systemPalette.highlight, 1.05))
                                     : fallbackPrimary
    readonly property color primaryDark: (useSystemPalette || useSystemAccent)
                                         ? Qt.darker(systemPalette.highlight, 1.2)
                                         : fallbackPrimaryDark
    readonly property color primaryLight: (useSystemPalette || useSystemAccent)
                                          ? Qt.lighter(systemPalette.highlight, 1.15)
                                          : fallbackPrimaryLight
    readonly property color onPrimary: (useSystemPalette || useSystemAccent)
                                       ? systemPalette.highlightedText
                                       : fallbackOnPrimary

    readonly property color background: useSystemPalette ? systemPalette.window
                                                          : fallbackBackground
    readonly property color onBackground: useSystemPalette ? systemPalette.windowText
                                                            : fallbackOnBackground

    readonly property color surface: useSystemPalette
                                     ? (darkTheme ? Qt.lighter(systemPalette.base, 1.08)
                                                  : systemPalette.base)
                                     : fallbackSurface
    readonly property color surfaceAlt: useSystemPalette
                                        ? (darkTheme ? Qt.lighter(systemPalette.alternateBase, 1.05)
                                                     : systemPalette.alternateBase)
                                        : fallbackSurfaceAlt
    readonly property color onSurface: useSystemPalette
                                       ? (darkTheme ? Qt.lighter(systemPalette.windowText, 1.1)
                                                    : Qt.darker(systemPalette.windowText, 1.05))
                                       : fallbackOnSurface
    readonly property color onSurfaceMuted: useSystemPalette
                                            ? (darkTheme ? Qt.lighter(systemPalette.windowText, 0.72)
                                                         : Qt.lighter(systemPalette.windowText, 1.35))
                                            : fallbackOnSurfaceMuted

    readonly property color border: useSystemPalette
                                    ? (darkTheme ? Qt.lighter(systemPalette.mid, 1.05)
                                                 : systemPalette.midlight)
                                    : fallbackBorder
    readonly property color borderStrong: useSystemPalette
                                          ? (darkTheme ? Qt.lighter(systemPalette.midlight, 1.35)
                                                       : systemPalette.mid)
                                          : fallbackBorderStrong

    readonly property color button: useSystemPalette
                                    ? (darkTheme ? Qt.lighter(systemPalette.button, 1.18)
                                                 : systemPalette.button)
                                    : fallbackButton
    readonly property color onButton: useSystemPalette ? systemPalette.buttonText
                                                        : fallbackOnButton

    readonly property color selection: (useSystemPalette || useSystemAccent) ? systemPalette.highlight
                                                                              : primary
    readonly property color onSelection: (useSystemPalette || useSystemAccent) ? systemPalette.highlightedText
                                                                                : fallbackOnPrimary
    readonly property color warning: useSystemPalette ? (darkTheme ? "#d9a441" : "#9a5a00")
                                                       : fallbackWarning
    readonly property color danger: useSystemPalette ? (darkTheme ? "#e56a6a" : "#b42318")
                                                      : fallbackDanger

    readonly property color headerBackground: surface
    readonly property color footerBackground: surface
    readonly property color fieldBackground: useSystemPalette
                                            ? (darkTheme ? Qt.darker(surface, 1.18)
                                                         : Qt.lighter(surface, 1.02))
                                            : fallbackFieldBackground
    readonly property color fieldText: onSurface
    readonly property color placeholderText: useSystemPalette ? onSurfaceMuted
                                                              : fallbackPlaceholderText
    readonly property color textPrimary: useSystemPalette ? systemPalette.text
                                                           : onSurface
    readonly property color textSecondary: useSystemPalette
                                           ? (darkTheme ? Qt.lighter(systemPalette.text, 1.18)
                                                        : Qt.darker(systemPalette.mid, 1.05))
                                           : onSurfaceMuted
}
