import QtQuick
import QtQuick.Controls
import TextFoundry

Frame {
    id: root

    property alias title: titleLabel.text

    padding: General.paddingLarge

    background: Rectangle {
        radius: General.radiusMedium
        color: ColorPalette.surface
        border.color: ColorPalette.border
        border.width: 1
    }

    Column {
        anchors.fill: parent
        spacing: General.paddingSmall

        Label {
            id: titleLabel
            color: ColorPalette.primary
            font.bold: true
            font.pixelSize: General.fontSmall
        }
    }
}
