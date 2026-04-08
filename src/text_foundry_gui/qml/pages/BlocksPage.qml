import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.syntaxhighlighting
import TextFoundry

Page {
    id: root
    property int rightPaneTab: 0

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
                spacing: General.spacingMedium

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Blocks"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }
                }

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Search block id, description, template, tags..."
                    text: BlocksModel.searchText
                    onTextChanged: BlocksModel.searchText = text
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 320
                    padding: 0
                    background: Rectangle {
                        radius: General.radiusMedium
                        color: ColorPalette.fieldBackground
                        border.color: ColorPalette.border
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: General.paddingSmall
                        spacing: General.spacingSmall

                        Label {
                            text: "Blocks"
                            font.bold: true
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
                                let visibleRow = -1

                                function visit(parentIndex) {
                                    const count = BlocksModel.rowCount(parentIndex)
                                    for (let row = 0; row < count; ++row) {
                                        const index = BlocksModel.index(row, 0, parentIndex)
                                        if (!index.valid)
                                            continue

                                        const isFolder = BlocksModel.data(index, BlocksModel.IsFolderRole)
                                        if (!isFolder)
                                            continue

                                        visibleRow += 1
                                        const fullPath = BlocksModel.data(index, BlocksModel.FullPathRole)
                                        const expanded = blocksTree.isExpanded(visibleRow)
                                        if (expanded)
                                            remembered[fullPath] = true

                                        if (expanded)
                                            visit(index)
                                    }
                                }

                                visit(Qt.invalidModelIndex)
                                expandedFolders = remembered
                            }

                            function restoreExpandedFolders() {
                                let visibleRow = -1

                                function visit(parentIndex) {
                                    const count = BlocksModel.rowCount(parentIndex)
                                    for (let row = 0; row < count; ++row) {
                                        const index = BlocksModel.index(row, 0, parentIndex)
                                        if (!index.valid)
                                            continue

                                        const isFolder = BlocksModel.data(index, BlocksModel.IsFolderRole)
                                        if (!isFolder)
                                            continue

                                        visibleRow += 1
                                        const fullPath = BlocksModel.data(index, BlocksModel.FullPathRole)
                                        const shouldExpand = !!expandedFolders[fullPath]
                                        if (shouldExpand)
                                            blocksTree.expand(visibleRow)

                                        if (shouldExpand)
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
                                    BlocksModel.selectTreeItem(model.fullPath, isFolder, blockId)
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

                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    gesturePolicy: TapHandler.ReleaseWithinBounds
                                    onTapped: function(eventPoint) {
                                        const modelIndex = blocksTree.index(row, column)
                                        blocksTree.selectionModel.setCurrentIndex(
                                                    modelIndex, ItemSelectionModel.ClearAndSelect)
                                        BlocksModel.selectTreeItem(model.fullPath, isFolder, blockId)
                                        if (!isFolder)
                                            BlocksModel.selectBlock(blockId)
                                        blockContextMenu.popup(eventPoint.position.x,
                                                               eventPoint.position.y)
                                    }
                                }

                                Menu {
                                    id: blockContextMenu

                                    MenuItem {
                                        text: isFolder ? "Delete Folder" : "Delete"
                                        onTriggered: {
                                            blocksTree.rememberExpandedFolders()
                                            BlocksModel.deleteSelected()
                                        }
                                    }

                                    MenuItem {
                                        text: "Edit"
                                        visible: !isFolder
                                        onTriggered: BlockEditorVm.openEditor()
                                    }

                                    MenuItem {
                                        text: "Deprecate"
                                        visible: !isFolder
                                        onTriggered: {
                                            blocksTree.rememberExpandedFolders()
                                            BlocksModel.deprecateSelected()
                                        }
                                    }

                                    MenuItem {
                                        text: isFolder
                                              ? (blocksTree.isExpanded(row) ? "Collapse" : "Expand")
                                              : "AI Slice"
                                        onTriggered: {
                                            if (isFolder) {
                                                blocksTree.rememberExpandedFolders()
                                                blocksTree.toggleExpanded(row)
                                            } else {
                                                BlockSliceVm.openDialog()
                                            }
                                        }
                                    }
                                }
                            }

                            ScrollBar.vertical: ScrollBar {}

                            Component.onCompleted: expandRecursively()

                            Connections {
                                target: BlocksModel

                                function onTreeReloaded() {
                                    Qt.callLater(function() {
                                        if (BlocksModel.searchText.trim().length > 0)
                                            blocksTree.expandRecursively()
                                        else
                                            blocksTree.restoreExpandedFolders()
                                    })
                                }
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 280
                    padding: 0
                    background: Rectangle {
                        radius: General.radiusMedium
                        color: ColorPalette.fieldBackground
                        border.color: ColorPalette.border
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: General.paddingSmall
                        spacing: General.spacingSmall

                        Label {
                            text: "Versions"
                            font.bold: true
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: General.spacingSmall
                            model: BlocksModel.versionEntries

                            delegate: ItemDelegate {
                                required property var modelData

                                width: ListView.view.width
                                padding: General.paddingSmall
                                highlighted: modelData.isSelected
                                onClicked: BlocksModel.selectBlockVersion(modelData.version)

                                contentItem: ColumnLayout {
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Label {
                                            text: modelData.label
                                            font.bold: true
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Label {
                                            visible: modelData.state.length > 0
                                            text: modelData.state
                                            opacity: 0.72
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.comment.length > 0
                                              ? modelData.comment
                                              : "No revision comment"
                                        wrapMode: Text.WordWrap
                                        opacity: modelData.comment.length > 0 ? 0.82 : 0.58
                                    }
                                }
                            }
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
                        text: root.rightPaneTab === 0 ? "Inspect" : "Block Editor"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Loader {
                        active: root.rightPaneTab === 0
                        visible: active
                        sourceComponent: inspectToolbar
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.rightPaneTab

                    Item {
                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            rowSpacing: General.spacingMedium
                            columnSpacing: General.spacingMedium

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 340
                                Layout.minimumWidth: 300
                                Layout.maximumWidth: 420
                                radius: General.radiusSmall
                                color: ColorPalette.fieldBackground
                                border.color: ColorPalette.borderStrong

                                ScrollView {
                                    anchors.fill: parent
                                    anchors.margins: General.paddingMedium
                                    clip: true

                                    ColumnLayout {
                                        width: Math.max(0, parent.availableWidth)
                                        spacing: General.spacingMedium

                                        GridLayout {
                                            Layout.fillWidth: true
                                            columns: 2
                                            rowSpacing: General.spacingMedium
                                            columnSpacing: General.spacingLarge

                                            DetailField {
                                                Layout.columnSpan: 2
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
                                            value: BlocksModel.highlightSearchText(BlocksModel.selectedBlockDescription)
                                            placeholder: "No description"
                                            richText: true
                                        }

                                        DetailField {
                                            label: "Revision Comment"
                                            value: BlocksModel.highlightSearchText(BlocksModel.selectedBlockRevisionComment)
                                            placeholder: "No revision comment"
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
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 820
                                Layout.minimumWidth: 620
                                radius: General.radiusSmall
                                color: ColorPalette.fieldBackground
                                border.color: ColorPalette.borderStrong

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: General.paddingMedium
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
                                        active: true
                                        sourceComponent: BlocksModel.searchText.trim().length > 0
                                                         ? highlightedTemplatePreview
                                                         : plainTemplatePreview
                                    }
                                }
                            }
                        }
                    }

                    BlockEditorWorkspace {}
                }
            }
        }
    }

    Component {
        id: inspectToolbar

        RowLayout {
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
                onClicked: {
                    blocksTree.rememberExpandedFolders()
                    BlocksModel.reload()
                }
            }

            CheckBox {
                text: "Show Derived"
                checked: BlocksModel.showDerivedBlocks
                onToggled: BlocksModel.showDerivedBlocks = checked
            }

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
                onClicked: {
                    blocksTree.rememberExpandedFolders()
                    BlocksModel.deprecateSelected()
                }
            }

            SvgToolButton {
                compact: true
                iconSource: Icons.removeSvg
                labelText: BlocksModel.selectedTreeIsFolder ? "Delete Folder" : "Delete"
                enabled: BlocksModel.selectedTreePath.length > 0
                         || BlocksModel.selectedBlockId.length > 0
                onClicked: {
                    blocksTree.rememberExpandedFolders()
                    BlocksModel.deleteSelected()
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

        ScrollView {
            id: highlightedTemplateScroll
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

    Connections {
        target: BlockEditorVm

        function onSaved() {
        }
    }
}
