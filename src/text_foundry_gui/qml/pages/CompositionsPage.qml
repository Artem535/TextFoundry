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

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: General.spacingSmall

                                    Label {
                                        text: "Normalization Presets"
                                        font.bold: true
                                    }

                                    Flow {
                                        Layout.fillWidth: true
                                        spacing: General.spacingSmall

                                        SvgToolButton {
                                            compact: true
                                            iconSource: Icons.aiAssistSvg
                                            labelText: "Formal EN"
                                            onClicked: {
                                                CompositionsVm.tone = "formal"
                                                CompositionsVm.tense = "present"
                                                CompositionsVm.targetLanguage = "en"
                                                CompositionsVm.person = "second"
                                                CompositionsVm.rewriteStrength = "light"
                                                CompositionsVm.terminologyRigidity = "strict"
                                                CompositionsVm.preserveFormatting = true
                                                CompositionsVm.preserveExamples = true
                                            }
                                        }

                                        SvgToolButton {
                                            compact: true
                                            iconSource: Icons.aiAssistSvg
                                            labelText: "Warm RU"
                                            onClicked: {
                                                CompositionsVm.tone = "warm"
                                                CompositionsVm.tense = "present"
                                                CompositionsVm.targetLanguage = "ru"
                                                CompositionsVm.person = "second"
                                                CompositionsVm.rewriteStrength = "light"
                                                CompositionsVm.terminologyRigidity = "strict"
                                                CompositionsVm.preserveFormatting = true
                                                CompositionsVm.preserveExamples = true
                                            }
                                        }

                                        SvgToolButton {
                                            compact: true
                                            iconSource: Icons.aiAssistSvg
                                            labelText: "Neutral 3rd"
                                            onClicked: {
                                                CompositionsVm.tone = "neutral"
                                                CompositionsVm.tense = "present"
                                                CompositionsVm.person = "third"
                                                CompositionsVm.rewriteStrength = "light"
                                                CompositionsVm.terminologyRigidity = "strict"
                                                CompositionsVm.preserveFormatting = true
                                                CompositionsVm.preserveExamples = true
                                            }
                                        }

                                        SvgToolButton {
                                            compact: true
                                            iconSource: Icons.clearSvg
                                            labelText: "Clear Style"
                                            onClicked: {
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
                                    }
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: General.spacingMedium
                                    columnSpacing: General.spacingLarge

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Tone"
                                            font.bold: true
                                        }

                                        TextField {
                                            Layout.fillWidth: true
                                            text: CompositionsVm.tone
                                            placeholderText: "formal"
                                            onTextEdited: CompositionsVm.tone = text
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "How the text should sound: formal, warm, neutral, direct."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Tense"
                                            font.bold: true
                                        }

                                        TextField {
                                            Layout.fillWidth: true
                                            text: CompositionsVm.tense
                                            placeholderText: "present"
                                            onTextEdited: CompositionsVm.tense = text
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "Verb tense to prefer: present, past, or future."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Target Language"
                                            font.bold: true
                                        }

                                        TextField {
                                            Layout.fillWidth: true
                                            text: CompositionsVm.targetLanguage
                                            placeholderText: "en"
                                            onTextEdited: CompositionsVm.targetLanguage = text
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "Language for the derived normalized composition, for example en or ru."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Person"
                                            font.bold: true
                                        }

                                        TextField {
                                            Layout.fillWidth: true
                                            text: CompositionsVm.person
                                            placeholderText: "second"
                                            onTextEdited: CompositionsVm.person = text
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "Point of view to use: first, second, or third person."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Rewrite Strength"
                                            font.bold: true
                                        }

                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: ["light", "medium", "strong"]
                                            currentIndex: Math.max(0, model.indexOf(CompositionsVm.rewriteStrength))
                                            onActivated: CompositionsVm.rewriteStrength = currentText
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "How far the rewrite may go. For prompts this should usually stay on light."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Audience"
                                            font.bold: true
                                        }

                                        TextField {
                                            Layout.fillWidth: true
                                            text: CompositionsVm.audience
                                            placeholderText: "end-user"
                                            onTextEdited: CompositionsVm.audience = text
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "Who the text is written for: end-user, developer, executive."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Locale"
                                            font.bold: true
                                        }

                                        TextField {
                                            Layout.fillWidth: true
                                            text: CompositionsVm.locale
                                            placeholderText: "en-US"
                                            onTextEdited: CompositionsVm.locale = text
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "Regional preference such as en-US, en-GB, or ru-RU."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Terminology"
                                            font.bold: true
                                        }

                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: ["strict", "balanced", "flexible"]
                                            currentIndex: Math.max(0, model.indexOf(CompositionsVm.terminologyRigidity))
                                            onActivated: CompositionsVm.terminologyRigidity = currentText
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "How strictly key terms and names should be preserved."
                                            wrapMode: Text.WordWrap
                                            opacity: 0.72
                                        }
                                    }
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: 2
                                    rowSpacing: General.spacingMedium
                                    columnSpacing: General.spacingLarge

                                    CheckBox {
                                        text: "Preserve Formatting"
                                        checked: CompositionsVm.preserveFormatting
                                        onToggled: CompositionsVm.preserveFormatting = checked
                                    }

                                    CheckBox {
                                        text: "Preserve Examples"
                                        checked: CompositionsVm.preserveExamples
                                        onToggled: CompositionsVm.preserveExamples = checked
                                    }
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: General.spacingSmall

                                    SvgToolButton {
                                        iconSource: Icons.aiAssistSvg
                                        labelText: CompositionsVm.previewingNormalization ? "Previewing..." : "Preview Normalize"
                                        toolTipText: "Preserve structure and adjust style settings such as tone, tense, audience, and locale."
                                        enabled: !CompositionsVm.previewingNormalization
                                                 && !CompositionsVm.normalizing
                                                 && CompositionsVm.normalizationAvailable
                                                 && CompositionsVm.selectedCompositionId.length > 0
                                        onClicked: CompositionsVm.previewNormalizeSelected()
                                    }

                                    SvgToolButton {
                                        iconSource: Icons.saveSvg
                                        labelText: CompositionsVm.normalizing ? "Applying..." : "Apply Normalize"
                                        toolTipText: "Create the derived normalized composition from the current preview settings."
                                        enabled: !CompositionsVm.previewingNormalization
                                                 && !CompositionsVm.normalizing
                                                 && CompositionsVm.hasNormalizationPreview
                                        onClicked: CompositionsVm.normalizeSelected()
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: CompositionsVm.normalizationStatusText
                                    wrapMode: Text.WordWrap
                                    opacity: 0.72
                                    visible: false
                                }

                                Label {
                                    text: CompositionsVm.statusText
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    opacity: 0.72
                                    visible: false
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
                        CompositionsVm.deleteSelected()
                    }
                }
            }
        }
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

        contentItem: Item {
            implicitWidth: compareDialog.availableWidth
            implicitHeight: compareDialog.availableHeight

            ColumnLayout {
                anchors.fill: parent
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
