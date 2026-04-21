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
    topPadding: compact ? 8 : 6
    bottomPadding: compact ? 8 : 6

    contentItem: RowLayout {
        width: parent ? parent.width : implicitWidth
        height: parent ? parent.height : implicitHeight
        spacing: General.spacingSmall
        layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight
        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined

        SvgIcon {
            source: control.iconSource
            color: control.enabled ? control.accentColor
                                   : Qt.alpha(control.accentColor, 0.45)
            iconWidth: 16
            iconHeight: 16
            Layout.alignment: compact ? Qt.AlignCenter : Qt.AlignVCenter
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

    ToolTip.visible: control.compact && control.hovered
                     && (control.toolTipText.length > 0 || control.labelText.length > 0)
    ToolTip.text: control.toolTipText.length > 0 ? control.toolTipText : control.labelText
    ToolTip.delay: 300
    ToolTip.timeout: 2000
}
