import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Dialog {
    id: slicePromptDialog
    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(parent.width - 64, 1040)
    height: Math.min(parent.height - 64, 820)
    modal: true
    dim: true
    visible: BlockSliceVm.open
    title: BlockSliceVm.dialogTitle
    standardButtons: Dialog.NoButton
    onClosed: BlockSliceVm.closeDialog()

    background: Rectangle {
        radius: General.radiusMedium
        color: ColorPalette.surface
        border.color: ColorPalette.border
    }

    contentItem: ColumnLayout {
        spacing: General.spacingMedium

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: General.spacingMedium

            Frame {
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                background: Rectangle {
                    radius: General.radiusMedium
                    color: ColorPalette.fieldBackground
                    border.color: ColorPalette.border
                }

                ScrollView {
                    id: sliceScroll
                    anchors.fill: parent
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    Column {
                        width: sliceScroll.availableWidth
                        spacing: General.spacingMedium

                        DetailField {
                            width: parent.width
                            label: "Target Composition"
                            value: BlockSliceVm.targetCompositionId
                            visible: BlockSliceVm.updateMode
                        }

                        DetailField {
                            width: parent.width
                            label: "Base Version"
                            value: BlockSliceVm.targetCompositionVersion
                            visible: BlockSliceVm.updateMode
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "Namespace Prefix"
                                font.bold: true
                            }

                            TextField {
                                width: parent.width
                                text: BlockSliceVm.namespacePrefix
                                placeholderText: "team.prompt"
                                onTextEdited: BlockSliceVm.namespacePrefix = text
                            }
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "Language"
                                font.bold: true
                            }

                            TextField {
                                width: parent.width
                                text: BlockSliceVm.language
                                placeholderText: "en"
                                onTextEdited: BlockSliceVm.language = text
                            }
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "Revision Comment"
                                font.bold: true
                            }

                            TextArea {
                                width: parent.width
                                height: 84
                                text: BlockSliceVm.revisionComment
                                placeholderText: BlockSliceVm.updateMode
                                                 ? "What changed in this prompt update?"
                                                 : "Optional note for generated blocks/composition"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== BlockSliceVm.revisionComment) BlockSliceVm.revisionComment = text
                            }
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: BlockSliceVm.updateMode ? "Updated Prompt" : "Source Prompt"
                                font.bold: true
                            }

                            TextArea {
                                width: parent.width
                                height: 420
                                text: BlockSliceVm.sourcePromptText
                                placeholderText: BlockSliceVm.updateMode
                                                 ? "Paste or edit the updated prompt text"
                                                 : "Paste the full prompt you want to decompose"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== BlockSliceVm.sourcePromptText) BlockSliceVm.sourcePromptText = text
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: BlockSliceVm.updateMode ? "Updated Composition Preview" : "Composition Preview"
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: BlockSliceVm.compositionPreviewId
                            opacity: 0.72
                            visible: BlockSliceVm.compositionPreviewId.length > 0
                        }
                    }

                    CodePreview {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 180
                        text: BlockSliceVm.compositionPreviewText
                        definition: "Markdown"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Generated Blocks"
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: BlockSliceVm.generatedCount > 0
                              ? BlockSliceVm.generatedCount + " blocks"
                              : ""
                        opacity: 0.72
                    }
                }

                CodePreview {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 320
                    text: BlockSliceVm.generatedPreviewText
                    definition: "Markdown"
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: General.spacingSmall

            Item { Layout.fillWidth: true }

            SvgToolButton {
                iconSource: Icons.closeSvg
                labelText: "Cancel"
                onClicked: slicePromptDialog.close()
            }

            SvgToolButton {
                iconSource: Icons.aiAssistSvg
                labelText: BlockSliceVm.generating ? "Generating..." : "Generate"
                enabled: !BlockSliceVm.generating
                         && !BlockSliceVm.publishing
                         && BlockSliceVm.aiGenerationAvailable
                onClicked: BlockSliceVm.generate()
            }

            SvgToolButton {
                iconSource: Icons.saveSvg
                labelText: BlockSliceVm.publishing ? "Publishing..." : BlockSliceVm.publishButtonText
                enabled: !BlockSliceVm.generating
                         && !BlockSliceVm.publishing
                         && BlockSliceVm.generatedCount > 0
                onClicked: BlockSliceVm.publishAll()
            }
        }
    }

    Connections {
        target: BlockSliceVm

        function onPublished() {
            slicePromptDialog.close()
        }
    }
}
