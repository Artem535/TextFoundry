import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

ApplicationWindow {
    id: root
    width: General.windowWidth
    height: General.windowHeight
    visible: true
    title: "TextFoundry"
    color: ColorPalette.background

    property int currentTab: 0
    property int blocksWorkspaceTab: BlockEditorVm.open ? 1 : 0
    property int pendingNavigationTab: -1
    property bool pendingCloseBlockEditor: false
    property int pendingCloseBlockTabIndex: -1
    property var navigationItems: [
        { title: "Blocks", subtitle: "Templates and blocks", icon: Icons.blocksSvg },
        { title: "Compositions", subtitle: "Prompt assembly", icon: Icons.compositionsSvg },
        { title: "Render", subtitle: "Preview and export", icon: Icons.renderSvg },
        { title: "Settings", subtitle: "Session and engine", icon: Icons.settingsSvg }
    ]

    palette.window: ColorPalette.background
    palette.windowText: ColorPalette.onBackground
    palette.base: ColorPalette.surface
    palette.alternateBase: ColorPalette.surfaceAlt
    palette.text: ColorPalette.onSurface
    palette.button: ColorPalette.button
    palette.buttonText: ColorPalette.onButton
    palette.highlight: ColorPalette.primary
    palette.highlightedText: ColorPalette.onPrimary
    palette.placeholderText: ColorPalette.placeholderText

    function hasDirtyEditor() {
        return (BlockEditorVm.open && BlockEditorVm.anyDirty)
                || (CompositionEditorVm.open && CompositionEditorVm.dirty)
    }

    function requestNavigationTab(index) {
        if (currentTab === index)
            return

        if (hasDirtyEditor()) {
            pendingNavigationTab = index
            pendingCloseBlockEditor = false
            pendingCloseBlockTabIndex = -1
            discardChangesDialog.open()
            return
        }

        currentTab = index
    }

    function requestCloseBlockTab(index) {
        if (index === undefined)
            index = BlockEditorVm.currentTabIndex

        if (index < 0)
            return

        const tabEntry = BlockEditorVm.tabEntries[index]
        if (tabEntry && tabEntry.dirty) {
            pendingNavigationTab = -1
            pendingCloseBlockEditor = false
            pendingCloseBlockTabIndex = index
            discardChangesDialog.open()
            return
        }

        BlockEditorVm.closeTab(index)
    }

    function applyPendingDiscardAction() {
        if (pendingCloseBlockTabIndex >= 0) {
            const tabIndex = pendingCloseBlockTabIndex
            pendingCloseBlockTabIndex = -1
            BlockEditorVm.closeTab(tabIndex)
        }

        if (pendingCloseBlockEditor) {
            pendingCloseBlockEditor = false
            BlockEditorVm.closeEditor()
        }

        if (pendingNavigationTab >= 0) {
            const targetTab = pendingNavigationTab
            pendingNavigationTab = -1
            if (BlockEditorVm.open)
                BlockEditorVm.closeAllEditors()
            if (CompositionEditorVm.open)
                CompositionEditorVm.closeEditor()
            currentTab = targetTab
        }
    }

    header: ToolBar {
        padding: General.headerPadding
        contentHeight: General.headerHeight
        background: Rectangle {
            color: ColorPalette.headerBackground
            border.color: ColorPalette.border
        }

        RowLayout {
            anchors.fill: parent

            Label {
                text: "TextFoundry"
                color: ColorPalette.primary
                font.pixelSize: General.fontLarge
                font.bold: true
            }

            Item { Layout.fillWidth: true }
            Label {
                text: root.navigationItems[root.currentTab].title
                color: ColorPalette.onSurfaceMuted
                font.pixelSize: General.fontSmall
                font.bold: true
            }
        }
    }

    footer: Frame {
        padding: General.footerPadding
        background: Rectangle {
            color: ColorPalette.footerBackground
            border.color: ColorPalette.border
        }

        Label {
            text: SessionVm.statusText
            color: ColorPalette.onSurfaceMuted
            width: parent.width
            elide: Text.ElideRight
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: General.pageMargin

        Frame {
            Layout.preferredWidth: General.sidebarWidth
            Layout.fillHeight: true
            padding: 0
            background: Rectangle {
                color: ColorPalette.surface
                border.color: ColorPalette.border
                radius: General.radiusMedium
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: navigationColumn.implicitHeight

                    ColumnLayout {
                        id: navigationColumn
                        anchors.fill: parent
                        anchors.leftMargin: General.paddingMedium
                        anchors.rightMargin: General.paddingMedium
                        anchors.topMargin: General.paddingLarge
                        spacing: General.spacingSmall

                        Label {
                            text: "Navigation"
                            color: ColorPalette.onSurfaceMuted
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Repeater {
                            model: root.navigationItems

                            delegate: ItemDelegate {
                                required property int index
                                required property var modelData

                                Layout.fillWidth: true
                                highlighted: root.currentTab === index
                                leftPadding: General.paddingMedium
                                rightPadding: General.paddingMedium
                                topPadding: General.paddingSmall
                                bottomPadding: General.paddingSmall
                                onClicked: root.requestNavigationTab(index)

                                background: Rectangle {
                                    radius: General.radiusMedium
                                    color: root.currentTab === index
                                           ? ColorPalette.primary
                                           : (parent.hovered ? ColorPalette.fieldBackground : "transparent")
                                    border.color: root.currentTab === index
                                                  ? "transparent"
                                                  : (parent.hovered ? ColorPalette.border : "transparent")
                                }

                                contentItem: Column {
                                    spacing: 2

                                    RowLayout {
                                        spacing: General.spacingSmall

                                        SvgIcon {
                                            source: modelData.icon
                                            Layout.alignment: Qt.AlignVCenter
                                            color: root.currentTab === index
                                                   ? ColorPalette.onPrimary
                                                   : ColorPalette.textPrimary
                                            iconWidth: 14
                                            iconHeight: 14
                                        }

                                        Label {
                                            text: modelData.title
                                            Layout.alignment: Qt.AlignVCenter
                                            color: root.currentTab === index
                                                   ? ColorPalette.onPrimary
                                                   : ColorPalette.textPrimary
                                            font.pixelSize: 18
                                            font.bold: true
                                        }
                                    }

                                    Label {
                                        text: modelData.subtitle
                                        color: root.currentTab === index
                                               ? ColorPalette.onPrimary
                                               : ColorPalette.onSurfaceMuted
                                        font.pixelSize: 14
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: General.spacingMedium

            Frame {
                visible: root.currentTab === 0
                Layout.fillWidth: true
                padding: General.paddingSmall
                background: Rectangle {
                    color: ColorPalette.surface
                    border.color: ColorPalette.border
                    radius: General.radiusMedium
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: General.spacingSmall

                    Button {
                        text: "Inspect"
                        checkable: true
                        checked: root.blocksWorkspaceTab === 0
                        onClicked: root.blocksWorkspaceTab = 0
                        background: Rectangle {
                            radius: General.radiusMedium
                            color: parent.checked ? ColorPalette.primary : ColorPalette.fieldBackground
                            border.color: parent.checked ? "transparent" : ColorPalette.border
                        }
                        contentItem: Label {
                            text: parent.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: parent.checked ? ColorPalette.onPrimary : ColorPalette.textPrimary
                            font.bold: true
                        }
                    }

                    Repeater {
                        model: BlockEditorVm.tabEntries

                        delegate: RowLayout {
                            required property int index
                            required property var modelData
                            spacing: 0

                            Button {
                                text: (modelData.dirty ? "* " : "") + modelData.title
                                checkable: true
                                checked: root.blocksWorkspaceTab === 1
                                         && BlockEditorVm.currentTabIndex === index
                                onClicked: {
                                    BlockEditorVm.currentTabIndex = index
                                    root.blocksWorkspaceTab = 1
                                }
                                background: Rectangle {
                                    radius: General.radiusMedium
                                    color: parent.checked ? ColorPalette.primary : ColorPalette.fieldBackground
                                    border.color: parent.checked ? "transparent" : ColorPalette.border
                                }
                                contentItem: Label {
                                    text: parent.text
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    color: parent.checked ? ColorPalette.onPrimary : ColorPalette.textPrimary
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                            }

                            SvgToolButton {
                                compact: true
                                iconSource: Icons.closeSvg
                                labelText: "Close"
                                onClicked: root.requestCloseBlockTab(index)
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        visible: root.currentTab === 0 && root.blocksWorkspaceTab === 0
                        spacing: General.spacingSmall

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.addSvg
                            labelText: "New"
                            onClicked: BlockEditorVm.openCreateEditor()
                        }

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.aiAssistSvg
                            labelText: "Slice Into Blocks"
                            toolTipText: "Decompose prompt text into reusable blocks."
                            enabled: BlockSliceVm.aiGenerationAvailable
                            onClicked: BlockSliceVm.openDialog()
                        }

                        CheckBox {
                            text: "Show Derived"
                            checked: BlocksModel.showDerivedBlocks
                            onToggled: BlocksModel.showDerivedBlocks = checked
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                StackLayout {
                    anchors.fill: parent
                    currentIndex: root.currentTab

                    BlocksPage {
                        id: blocksPage
                        rightPaneTab: root.blocksWorkspaceTab
                    }
                    CompositionsPage {}
                    RenderPage {}
                    SettingsPage {}
                }

                BlockSliceDialog {}
                CompositionBlockRewriteDialog {}
            }
        }
    }

    Dialog {
        id: discardChangesDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(parent.width - 64, 460)
        modal: true
        dim: true
        title: "Discard Changes"
        standardButtons: Dialog.NoButton

        background: Rectangle {
            color: ColorPalette.surface
            border.color: ColorPalette.border
            radius: General.radiusMedium
        }

        contentItem: ColumnLayout {
            spacing: General.spacingMedium

            Label {
                Layout.fillWidth: true
                text: "You have unsaved editor changes. Continue and discard them?"
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
                            onClicked: {
                                pendingNavigationTab = -1
                                pendingCloseBlockEditor = false
                                pendingCloseBlockTabIndex = -1
                                discardChangesDialog.close()
                            }
                        }

                SvgToolButton {
                    iconSource: Icons.removeSvg
                    labelText: "Discard"
                    accentColor: ColorPalette.danger
                    onClicked: {
                        discardChangesDialog.close()
                        root.applyPendingDiscardAction()
                    }
                }
            }
        }
    }

    Connections {
        target: BlockEditorVm

        function onSaved() {
            root.blocksWorkspaceTab = 1
        }

        function onOpenChanged() {
            if (BlockEditorVm.open)
                root.blocksWorkspaceTab = 1
            else if (root.blocksWorkspaceTab !== 0)
                root.blocksWorkspaceTab = 0
        }

        function onTabsChanged() {
            if (BlockEditorVm.open)
                root.blocksWorkspaceTab = 1
        }
    }
}
