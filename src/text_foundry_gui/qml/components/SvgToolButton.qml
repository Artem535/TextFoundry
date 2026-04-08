import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

ToolButton {
    id: control

    property url iconSource
    property string labelText: ""
    property string toolTipText: ""
    property bool compact: false
    property color accentColor: ColorPalette.selection

    text: labelText
    hoverEnabled: true
    spacing: General.spacingSmall
    leftPadding: compact ? 8 : 10
    rightPadding: compact ? 8 : 10

    contentItem: RowLayout {
        spacing: General.spacingSmall

        SvgIcon {
            source: control.iconSource
            color: control.enabled ? control.accentColor
                                   : Qt.alpha(control.accentColor, 0.45)
            iconWidth: 16
            iconHeight: 16
            Layout.alignment: Qt.AlignVCenter
        }

        Label {
            visible: !control.compact
            text: control.labelText
            Layout.alignment: Qt.AlignVCenter
            color: control.enabled ? control.accentColor
                                   : Qt.alpha(control.accentColor, 0.45)
            font.pixelSize: 15
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    ToolTip {
        parent: control
        visible: control.compact && control.hovered
                 && (control.toolTipText.length > 0 || control.labelText.length > 0)
        text: control.toolTipText.length > 0 ? control.toolTipText : control.labelText
        delay: 300
        timeout: 2000
    }
}
