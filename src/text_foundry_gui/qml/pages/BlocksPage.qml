import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.syntaxhighlighting
import TextFoundry

Page {
    background: Rectangle {
        color: ColorPalette.background
    }

    RowLayout {
        anchors.fill: parent
        spacing: General.spacingLarge

        Frame {
            Layout.preferredWidth: General.blocksTreeWidth
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

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Blocks"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "New"
                        onClicked: BlockEditorVm.openCreateEditor()
                    }

                    Button {
                        text: "Reload"
                        onClicked: BlocksModel.reload()
                    }
                }

                TreeView {
                    id: blocksTree
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: BlocksModel
                    selectionModel: ItemSelectionModel {}
                    columnWidthProvider: function(column) {
                        return column === 0 ? blocksTree.width : 0
                    }
                    palette.highlight: ColorPalette.selection
                    palette.highlightedText: ColorPalette.onSelection

                    delegate: TreeViewDelegate {
                        id: treeDelegate
                        required property bool isFolder
                        required property string blockId

                        implicitHeight: isFolder ? 32 : 28
                        indentation: General.treeIndent
                        text: model.display

                        background: Rectangle {
                            anchors.fill: parent
                            color: treeDelegate.current
                                   ? treeDelegate.palette.highlight
                                   : "transparent"
                            radius: General.radiusSmall
                        }

                        contentItem: Label {
                            text: treeDelegate.text
                            color: treeDelegate.current
                                   ? treeDelegate.palette.highlightedText
                                   : ColorPalette.textPrimary
                            font.bold: isFolder
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            const modelIndex = blocksTree.index(row, column)
                            blocksTree.selectionModel.setCurrentIndex(
                                        modelIndex, ItemSelectionModel.ClearAndSelect)
                            if (isFolder) {
                                blocksTree.toggleExpanded(row)
                            } else {
                                BlocksModel.selectBlock(blockId)
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {}

                    Component.onCompleted: expandRecursively()
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

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Details"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Edit"
                        enabled: BlocksModel.selectedBlockId.length > 0
                        onClicked: BlockEditorVm.openEditor()
                    }
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
                                value: BlocksModel.selectedBlockId
                            }

                            DetailField {
                                label: "Version"
                                value: BlocksModel.selectedBlockVersion
                            }

                            DetailField {
                                label: "Type"
                                value: BlocksModel.selectedBlockType
                            }

                            DetailField {
                                label: "Language"
                                value: BlocksModel.selectedBlockLanguage
                            }
                        }

                        DetailField {
                            label: "Description"
                            value: BlocksModel.selectedBlockDescription
                            placeholder: "No description"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: "Tags"
                                font.bold: true
                            }

                            Flow {
                                Layout.fillWidth: true
                                width: parent.width
                                spacing: General.spacingSmall

                                Repeater {
                                    model: BlocksModel.selectedBlockTags

                                    delegate: TagChip {
                                        required property string modelData
                                        text: modelData
                                    }
                                }
                            }

                            Label {
                                visible: BlocksModel.selectedBlockTags.length === 0
                                text: "No tags"
                                opacity: 0.72
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: "Defaults"
                                font.bold: true
                            }

                            Flow {
                                Layout.fillWidth: true
                                width: parent.width
                                spacing: General.spacingSmall

                                Repeater {
                                    model: BlocksModel.selectedBlockDefaults

                                    delegate: TagChip {
                                        required property string modelData
                                        text: modelData
                                    }
                                }
                            }

                            Label {
                                visible: BlocksModel.selectedBlockDefaults.length === 0
                                text: "No defaults"
                                opacity: 0.72
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4

                            Label {
                                text: "Template"
                                font.bold: true
                            }

                            CodePreview {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 220
                                text: BlocksModel.selectedBlockTemplate
                                definition: "Markdown"
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: editBlockDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(parent.width - 64, 960)
        height: Math.min(parent.height - 64, 760)
        modal: true
        dim: true
        visible: BlockEditorVm.open
        title: BlockEditorVm.dialogTitle
        standardButtons: Dialog.NoButton
        onClosed: BlockEditorVm.closeEditor()

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
                    Layout.preferredWidth: 320
                    Layout.fillHeight: true
                    background: Rectangle {
                        radius: General.radiusMedium
                        color: ColorPalette.fieldBackground
                        border.color: ColorPalette.border
                    }

                    ScrollView {
                        id: metadataScroll
                        anchors.fill: parent
                        clip: true

                        Column {
                            width: metadataScroll.availableWidth
                            spacing: General.spacingMedium

                            DetailField {
                                width: parent.width
                                label: "Id"
                                value: ""
                                visible: !BlockEditorVm.createMode
                            }

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
                                label: "Current Version"
                                value: BlockEditorVm.currentVersion
                                visible: !BlockEditorVm.createMode
                            }

                            ColumnLayout {
                                width: parent.width
                                spacing: 4

                                Label {
                                    text: "Type"
                                    font.bold: true
                                }

                                ComboBox {
                                    width: parent.width
                                    model: BlockEditorVm.typeOptions
                                    currentIndex: Math.max(0, BlockEditorVm.typeOptions.indexOf(BlockEditorVm.type))
                                    onActivated: BlockEditorVm.type = currentText
                                }
                            }

                            ColumnLayout {
                                width: parent.width
                                spacing: 4

                                Label {
                                    text: "Version Bump"
                                    font.bold: true
                                }

                                ComboBox {
                                    width: parent.width
                                    model: BlockEditorVm.bumpOptions
                                    currentIndex: Math.max(0, BlockEditorVm.bumpOptions.indexOf(BlockEditorVm.bumpMode))
                                    onActivated: BlockEditorVm.bumpMode = currentText
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
                                    text: BlockEditorVm.language
                                    onTextEdited: BlockEditorVm.language = text
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
                                    height: 72
                                    text: BlockEditorVm.description
                                    wrapMode: TextEdit.Wrap
                                    onTextChanged: if (text !== BlockEditorVm.description) BlockEditorVm.description = text
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

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 4

                    Label {
                        text: "Template Editor"
                        font.bold: true
                    }

                    Frame {
                        id: templateEditorFrame
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 520
                        padding: General.paddingMedium
                        background: Rectangle {
                            radius: General.radiusMedium
                            color: ColorPalette.fieldBackground
                            border.color: ColorPalette.borderStrong
                        }

                        ScrollView {
                            id: templateEditorScroll
                            anchors.fill: parent
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            TextEdit {
                                id: templateEditor
                                width: templateEditorScroll.availableWidth
                                text: BlockEditorVm.templateText
                                wrapMode: TextEdit.Wrap
                                selectByMouse: true
                                color: templateEditorFrame.palette.windowText
                                selectedTextColor: templateEditorFrame.palette.highlightedText
                                selectionColor: templateEditorFrame.palette.highlight
                                onTextChanged: if (text !== BlockEditorVm.templateText) BlockEditorVm.templateText = text
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

            Label {
                text: BlockEditorVm.statusText
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.72
            }

            RowLayout {
                Layout.fillWidth: true

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    onClicked: editBlockDialog.close()
                }

                Button {
                    text: BlockEditorVm.saving ? "Saving..." : BlockEditorVm.saveButtonText
                    enabled: !BlockEditorVm.saving
                    onClicked: BlockEditorVm.save()
                }
            }
        }
    }

    Connections {
        target: BlockEditorVm

        function onSaved() {
            editBlockDialog.close()
        }
    }
}
