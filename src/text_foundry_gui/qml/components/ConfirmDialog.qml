import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Dialog {
    id: control

    property string messageText: ""
    property string cancelText: "Cancel"
    property string confirmText: "Confirm"
    property url confirmIconSource: Icons.checkSvg
    property color confirmAccentColor: ColorPalette.selection

    signal confirmed()
    signal cancelled()

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(parent.width - 64, 460)
    modal: true
    dim: true
    standardButtons: Dialog.NoButton

    background: Rectangle {
        radius: General.radiusMedium
        color: ColorPalette.surface
        border.color: ColorPalette.border
    }

    contentItem: ColumnLayout {
        spacing: General.spacingMedium

        Label {
            Layout.fillWidth: true
            text: control.messageText
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
            }

            SvgToolButton {
                iconSource: Icons.closeSvg
                labelText: control.cancelText
                onClicked: {
                    control.cancelled()
                    control.close()
                }
            }

            SvgToolButton {
                iconSource: control.confirmIconSource
                labelText: control.confirmText
                accentColor: control.confirmAccentColor
                onClicked: {
                    control.close()
                    control.confirmed()
                }
            }
        }
    }
}
