import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Page {
    id: root
    property string pendingDeleteTitle: ""
    property string pendingDeleteMessage: ""
    readonly property bool detailsStacked: detailsFrame.width < 1180

    function confirmDeleteSelected() {
        if (CompositionsVm.selectedCompositionId.length === 0)
            return

        pendingDeleteTitle = "Delete Composition"
        pendingDeleteMessage = "Delete composition '" + CompositionsVm.selectedCompositionId + "'?"
        deleteConfirmDialog.open()
    }

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
                spacing: General.spacingMedium

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: General.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Compositions"
                            color: ColorPalette.primary
                            font.bold: true
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.addSvg
                            labelText: "New"
                            onClicked: CompositionEditorVm.openCreateEditor()
                        }

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.reloadSvg
                            labelText: "Reload"
                            onClicked: CompositionsVm.reload()
                        }
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Search compositions..."
                        text: CompositionsVm.searchText
                        onTextChanged: CompositionsVm.searchText = text
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280
                    Layout.fillHeight: true
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
                            text: "Compositions"
                            font.bold: true
                        }

                        ListView {
                            id: compositionsList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: CompositionsVm.filteredCompositionIds
                            currentIndex: Math.max(0, CompositionsVm.filteredCompositionIds.indexOf(CompositionsVm.selectedCompositionId))

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

                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    gesturePolicy: TapHandler.ReleaseWithinBounds
                                    onTapped: function(eventPoint) {
                                        compositionsList.currentIndex = index
                                        CompositionsVm.selectComposition(modelData)
                                        compositionContextMenu.popup(eventPoint.position.x,
                                                                     eventPoint.position.y)
                                    }
                                }

                                Menu {
                                    id: compositionContextMenu

                                    MenuItem {
                                        text: "Edit"
                                        onTriggered: CompositionEditorVm.openEditor()
                                    }

                                    MenuItem {
                                        text: "Deprecate"
                                        onTriggered: CompositionsVm.deprecateSelected()
                                    }

                                    MenuItem {
                                        text: "Delete"
                                        onTriggered: root.confirmDeleteSelected()
                                    }
                                }
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 280
                    Layout.fillHeight: true
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
                                text: "Versions"
                                font.bold: true
                            }

                            Item {
                                Layout.fillWidth: true
                            }
                        }

                        ListView {
                            id: versionsList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: General.spacingSmall
                            model: CompositionsVm.versionEntries

                            delegate: ItemDelegate {
                                required property var modelData

                                width: ListView.view.width
                                padding: General.paddingSmall
                                highlighted: modelData.isSelected
                                onClicked: CompositionsVm.selectCompositionVersion(modelData.version)

                                TapHandler {
                                    acceptedButtons: Qt.RightButton
                                    gesturePolicy: TapHandler.ReleaseWithinBounds
                                    onTapped: function(eventPoint) {
                                        versionsList.currentIndex = index
                                        CompositionsVm.selectCompositionVersion(modelData.version)
                                        versionContextMenu.popup(eventPoint.position.x,
                                                                 eventPoint.position.y)
                                    }
                                }

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

                                Menu {
                                    id: versionContextMenu

                                    MenuItem {
                                        text: "Deprecate Version"
                                        onTriggered: CompositionsVm.deprecateSelected()
                                    }

                                    MenuItem {
                                        text: "Compare Raw"
                                        enabled: CompositionsVm.selectedVersions.indexOf(CompositionsVm.selectedVersion) > 0
                                        onTriggered: CompositionsVm.openCompareWithLatest()
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
                    Layout.fillHeight: true
                    columns: root.detailsStacked ? 1 : 2
                    rowSpacing: General.spacingMedium
                    columnSpacing: General.spacingMedium

                    Rectangle {
                        id: detailsMetaPane
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: root.detailsStacked ? -1 : 420
                        Layout.minimumWidth: root.detailsStacked ? 0 : 360
                        Layout.maximumWidth: root.detailsStacked ? Number.POSITIVE_INFINITY : 500
                        Layout.preferredHeight: root.detailsStacked ? 560 : -1
                        radius: General.radiusSmall
                        color: ColorPalette.fieldBackground
                        border.color: ColorPalette.borderStrong

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: General.paddingMedium
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

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
                                        Layout.horizontalStretchFactor: 1
                                        label: "Id"
                                        value: CompositionsVm.selectedCompositionId
                                    }

                                    DetailField {
                                        Layout.horizontalStretchFactor: 1
                                        label: "Version"
                                        value: CompositionsVm.selectedVersion
                                    }

                                    DetailField {
                                        Layout.horizontalStretchFactor: 1
                                        label: "State"
                                        value: CompositionsVm.selectedState
                                    }

                                    DetailField {
                                        Layout.horizontalStretchFactor: 1
                                        label: "Fragments"
                                        value: CompositionsVm.selectedFragmentCount
                                    }
                                }

                                DetailField {
                                    label: "Description"
                                    value: CompositionsVm.selectedDescription
                                    placeholder: "No description"
                                }

                                DetailField {
                                    label: "Revision Comment"
                                    value: CompositionsVm.selectedRevisionComment
                                    placeholder: "No revision comment"
                                }

                                NormalizationStyleForm {
                                    Layout.fillWidth: true
                                    showPresets: true
                                    gridColumns: detailsMetaPane.width < 430 ? 1 : 2
                                    headingText: "Asset Normalization"
                                    hintText: "Preview block-level changes, then create a normalized composition version from the selected asset."
                                    tone: CompositionsVm.tone
                                    tense: CompositionsVm.tense
                                    targetLanguage: CompositionsVm.targetLanguage
                                    person: CompositionsVm.person
                                    rewriteStrength: CompositionsVm.rewriteStrength
                                    audience: CompositionsVm.audience
                                    locale: CompositionsVm.locale
                                    terminologyRigidity: CompositionsVm.terminologyRigidity
                                    preserveFormatting: CompositionsVm.preserveFormatting
                                    preserveExamples: CompositionsVm.preserveExamples
                                    onToneEdited: function(value) { CompositionsVm.tone = value }
                                    onTenseEdited: function(value) { CompositionsVm.tense = value }
                                    onTargetLanguageEdited: function(value) { CompositionsVm.targetLanguage = value }
                                    onPersonEdited: function(value) { CompositionsVm.person = value }
                                    onRewriteStrengthEdited: function(value) { CompositionsVm.rewriteStrength = value }
                                    onAudienceEdited: function(value) { CompositionsVm.audience = value }
                                    onLocaleEdited: function(value) { CompositionsVm.locale = value }
                                    onTerminologyRigidityEdited: function(value) { CompositionsVm.terminologyRigidity = value }
                                    onPreserveFormattingEdited: function(value) { CompositionsVm.preserveFormatting = value }
                                    onPreserveExamplesEdited: function(value) { CompositionsVm.preserveExamples = value }
                                    onFormalEnglishPresetRequested: {
                                        CompositionsVm.tone = "formal"
                                        CompositionsVm.tense = "present"
                                        CompositionsVm.targetLanguage = "en"
                                        CompositionsVm.person = "second"
                                        CompositionsVm.rewriteStrength = "light"
                                        CompositionsVm.terminologyRigidity = "strict"
                                        CompositionsVm.preserveFormatting = true
                                        CompositionsVm.preserveExamples = true
                                    }
                                    onWarmRussianPresetRequested: {
                                        CompositionsVm.tone = "warm"
                                        CompositionsVm.tense = "present"
                                        CompositionsVm.targetLanguage = "ru"
                                        CompositionsVm.person = "second"
                                        CompositionsVm.rewriteStrength = "light"
                                        CompositionsVm.terminologyRigidity = "strict"
                                        CompositionsVm.preserveFormatting = true
                                        CompositionsVm.preserveExamples = true
                                    }
                                    onNeutralThirdPresetRequested: {
                                        CompositionsVm.tone = "neutral"
                                        CompositionsVm.tense = "present"
                                        CompositionsVm.person = "third"
                                        CompositionsVm.rewriteStrength = "light"
                                        CompositionsVm.terminologyRigidity = "strict"
                                        CompositionsVm.preserveFormatting = true
                                        CompositionsVm.preserveExamples = true
                                    }
                                    onClearStyleRequested: {
                                        CompositionsVm.tone = ""
                                        CompositionsVm.tense = ""
                                        CompositionsVm.targetLanguage = ""
                                        CompositionsVm.person = ""
                                        CompositionsVm.rewriteStrength = "light"
                                        CompositionsVm.audience = ""
                                        CompositionsVm.locale = ""
                                        CompositionsVm.terminologyRigidity = "strict"
                                        CompositionsVm.preserveFormatting = true
                                        CompositionsVm.preserveExamples = true
                                    }
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: General.spacingSmall

                                    SvgToolButton {
                                        iconSource: Icons.aiAssistSvg
                                        labelText: CompositionsVm.previewingNormalization ? "Previewing..." : "Preview Asset Rewrite"
                                        toolTipText: "Preview block-level normalization while preserving composition structure and block ids."
                                        enabled: !CompositionsVm.previewingNormalization
                                                 && !CompositionsVm.normalizing
                                                 && CompositionsVm.normalizationAvailable
                                                 && CompositionsVm.selectedCompositionId.length > 0
                                        onClicked: CompositionsVm.previewNormalizeSelected()
                                    }

                                    SvgToolButton {
                                        iconSource: Icons.saveSvg
                                        labelText: CompositionsVm.normalizing ? "Creating..." : "Create Normalized Version"
                                        toolTipText: "Publish the preview as a normalized composition version with derived block changes."
                                        enabled: !CompositionsVm.previewingNormalization
                                                 && !CompositionsVm.normalizing
                                                 && CompositionsVm.hasNormalizationPreview
                                        onClicked: CompositionsVm.normalizeSelected()
                                    }
                                }

                                ProgressBar {
                                    Layout.fillWidth: true
                                    indeterminate: true
                                    visible: CompositionsVm.previewingNormalization
                                             || CompositionsVm.normalizing
                                }

                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: root.detailsStacked ? 0 : 420
                        Layout.preferredHeight: root.detailsStacked ? 420 : -1
                        radius: General.radiusSmall
                        color: ColorPalette.fieldBackground
                        border.color: ColorPalette.borderStrong

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: General.paddingMedium
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    text: "Content"
                                    font.bold: true
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: CompositionsVm.hasNormalizationPreview
                                          ? CompositionsVm.normalizationPreviewTargetId
                                          : ""
                                    opacity: 0.72
                                    visible: CompositionsVm.hasNormalizationPreview
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
                                    iconSource: Icons.aiAssistSvg
                                    labelText: "Rewrite Blocks"
                                    toolTipText: "Preview block-level AI patches without changing composition structure."
                                    enabled: CompositionsVm.selectedCompositionId.length > 0
                                             && CompositionBlockRewriteVm.rewriteAvailable
                                    onClicked: CompositionBlockRewriteVm.openDialog(
                                                   CompositionsVm.selectedCompositionId,
                                                   CompositionsVm.selectedVersion)
                                }

                                SvgToolButton {
                                    compact: true
                                    iconSource: Icons.reloadSvg
                                    labelText: "Use Latest Blocks"
                                    enabled: CompositionsVm.selectedCompositionId.length > 0
                                    onClicked: CompositionsVm.updateBlocksToLatest()
                                }

                                SvgToolButton {
                                    compact: true
                                    iconSource: Icons.copySvg
                                    labelText: "Compare Raw"
                                    enabled: CompositionsVm.selectedVersions.indexOf(CompositionsVm.selectedVersion) > 0
                                    onClicked: CompositionsVm.openCompareWithLatest()
                                }

                                SvgToolButton {
                                    compact: true
                                    iconSource: Icons.deprecateSvg
                                    labelText: "Deprecate"
                                    accentColor: ColorPalette.warning
                                    enabled: CompositionsVm.selectedCompositionId.length > 0
                                    onClicked: CompositionsVm.deprecateSelected()
                                }

                                SvgToolButton {
                                    compact: true
                                    iconSource: Icons.removeSvg
                                    labelText: "Delete"
                                    accentColor: ColorPalette.danger
                                    enabled: CompositionsVm.selectedCompositionId.length > 0
                                    onClicked: root.confirmDeleteSelected()
                                }
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
                                    visible: !CompositionsVm.hasNormalizationPreview

                                    delegate: Label {
                                        required property string modelData
                                        width: ListView.view.width
                                        text: modelData
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                CodePreview {
                                    anchors.fill: parent
                                    anchors.margins: General.paddingSmall
                                    visible: CompositionsVm.hasNormalizationPreview
                                    text: CompositionsVm.normalizationPreviewText
                                    definition: "Markdown"
                                }
                            }
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

    ConfirmDialog {
        id: deleteConfirmDialog
        width: Math.min(parent.width - 64, 440)
        title: root.pendingDeleteTitle
        messageText: root.pendingDeleteMessage
        confirmText: "Delete"
        confirmIconSource: Icons.removeSvg
        confirmAccentColor: ColorPalette.danger
        onConfirmed: CompositionsVm.deleteSelected()
    }

    Dialog {
        id: compareDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(parent.width - 80, 1180)
        height: Math.min(parent.height - 80, 780)
        modal: true
        dim: true
        visible: CompositionsVm.compareOpen
        title: "Compare Raw Prompts"
        standardButtons: Dialog.NoButton
        onClosed: CompositionsVm.closeCompare()

        background: Rectangle {
            radius: General.radiusMedium
            color: ColorPalette.surface
            border.color: ColorPalette.border
        }

        contentItem: ColumnLayout {
            implicitWidth: compareDialog.availableWidth
            implicitHeight: compareDialog.availableHeight
            spacing: General.spacingMedium

            Label {
                Layout.fillWidth: true
                text: CompositionsVm.compareSummary
                wrapMode: Text.WordWrap
                opacity: 0.78
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: General.spacingMedium

                Label {
                    Layout.fillWidth: true
                    text: CompositionsVm.compareLeftTitle
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    text: CompositionsVm.compareRightTitle
                    font.bold: true
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.fillHeight: true
                padding: 0
                background: Rectangle {
                    radius: General.radiusMedium
                    color: ColorPalette.fieldBackground
                    border.color: ColorPalette.border
                }

                ListView {
                    id: compareRowsView
                    anchors.fill: parent
                    anchors.margins: General.paddingMedium
                    clip: true
                    spacing: General.spacingSmall
                    model: CompositionsVm.compareRows

                    ScrollBar.horizontal: ScrollBar {
                        policy: ScrollBar.AlwaysOff
                    }
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    delegate: Item {
                        required property var modelData

                        function diffBackground(kind) {
                            if (kind === "removed")
                                return "#4d2b31"
                            if (kind === "added")
                                return "#1f4a37"
                            if (kind === "changed")
                                return "#4e3f1f"
                            return "transparent"
                        }

                        width: compareRowsView.width
                        height: Math.max(leftCellText.implicitHeight, rightCellText.implicitHeight) + 8

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: General.paddingSmall
                            anchors.rightMargin: General.paddingSmall
                            spacing: General.spacingMedium

                            RowLayout {
                                id: leftLineRow
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignTop
                                spacing: General.spacingSmall

                                Label {
                                    Layout.preferredWidth: 36
                                    Layout.alignment: Qt.AlignTop
                                    horizontalAlignment: Text.AlignRight
                                    color: ColorPalette.onSurfaceMuted
                                    opacity: 0.72
                                    font.family: General.monospaceFamily
                                    text: modelData.leftLineNumber
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 2
                                    color: diffBackground(modelData.leftKind)

                                    Label {
                                        id: leftCellText
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        text: modelData.leftText.length > 0 ? modelData.leftText : " "
                                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                        font.family: General.monospaceFamily
                                        verticalAlignment: Text.AlignTop
                                    }
                                }
                            }

                            RowLayout {
                                id: rightLineRow
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignTop
                                spacing: General.spacingSmall

                                Label {
                                    Layout.preferredWidth: 36
                                    Layout.alignment: Qt.AlignTop
                                    horizontalAlignment: Text.AlignRight
                                    color: ColorPalette.onSurfaceMuted
                                    opacity: 0.72
                                    font.family: General.monospaceFamily
                                    text: modelData.rightLineNumber
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 2
                                    color: diffBackground(modelData.rightKind)

                                    Label {
                                        id: rightCellText
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        text: modelData.rightText.length > 0 ? modelData.rightText : " "
                                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                        font.family: General.monospaceFamily
                                        verticalAlignment: Text.AlignTop
                                    }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                SvgToolButton {
                    iconSource: Icons.clearSvg
                    labelText: "Close"
                    onClicked: CompositionsVm.closeCompare()
                }
            }
        }
    }
}
