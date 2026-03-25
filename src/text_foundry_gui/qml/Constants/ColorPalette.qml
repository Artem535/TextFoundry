pragma Singleton
import QtQuick
import QtQuick.Window

Item {
    function luminance(c) {
        return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b
    }

    SystemPalette {
        id: systemPalette
        colorGroup: SystemPalette.Active
    }

    property bool darkTheme: luminance(systemPalette.window) < 0.5

    property color primary: darkTheme ? Qt.lighter(systemPalette.highlight, 1.15)
                                      : Qt.darker(systemPalette.highlight, 1.05)
    property color primaryDark: Qt.darker(systemPalette.highlight, 1.2)
    property color primaryLight: Qt.lighter(systemPalette.highlight, 1.15)
    property color onPrimary: systemPalette.highlightedText

    property color background: systemPalette.window
    property color onBackground: systemPalette.windowText

    property color surface: darkTheme ? Qt.lighter(systemPalette.base, 1.08) : systemPalette.base
    property color surfaceAlt: darkTheme ? Qt.lighter(systemPalette.alternateBase, 1.05) : systemPalette.alternateBase
    property color onSurface: darkTheme ? Qt.lighter(systemPalette.windowText, 1.1)
                                        : Qt.darker(systemPalette.windowText, 1.05)
    property color onSurfaceMuted: darkTheme ? Qt.lighter(systemPalette.windowText, 0.72)
                                             : Qt.lighter(systemPalette.windowText, 1.35)

    property color border: darkTheme ? Qt.lighter(systemPalette.mid, 1.05)
                                     : systemPalette.midlight
    property color borderStrong: darkTheme ? Qt.lighter(systemPalette.midlight, 1.35)
                                           : systemPalette.mid

    property color button: darkTheme ? Qt.lighter(systemPalette.button, 1.18)
                                     : systemPalette.button
    property color onButton: systemPalette.buttonText

    property color selection: systemPalette.highlight
    property color onSelection: systemPalette.highlightedText

    property color headerBackground: surface
    property color footerBackground: surface
    property color fieldBackground: darkTheme ? Qt.darker(surface, 1.18) : Qt.lighter(surface, 1.02)
    property color fieldText: onSurface
    property color placeholderText: onSurfaceMuted
    property color textPrimary: systemPalette.text
    property color textSecondary: darkTheme
                                  ? Qt.lighter(systemPalette.text, 1.18)
                                  : Qt.darker(systemPalette.mid, 1.05)
}
