import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Page {
    id: root

    background: Rectangle {
        color: ColorPalette.background
    }

    RowLayout {
        anchors.fill: parent
        spacing: General.spacingLarge
        visible: !CompositionEditorVm.open

        Frame {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            background: Rectangle {
                radius: General.radiusMedium
                color: ColorPalette.surface
                border.color: ColorPalette.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: General.paddingMedium
                spacing: General.paddingSmall

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: General.spacingSmall

                    Label {
                        text: "Compositions"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: General.spacingSmall

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.addSvg
                            labelText: "New"
                            onClicked: CompositionEditorVm.openCreateEditor()
                        }

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.editSvg
                            labelText: "Edit"
                            enabled: CompositionsVm.selectedCompositionId.length > 0
                            onClicked: CompositionEditorVm.openEditor()
                        }

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.deprecateSvg
                            labelText: "Deprecate"
                            enabled: CompositionsVm.selectedCompositionId.length > 0
                            onClicked: CompositionsVm.deprecateSelected()
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.reloadSvg
                            labelText: "Reload"
                            onClicked: CompositionsVm.reload()
                        }
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: CompositionsVm.compositionIds
                    currentIndex: Math.max(0, CompositionsVm.compositionIds.indexOf(CompositionsVm.selectedCompositionId))

                    delegate: ItemDelegate {
                        required property string modelData
                        required property int index

                        width: ListView.view.width
                        text: modelData
                        highlighted: ListView.isCurrentItem
                        onClicked: {
                            ListView.view.currentIndex = index
                            CompositionsVm.selectComposition(modelData)
                        }
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            background: Rectangle {
                radius: General.radiusMedium
                color: ColorPalette.surface
                border.color: ColorPalette.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: General.paddingMedium
                spacing: General.spacingMedium

                Label {
                    text: "Details"
                    color: ColorPalette.primary
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: General.radiusSmall
                    color: ColorPalette.fieldBackground
                    border.color: ColorPalette.borderStrong

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: General.paddingMedium
                        spacing: General.spacingMedium

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 4
                            rowSpacing: General.spacingMedium
                            columnSpacing: General.spacingLarge

                            DetailField {
                                label: "Id"
                                value: CompositionsVm.selectedCompositionId
                            }

                            DetailField {
                                label: "Version"
                                value: CompositionsVm.selectedVersion
                            }

                            DetailField {
                                label: "State"
                                value: CompositionsVm.selectedState
                            }

                            DetailField {
                                label: "Fragments"
                                value: CompositionsVm.selectedFragmentCount
                            }
                        }

                        DetailField {
                            label: "Description"
                            value: CompositionsVm.selectedDescription
                            placeholder: "No description"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4

                            Label {
                                text: "Content"
                                font.bold: true
                            }

                            Frame {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                background: Rectangle {
                                    radius: General.radiusMedium
                                    color: ColorPalette.fieldBackground
                                    border.color: ColorPalette.border
                                }

                                ListView {
                                    anchors.fill: parent
                                    anchors.margins: General.paddingSmall
                                    clip: true
                                    spacing: General.spacingSmall
                                    model: CompositionsVm.selectedFragments

                                    delegate: Label {
                                        required property string modelData
                                        width: ListView.view.width
                                        text: modelData
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }

                        Label {
                            text: CompositionsVm.statusText
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            opacity: 0.72
                        }
                    }
                }
            }
        }
    }

    CompositionEditorWindow {
        anchors.fill: parent
        visible: CompositionEditorVm.open
    }

    Connections {
        target: CompositionEditorVm

        function onSaved() {
            CompositionEditorVm.closeEditor()
        }
    }
}
