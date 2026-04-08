import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Dialog {
    id: rewriteDialog
    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(parent.width - 64, 980)
    height: Math.min(parent.height - 64, 760)
    modal: true
    dim: true
    visible: CompositionBlockRewriteVm.open
    title: "Rewrite Blocks"
    standardButtons: Dialog.NoButton
    onClosed: CompositionBlockRewriteVm.closeDialog()

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
                    id: rewriteScroll
                    anchors.fill: parent
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    Column {
                        width: rewriteScroll.availableWidth
                        spacing: General.spacingMedium

                        DetailField {
                            width: parent.width
                            label: "Target Composition"
                            value: CompositionBlockRewriteVm.targetCompositionId
                        }

                        DetailField {
                            width: parent.width
                            label: "Base Version"
                            value: CompositionBlockRewriteVm.targetCompositionVersion
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "Instruction"
                                font.bold: true
                            }

                            TextArea {
                                width: parent.width
                                height: 180
                                text: CompositionBlockRewriteVm.instruction
                                placeholderText: "Describe how the existing blocks should change without changing composition structure."
                                wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
                                selectByMouse: true
                                leftPadding: General.paddingSmall
                                rightPadding: General.paddingSmall
                                topPadding: General.paddingSmall
                                bottomPadding: General.paddingSmall
                                onTextChanged: if (text !== CompositionBlockRewriteVm.instruction) CompositionBlockRewriteVm.instruction = text
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Preview"
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Label {
                        text: CompositionBlockRewriteVm.patchCount > 0
                              ? CompositionBlockRewriteVm.patchCount + " patches"
                              : ""
                        opacity: 0.72
                    }
                }

                CodePreview {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 420
                    text: CompositionBlockRewriteVm.previewText
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
                onClicked: rewriteDialog.close()
            }

            SvgToolButton {
                iconSource: Icons.aiAssistSvg
                labelText: CompositionBlockRewriteVm.previewing ? "Previewing..." : "Preview"
                enabled: !CompositionBlockRewriteVm.previewing
                         && !CompositionBlockRewriteVm.applying
                         && CompositionBlockRewriteVm.rewriteAvailable
                onClicked: CompositionBlockRewriteVm.preview()
            }

            SvgToolButton {
                iconSource: Icons.saveSvg
                labelText: CompositionBlockRewriteVm.applying ? "Applying..." : "Apply All"
                enabled: !CompositionBlockRewriteVm.previewing
                         && !CompositionBlockRewriteVm.applying
                         && CompositionBlockRewriteVm.patchCount > 0
                onClicked: CompositionBlockRewriteVm.applyAll()
            }
        }
    }
}
