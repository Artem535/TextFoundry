import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.syntaxhighlighting
import TextFoundry

Page {
    id: root
    property int rightPaneTab: 0
    property string pendingDeleteTitle: ""
    property string pendingDeleteMessage: ""
    readonly property bool detailsStacked: detailsFrame.width < 1180

    function collapseAllFolders() {
        blocksTree.expandedFolders = ({})
        blocksTree.collapseRecursively()
    }

    function expandAllFolders() {
        blocksTree.expandRecursively()
        blocksTree.rememberExpandedFolders()
    }

    function confirmDeleteSelection() {
        const isFolder = BlocksModel.selectedTreeIsFolder
        const targetLabel = isFolder
                ? BlocksModel.selectedTreePath
                : BlocksModel.selectedBlockId
        if (!targetLabel || targetLabel.length === 0)
            return

        pendingDeleteTitle = isFolder ? "Delete Folder" : "Delete Block"
        pendingDeleteMessage = isFolder
                ? "Delete folder '" + targetLabel + "' and all removable blocks inside it?"
                : "Delete block '" + targetLabel + "'?"
        deleteConfirmDialog.open()
    }

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

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: "Blocks"
                                font.bold: true
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            SvgToolButton {
                                compact: true
                                iconSource: Icons.folderOpenSvg
                                labelText: "Expand All"
                                onClicked: root.expandAllFolders()
                            }

                            SvgToolButton {
                                compact: true
                                iconSource: Icons.folderSvg
                                labelText: "Collapse All"
                                onClicked: root.collapseAllFolders()
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

                                        visibleRow += 1
                                        const isFolder = BlocksModel.data(index, BlocksModel.IsFolderRole)
                                        if (!isFolder)
                                            continue

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

                                        visibleRow += 1
                                        const isFolder = BlocksModel.data(index, BlocksModel.IsFolderRole)
                                        if (!isFolder)
                                            continue

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
                                            root.confirmDeleteSelection()
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

                            Component.onCompleted: root.expandAllFolders()

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
            id: detailsFrame
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

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.rightPaneTab

                    Item {
                        GridLayout {
                            anchors.fill: parent
                            columns: root.detailsStacked ? 1 : 2
                            rowSpacing: General.spacingMedium
                            columnSpacing: General.spacingMedium

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: root.detailsStacked ? -1 : 340
                                Layout.minimumWidth: root.detailsStacked ? 0 : 300
                                Layout.maximumWidth: root.detailsStacked ? Number.POSITIVE_INFINITY : 420
                                Layout.preferredHeight: root.detailsStacked ? 420 : -1
                                radius: General.radiusSmall
                                color: ColorPalette.fieldBackground
                                border.color: ColorPalette.borderStrong

                                ScrollView {
                                    anchors.fill: parent
                                    anchors.margins: General.paddingMedium
                                    clip: true
                                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                                    ColumnLayout {
                                        width: Math.max(0, parent.availableWidth)
                                        spacing: General.spacingMedium

                                        RowLayout {
                                            Layout.fillWidth: true

                                            Label {
                                                text: "Details"
                                                color: ColorPalette.primary
                                                font.bold: true
                                            }
                                        }

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
                                Layout.preferredWidth: root.detailsStacked ? -1 : 820
                                Layout.minimumWidth: root.detailsStacked ? 0 : 620
                                Layout.preferredHeight: root.detailsStacked ? 520 : -1
                                radius: General.radiusSmall
                                color: ColorPalette.fieldBackground
                                border.color: ColorPalette.borderStrong

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: General.paddingMedium
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Label {
                                            text: "Template"
                                            font.bold: true
                                        }

                                        Item {
                                            Layout.fillWidth: true
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
                                            accentColor: ColorPalette.warning
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
                                            accentColor: ColorPalette.danger
                                            enabled: BlocksModel.selectedTreePath.length > 0
                                                     || BlocksModel.selectedBlockId.length > 0
                                            onClicked: {
                                                blocksTree.rememberExpandedFolders()
                                                root.confirmDeleteSelection()
                                            }
                                        }
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

    Dialog {
        id: deleteConfirmDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(parent.width - 64, 440)
        modal: true
        dim: true
        title: root.pendingDeleteTitle
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
                text: root.pendingDeleteMessage
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                SvgToolButton {
                    iconSource: Icons.closeSvg
                    labelText: "Cancel"
                    onClicked: deleteConfirmDialog.close()
                }

                SvgToolButton {
                    iconSource: Icons.removeSvg
                    labelText: "Delete"
                    accentColor: ColorPalette.danger
                    onClicked: {
                        deleteConfirmDialog.close()
                        BlocksModel.deleteSelected()
                    }
                }
            }
        }
    }

    Connections {
        target: BlockEditorVm

        function onSaved() {
        }
    }
}
