import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Page {
    background: Rectangle {
        color: ColorPalette.background
    }

    Frame {
        anchors.fill: parent
        background: Rectangle {
            radius: General.radiusMedium
            color: ColorPalette.surface
            border.color: ColorPalette.border
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: General.sectionMargin
            spacing: General.spacingMedium

            Label {
                text: "Settings"
                color: ColorPalette.primary
                font.bold: true
                font.pixelSize: General.fontMedium
            }

            TextField {
                Layout.fillWidth: true
                text: SessionVm.projectKey
                placeholderText: "Project key"
                onEditingFinished: SessionVm.projectKey = text
            }

            TextField {
                Layout.fillWidth: true
                text: SessionVm.dataPath
                placeholderText: "Data path"
                onEditingFinished: SessionVm.dataPath = text
            }

            CheckBox {
                text: "Strict mode"
                checked: SessionVm.strictMode
                onToggled: SessionVm.strictMode = checked
            }

            Item {
                Layout.fillHeight: true
            }

            Button {
                text: "Reload engine"
                onClicked: SessionVm.reload()
            }
        }
    }
}
