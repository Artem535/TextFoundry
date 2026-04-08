import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.syntaxhighlighting
import TextFoundry

Item {
    id: editorRoot

    Rectangle {
        anchors.fill: parent
        color: ColorPalette.background
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: General.spacingMedium

        Frame {
            Layout.fillWidth: true
            padding: General.paddingMedium
            background: Rectangle {
                radius: General.radiusMedium
                color: ColorPalette.surface
                border.color: ColorPalette.border
            }

            RowLayout {
                anchors.fill: parent
                spacing: General.spacingSmall

                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: General.spacingSmall

                    Label {
                        text: BlockEditorVm.dialogTitle
                        font.bold: true
                        color: ColorPalette.primary
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Label {
                        text: BlockEditorVm.createMode
                              ? "new block"
                              : (BlockEditorVm.currentVersion.length > 0
                                 ? "base " + BlockEditorVm.currentVersion
                                 : "draft")
                        opacity: 0.72
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: General.spacingSmall

                    SvgToolButton {
                        iconSource: Icons.aiAssistSvg
                        labelText: BlockEditorVm.generating ? "Generating..." : "Generate"
                        compact: true
                        enabled: !BlockEditorVm.generating
                                 && !BlockEditorVm.saving
                                 && BlockEditorVm.aiGenerationAvailable
                        onClicked: BlockEditorVm.generate()
                    }

                    SvgToolButton {
                        iconSource: Icons.backSvg
                        labelText: "Back"
                        compact: true
                        onClicked: BlockEditorVm.closeEditor()
                    }

                    SvgToolButton {
                        iconSource: Icons.saveSvg
                        labelText: BlockEditorVm.saving ? "Saving..." : BlockEditorVm.saveButtonText
                        compact: true
                        enabled: !BlockEditorVm.saving && !BlockEditorVm.generating
                        onClicked: BlockEditorVm.save()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: General.spacingMedium

            Frame {
                Layout.preferredWidth: 360
                Layout.minimumWidth: 320
                Layout.maximumWidth: 440
                Layout.fillHeight: true
                background: Rectangle {
                    radius: General.radiusMedium
                    color: ColorPalette.fieldBackground
                    border.color: ColorPalette.border
                }

                ScrollView {
                    id: metadataScroll
                    anchors.fill: parent
                    anchors.margins: General.paddingMedium
                    clip: true

                    Column {
                        width: metadataScroll.availableWidth
                        spacing: General.spacingMedium

                        ColumnLayout {
                            width: parent.width
                            visible: BlockEditorVm.createMode
                            spacing: 4

                            Label {
                                text: "Id"
                                font.bold: true
                            }

                            TextField {
                                width: parent.width
                                text: BlockEditorVm.blockId
                                placeholderText: "namespace.block_id"
                                onTextEdited: BlockEditorVm.blockId = text
                            }
                        }

                        DetailField {
                            width: parent.width
                            label: "Id"
                            value: BlockEditorVm.blockId
                            visible: !BlockEditorVm.createMode
                        }

                        DetailField {
                            width: parent.width
                            label: "Base Version"
                            value: BlockEditorVm.currentVersion
                            visible: !BlockEditorVm.createMode
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
                                text: BlockEditorVm.revisionComment
                                placeholderText: BlockEditorVm.createMode
                                                 ? "Optional note for the first published version"
                                                 : "What changed in this new version?"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== BlockEditorVm.revisionComment) BlockEditorVm.revisionComment = text
                            }
                        }

                        GridLayout {
                            width: parent.width
                            columns: 2
                            rowSpacing: General.spacingMedium
                            columnSpacing: General.spacingMedium

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Type"
                                    font.bold: true
                                }

                                ComboBox {
                                    Layout.fillWidth: true
                                    model: BlockEditorVm.typeOptions
                                    currentIndex: Math.max(0, BlockEditorVm.typeOptions.indexOf(BlockEditorVm.type))
                                    onActivated: BlockEditorVm.type = currentText
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Version Bump"
                                    font.bold: true
                                }

                                ComboBox {
                                    Layout.fillWidth: true
                                    model: BlockEditorVm.bumpOptions
                                    currentIndex: Math.max(0, BlockEditorVm.bumpOptions.indexOf(BlockEditorVm.bumpMode))
                                    onActivated: BlockEditorVm.bumpMode = currentText
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Language"
                                    font.bold: true
                                }

                                TextField {
                                    Layout.fillWidth: true
                                    text: BlockEditorVm.language
                                    onTextEdited: BlockEditorVm.language = text
                                }
                            }
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "Description"
                                font.bold: true
                            }

                            TextArea {
                                width: parent.width
                                height: 84
                                text: BlockEditorVm.description
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== BlockEditorVm.description) BlockEditorVm.description = text
                            }
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "AI Prompt"
                                font.bold: true
                            }

                            TextArea {
                                width: parent.width
                                height: 144
                                text: BlockEditorVm.aiPromptText
                                placeholderText: "Describe the block you want to generate"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== BlockEditorVm.aiPromptText) BlockEditorVm.aiPromptText = text
                            }
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "Tags"
                                font.bold: true
                            }

                            TextArea {
                                width: parent.width
                                height: 120
                                text: BlockEditorVm.tagsText
                                placeholderText: "one tag per line"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== BlockEditorVm.tagsText) BlockEditorVm.tagsText = text
                            }
                        }

                        ColumnLayout {
                            width: parent.width
                            spacing: 4

                            Label {
                                text: "Defaults"
                                font.bold: true
                            }

                            TextArea {
                                width: parent.width
                                height: 120
                                text: BlockEditorVm.defaultsText
                                placeholderText: "key=value"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== BlockEditorVm.defaultsText) BlockEditorVm.defaultsText = text
                            }
                        }
                    }
                }
            }

            Frame {
                id: templateEditorFrame
                Layout.fillWidth: true
                Layout.fillHeight: true
                background: Rectangle {
                    radius: General.radiusMedium
                    color: ColorPalette.fieldBackground
                    border.color: ColorPalette.borderStrong
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: General.paddingMedium
                    spacing: General.spacingSmall

                    Label {
                        text: "Template Editor"
                        font.bold: true
                    }

                    ScrollView {
                        id: templateEditorScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        TextEdit {
                            id: templateEditor
                            width: templateEditorScroll.availableWidth
                            text: BlockEditorVm.templateText
                            wrapMode: TextEdit.Wrap
                            selectByMouse: true
                            font.family: General.monospaceFamily
                            font.pixelSize: SessionVm.previewFontSize
                            color: templateEditorFrame.palette.windowText
                            selectedTextColor: templateEditorFrame.palette.highlightedText
                            selectionColor: templateEditorFrame.palette.highlight
                            onTextChanged: if (text !== BlockEditorVm.templateText) BlockEditorVm.templateText = text
                        }
                    }

                    Label {
                        text: BlockEditorVm.statusText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        opacity: 0.72
                    }
                }
            }
        }
    }

    SyntaxHighlighter {
        textEdit: templateEditor
        definition: "Markdown"
        theme: Repository.defaultTheme(ColorPalette.darkTheme
                                       ? Repository.DarkTheme
                                       : Repository.LightTheme)
    }
}
