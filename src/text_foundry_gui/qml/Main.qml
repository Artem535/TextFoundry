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
                                onClicked: root.currentTab = index

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

                    Button {
                        visible: BlockEditorVm.open
                        text: BlockEditorVm.createMode
                              ? "New Block"
                              : (BlockEditorVm.blockId.length > 0
                                 ? "Edit " + BlockEditorVm.blockId
                                 : "Edit")
                        checkable: true
                        checked: root.blocksWorkspaceTab === 1
                        onClicked: root.blocksWorkspaceTab = 1
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
                        visible: BlockEditorVm.open
                        compact: true
                        iconSource: Icons.closeSvg
                        labelText: "Close"
                        onClicked: BlockEditorVm.closeEditor()
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.currentTab

                BlocksPage {
                    rightPaneTab: root.blocksWorkspaceTab
                }
                CompositionsPage {}
                RenderPage {}
                SettingsPage {}
            }
        }
    }

    BlockSliceDialog {}

    Connections {
        target: BlockEditorVm

        function onSaved() {
            root.blocksWorkspaceTab = 0
        }

        function onOpenChanged() {
            if (BlockEditorVm.open)
                root.blocksWorkspaceTab = 1
            else if (root.blocksWorkspaceTab !== 0)
                root.blocksWorkspaceTab = 0
        }
    }
}
