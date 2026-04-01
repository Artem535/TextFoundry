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

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.addSvg
                        labelText: "New"
                        onClicked: BlockEditorVm.openCreateEditor()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.aiAssistSvg
                        labelText: "AI Slice"
                        enabled: BlockSliceVm.aiGenerationAvailable
                        onClicked: BlockSliceVm.openDialog()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.reloadSvg
                        labelText: "Reload"
                        onClicked: BlocksModel.reload()
                    }

                    CheckBox {
                        text: "Show Derived"
                        checked: BlocksModel.showDerivedBlocks
                        onToggled: BlocksModel.showDerivedBlocks = checked
                    }
                }

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Search block id, description, template, tags..."
                    text: BlocksModel.searchText
                    onTextChanged: BlocksModel.searchText = text
                }

                TreeView {
                    id: blocksTree
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    property var expandedFolders: ({})
                    model: BlocksModel
                    selectionModel: ItemSelectionModel {}
                    columnWidthProvider: function(column) {
                        return column === 0 ? blocksTree.width : 0
                    }
                    palette.highlight: ColorPalette.selection
                    palette.highlightedText: ColorPalette.onSelection

                    function rememberExpandedFolders() {
                        const remembered = {}

                        function visit(parentIndex) {
                            const count = BlocksModel.rowCount(parentIndex)
                            for (let row = 0; row < count; ++row) {
                                const index = BlocksModel.index(row, 0, parentIndex)
                                if (!index.valid)
                                    continue

                                const isFolder = BlocksModel.data(index, BlocksModel.IsFolderRole)
                                if (!isFolder)
                                    continue

                                const fullPath = BlocksModel.data(index, BlocksModel.FullPathRole)
                                if (blocksTree.isExpanded(row, parentIndex))
                                    remembered[fullPath] = true

                                visit(index)
                            }
                        }

                        visit(Qt.invalidModelIndex)
                        expandedFolders = remembered
                    }

                    function restoreExpandedFolders() {
                        function visit(parentIndex) {
                            const count = BlocksModel.rowCount(parentIndex)
                            for (let row = 0; row < count; ++row) {
                                const index = BlocksModel.index(row, 0, parentIndex)
                                if (!index.valid)
                                    continue

                                const isFolder = BlocksModel.data(index, BlocksModel.IsFolderRole)
                                if (!isFolder)
                                    continue

                                const fullPath = BlocksModel.data(index, BlocksModel.FullPathRole)
                                if (expandedFolders[fullPath])
                                    blocksTree.expand(row, parentIndex)

                                visit(index)
                            }
                        }

                        visit(Qt.invalidModelIndex)
                    }

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
                            RowLayout {
                                anchors.fill: parent
                                spacing: General.spacingSmall

                                SvgIcon {
                                    visible: isFolder
                                    source: blocksTree.isExpanded(row)
                                            ? Icons.folderOpenSvg
                                            : Icons.folderSvg
                                    color: treeDelegate.current
                                           ? treeDelegate.palette.highlightedText
                                           : ColorPalette.textPrimary
                                    iconWidth: 15
                                    iconHeight: 15
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: treeDelegate.text
                                    color: treeDelegate.current
                                           ? treeDelegate.palette.highlightedText
                                           : ColorPalette.textPrimary
                                    font.bold: isFolder
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        onClicked: {
                            const modelIndex = blocksTree.index(row, column)
                            blocksTree.selectionModel.setCurrentIndex(
                                        modelIndex, ItemSelectionModel.ClearAndSelect)
                            if (isFolder) {
                                blocksTree.rememberExpandedFolders()
                                blocksTree.toggleExpanded(row)

                                const fullPath = model.fullPath
                                if (blocksTree.isExpanded(row))
                                    blocksTree.expandedFolders[fullPath] = true
                                else
                                    delete blocksTree.expandedFolders[fullPath]
                            } else {
                                BlocksModel.selectBlock(blockId)
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {}

                    Component.onCompleted: expandRecursively()

                    Connections {
                        target: BlocksModel

                        function onTreeReloaded() {
                            if (BlocksModel.searchText.trim().length > 0)
                                blocksTree.expandRecursively()
                            else
                                blocksTree.restoreExpandedFolders()
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

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Details"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.editSvg
                        labelText: "Edit"
                        enabled: BlocksModel.selectedBlockId.length > 0
                        onClicked: BlockEditorVm.openEditor()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.deprecateSvg
                        labelText: "Deprecate"
                        enabled: BlocksModel.selectedBlockId.length > 0
                        onClicked: BlocksModel.deprecateSelected()
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

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Version"
                                    font.bold: true
                                }

                                ComboBox {
                                    Layout.fillWidth: true
                                    model: BlocksModel.selectedBlockVersionOptions
                                    enabled: BlocksModel.selectedBlockVersions.length > 0
                                    currentIndex: Math.max(0, BlocksModel.selectedBlockVersions.indexOf(BlocksModel.selectedBlockVersion))
                                    onActivated: BlocksModel.selectBlockVersion(BlocksModel.selectedBlockVersions[currentIndex])
                                }
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
                            value: BlocksModel.highlightSearchText(BlocksModel.selectedBlockDescription)
                            placeholder: "No description"
                            richText: true
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

                            Loader {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 220
                                active: true
                                sourceComponent: BlocksModel.searchText.trim().length > 0
                                                 ? highlightedTemplatePreview
                                                 : plainTemplatePreview
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: plainTemplatePreview

        CodePreview {
            text: BlocksModel.selectedBlockTemplate
            definition: "Markdown"
        }
    }

    Component {
        id: highlightedTemplatePreview

        Frame {
            padding: General.paddingMedium

            background: Rectangle {
                radius: General.radiusSmall
                color: ColorPalette.fieldBackground
                border.color: ColorPalette.borderStrong
            }

            ScrollView {
                id: highlightedTemplateScroll
                anchors.fill: parent
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                TextEdit {
                    width: highlightedTemplateScroll.availableWidth
                    text: "<div style='color:" + ColorPalette.textPrimary
                          + ";'>" + BlocksModel.highlightSearchContent(BlocksModel.selectedBlockTemplate)
                          + "</div>"
                    textFormat: TextEdit.RichText
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    font.family: General.monospaceFamily
                    font.pixelSize: SessionVm.previewFontSize
                    color: highlightedTemplateScroll.palette.windowText
                    selectedTextColor: highlightedTemplateScroll.palette.highlightedText
                    selectionColor: highlightedTemplateScroll.palette.highlight
                }
            }
        }
    }

    Dialog {
        id: slicePromptDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(parent.width - 64, 1040)
        height: Math.min(parent.height - 64, 780)
        modal: true
        dim: true
        visible: BlockSliceVm.open
        title: "AI Slice Prompt Into Blocks"
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

                        Column {
                            width: sliceScroll.availableWidth
                            spacing: General.spacingMedium

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
                                    text: "Source Prompt"
                                    font.bold: true
                                }

                                TextArea {
                                    width: parent.width
                                    height: 420
                                    text: BlockSliceVm.sourcePromptText
                                    placeholderText: "Paste the full prompt you want to decompose"
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
                                text: "Composition Preview"
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

            Label {
                text: BlockSliceVm.statusText
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                opacity: 0.72
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
                    labelText: BlockSliceVm.publishing ? "Publishing..." : "Publish All"
                    enabled: !BlockSliceVm.generating
                             && !BlockSliceVm.publishing
                             && BlockSliceVm.generatedCount > 0
                    onClicked: BlockSliceVm.publishAll()
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
                                    text: "AI Prompt"
                                    font.bold: true
                                }

                                TextArea {
                                    width: parent.width
                                    height: 120
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
                                font.family: General.monospaceFamily
                                font.pixelSize: SessionVm.previewFontSize
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

                SvgToolButton {
                    iconSource: Icons.closeSvg
                    labelText: "Cancel"
                    onClicked: editBlockDialog.close()
                }

                SvgToolButton {
                    iconSource: Icons.aiAssistSvg
                    labelText: BlockEditorVm.generating ? "Generating..." : "Generate"
                    enabled: !BlockEditorVm.generating
                             && !BlockEditorVm.saving
                             && BlockEditorVm.aiGenerationAvailable
                    onClicked: BlockEditorVm.generate()
                }

                SvgToolButton {
                    iconSource: Icons.saveSvg
                    labelText: BlockEditorVm.saving ? "Saving..." : BlockEditorVm.saveButtonText
                    enabled: !BlockEditorVm.saving && !BlockEditorVm.generating
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

    Connections {
        target: BlockSliceVm

        function onPublished() {
            slicePromptDialog.close()
        }
    }
}
