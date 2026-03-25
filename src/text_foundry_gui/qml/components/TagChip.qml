import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Rectangle {
    id: root

    property string text: ""

    radius: General.radiusSmall
    color: ColorPalette.surfaceAlt
    border.color: ColorPalette.borderStrong
    implicitHeight: chipLabel.implicitHeight + General.paddingSmall
    implicitWidth: chipLabel.implicitWidth + General.paddingMedium

    Label {
        id: chipLabel
        anchors.centerIn: parent
        text: root.text
    }
}
