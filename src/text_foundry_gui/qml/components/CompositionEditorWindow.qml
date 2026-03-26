import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Item {
    id: editorRoot

    property var targetResolver
    property int dropPreviewIndex: -1
    property color toolbarForeground: ColorPalette.onSurface

    function itemCenterInContainer(item) {
        const point = item.mapToItem(fragmentList.contentItem, 0, item.height / 2)
        return point.y
    }

    function resolveDropIndex(yPos) {
        for (let i = 0; i < fragmentList.count; ++i) {
            const item = fragmentList.itemAtIndex(i)
            if (!item) {
                continue
            }
            const middle = editorRoot.itemCenterInContainer(item)
            if (yPos < middle) {
                return i
            }
        }
        return Math.max(0, fragmentList.count - 1)
    }

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
                        text: CompositionEditorVm.dialogTitle
                        font.bold: true
                        color: ColorPalette.primary
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Label {
                        text: CompositionEditorVm.fragmentCount + " cards"
                        opacity: 0.72
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Item {
                        Layout.preferredWidth: General.spacingLarge
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: General.spacingSmall

                    SvgToolButton {
                        iconSource: Icons.addSvg
                        labelText: "Add Block"
                        compact: true
                        onClicked: CompositionEditorVm.addBlockRef()
                    }

                    SvgToolButton {
                        iconSource: Icons.textSvg
                        labelText: "Add Text"
                        compact: true
                        onClicked: CompositionEditorVm.addStaticText()
                    }

                    SvgToolButton {
                        iconSource: Icons.separatorSvg
                        labelText: "Add Sep"
                        compact: true
                        onClicked: CompositionEditorVm.addSeparator()
                    }

                    SvgToolButton {
                        iconSource: Icons.addSvg
                        labelText: "Insert \\n"
                        compact: true
                        onClicked: CompositionEditorVm.insertNewlinesBetween()
                    }

                    SvgToolButton {
                        iconSource: Icons.applySvg
                        labelText: CompositionEditorVm.insertModeActive ? "Insert" : "Apply"
                        compact: true
                        enabled: CompositionEditorVm.hasSelection
                        onClicked: CompositionEditorVm.applySelected()
                    }

                    SvgToolButton {
                        iconSource: Icons.cutSvg
                        labelText: "Cut"
                        compact: true
                        enabled: CompositionEditorVm.hasSelection
                        onClicked: CompositionEditorVm.cutSelected()
                    }

                    SvgToolButton {
                        iconSource: Icons.pasteSvg
                        labelText: "Paste Before"
                        compact: true
                        enabled: CompositionEditorVm.hasClipboardFragment
                        onClicked: CompositionEditorVm.pasteBeforeSelected()
                    }

                    SvgToolButton {
                        iconSource: Icons.pasteSvg
                        labelText: "Paste After"
                        compact: true
                        enabled: CompositionEditorVm.hasClipboardFragment
                        onClicked: CompositionEditorVm.pasteAfterSelected()
                    }

                    SvgToolButton {
                        iconSource: Icons.removeSvg
                        labelText: "Remove"
                        compact: true
                        enabled: CompositionEditorVm.hasSelection
                        onClicked: CompositionEditorVm.removeSelected()
                    }

                    SvgToolButton {
                        iconSource: Icons.cleanSvg
                        labelText: "Clean Seps"
                        compact: true
                        onClicked: CompositionEditorVm.removeAllSeparators()
                    }

                    SvgToolButton {
                        iconSource: Icons.backSvg
                        labelText: "Back"
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: CompositionEditorVm.closeEditor()
                    }

                    SvgToolButton {
                        iconSource: Icons.saveSvg
                        labelText: CompositionEditorVm.saving ? "Saving..." : CompositionEditorVm.saveButtonText
                        Layout.alignment: Qt.AlignVCenter
                        enabled: !CompositionEditorVm.saving
                        onClicked: CompositionEditorVm.save()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: General.spacingMedium

            CompositionEditorSidebar {
                Layout.preferredWidth: 440
                Layout.minimumWidth: 400
                Layout.maximumWidth: 540
                Layout.fillHeight: true
            }

            Frame {
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

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            spacing: 2

                            Label {
                                text: "Change Set"
                                font.bold: true
                            }

                            Label {
                                text: "Drag cards by the six-dot handle to reorder them."
                                opacity: 0.72
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    ListView {
                        id: fragmentList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: General.spacingSmall
                        model: CompositionEditorVm.fragmentsModel

                        delegate: Item {
                            id: delegateRoot
                            required property int index
                            required property string kind
                            required property string kindLabel
                            required property string title
                            required property string meta
                            required property string body

                            width: ListView.view.width
                            height: card.implicitHeight

                            CompositionFragmentCard {
                                id: card
                                width: delegateRoot.width
                                rowIndex: delegateRoot.index
                                kind: delegateRoot.kind
                                kindLabel: delegateRoot.kindLabel
                                title: delegateRoot.title
                                meta: delegateRoot.meta
                                body: delegateRoot.body
                                dragContainer: fragmentList.contentItem
                                itemCount: fragmentList.count
                                selectedIndex: CompositionEditorVm.selectedFragmentIndex
                                dropPreviewIndex: editorRoot.dropPreviewIndex
                                targetResolver: editorRoot.resolveDropIndex
                                moveCallback: function(from, to) {
                                    CompositionEditorVm.moveFragment(from, to)
                                }
                                dragPreviewCallback: function(value) {
                                    editorRoot.dropPreviewIndex = value
                                }
                                selectCallback: function(value) {
                                    CompositionEditorVm.selectedFragmentIndex = value
                                }
                                cutCallback: function(value) {
                                    CompositionEditorVm.selectedFragmentIndex = value
                                    CompositionEditorVm.cutSelected()
                                }
                                pasteBeforeCallback: function(value) {
                                    CompositionEditorVm.selectedFragmentIndex = value
                                    CompositionEditorVm.pasteBeforeSelected()
                                }
                                pasteAfterCallback: function(value) {
                                    CompositionEditorVm.selectedFragmentIndex = value
                                    CompositionEditorVm.pasteAfterSelected()
                                }
                                removeCallback: function(value) {
                                    CompositionEditorVm.selectedFragmentIndex = value
                                    CompositionEditorVm.removeSelected()
                                }
                                canPaste: CompositionEditorVm.hasClipboardFragment
                            }
                        }

                        footer: Item {
                            width: fragmentList.width
                            height: CompositionEditorVm.fragmentCount === 0 ? 160 : 0
                            visible: CompositionEditorVm.fragmentCount === 0

                            Rectangle {
                                anchors.fill: parent
                                radius: General.radiusMedium
                                color: Qt.alpha(ColorPalette.surface, 0.45)
                                border.color: Qt.alpha(ColorPalette.border, 0.9)
                                border.width: 1
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 6

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "No blocks yet"
                                    font.bold: true
                                }

                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "Use the toolbar to add blocks, text, or separators."
                                    opacity: 0.72
                                }
                            }
                        }
                    }

                    Label {
                        text: CompositionEditorVm.statusText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        opacity: 0.72
                    }
                }
            }
        }
    }
}
