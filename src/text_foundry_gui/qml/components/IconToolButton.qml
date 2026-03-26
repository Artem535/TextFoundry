import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

ToolButton {
    id: control

    property string glyph: ""
    property bool regularIcon: false
    property string labelText: ""
    property bool compact: false

    text: labelText
    spacing: General.spacingSmall
    leftPadding: compact ? 8 : 10
    rightPadding: compact ? 8 : 10

    contentItem: RowLayout {
        spacing: General.spacingSmall

        AppIcon {
            glyph: control.glyph
            regular: control.regularIcon
            Layout.alignment: Qt.AlignVCenter
            color: control.enabled ? ColorPalette.onSurface
                                   : Qt.alpha(ColorPalette.onSurface, 0.45)
            font.pixelSize: General.fontBody
        }

        Label {
            visible: !control.compact
            text: control.labelText
            Layout.alignment: Qt.AlignVCenter
            color: control.enabled ? ColorPalette.onSurface
                                   : Qt.alpha(ColorPalette.onSurface, 0.45)
            font.pixelSize: General.fontBody
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}
