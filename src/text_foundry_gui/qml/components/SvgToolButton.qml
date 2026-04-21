import QtQuick
import QtQuick.Controls
import TextFoundry

ToolButton {
    id: control

    property url iconSource
    property string labelText: ""
    property string toolTipText: ""
    property bool compact: false
    property color accentColor: ColorPalette.selection

    text: labelText
    display: compact ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
    hoverEnabled: true
    spacing: General.spacingSmall
    leftPadding: compact ? 8 : 10
    rightPadding: compact ? 8 : 10
    icon.width: 16
    icon.height: 16
    icon.source: control.iconSource
    icon.color: control.enabled ? control.accentColor
                                : Qt.alpha(control.accentColor, 0.45)
    font.pixelSize: 15

    ToolTip.visible: control.compact && control.hovered
                     && (control.toolTipText.length > 0 || control.labelText.length > 0)
    ToolTip.text: control.toolTipText.length > 0 ? control.toolTipText : control.labelText
    ToolTip.delay: 300
    ToolTip.timeout: 2000
}
