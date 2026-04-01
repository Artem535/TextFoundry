import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Frame {
    padding: 0

    background: Rectangle {
        radius: General.radiusMedium
        color: ColorPalette.fieldBackground
        border.color: ColorPalette.border
    }

    ScrollView {
        id: metadataScroll
        anchors.fill: parent
        clip: true
        padding: General.paddingMedium
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Item {
            width: metadataScroll.availableWidth
            implicitHeight: metadataColumn.implicitHeight
            clip: true

            ColumnLayout {
                id: metadataColumn
                width: parent.width
                spacing: General.spacingMedium

                Label {
                    text: "Metadata"
                    font.bold: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    width: parent.width
                    spacing: 4

                    Label {
                        text: "Id"
                        font.bold: true
                    }

                    TextField {
                        Layout.fillWidth: true
                        text: CompositionEditorVm.compositionId
                        readOnly: !CompositionEditorVm.createMode
                        placeholderText: "namespace.composition_id"
                        onTextEdited: CompositionEditorVm.compositionId = text
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    width: parent.width
                    spacing: 4
                    visible: !CompositionEditorVm.createMode

                    Label {
                        text: "Base Version"
                        font.bold: true
                    }

                    Label {
                        text: CompositionEditorVm.currentVersion.length > 0 ? CompositionEditorVm.currentVersion : "Unpublished"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    width: parent.width
                    spacing: 4

                    Label {
                        text: "Revision Comment"
                        font.bold: true
                    }

                    TextArea {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        text: CompositionEditorVm.revisionComment
                        placeholderText: CompositionEditorVm.createMode
                                         ? "Optional note for the first published version"
                                         : "What changed in this new version?"
                        wrapMode: TextEdit.Wrap
                        onTextChanged: if (text !== CompositionEditorVm.revisionComment) CompositionEditorVm.revisionComment = text
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    width: parent.width
                    spacing: 4

                    Label {
                        text: "Version Bump"
                        font.bold: true
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        model: CompositionEditorVm.bumpOptions
                        currentIndex: Math.max(0, CompositionEditorVm.bumpOptions.indexOf(CompositionEditorVm.bumpMode))
                        onActivated: CompositionEditorVm.bumpMode = currentText
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    width: parent.width
                    spacing: 4

                    Label {
                        text: "Description"
                        font.bold: true
                    }

                    TextArea {
                        Layout.fillWidth: true
                        height: 96
                        text: CompositionEditorVm.description
                        wrapMode: TextEdit.Wrap
                        onTextChanged: if (text !== CompositionEditorVm.description) CompositionEditorVm.description = text
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    width: parent.width
                    spacing: General.spacingSmall

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Selected Fragment"
                            font.bold: true
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Switch {
                            checked: CompositionEditorVm.insertModeActive
                            text: "Create"
                            enabled: CompositionEditorVm.hasSelection
                            onToggled: {
                                if (checked) {
                                    CompositionEditorVm.beginInsertAfter()
                                } else {
                                    CompositionEditorVm.cancelInsertMode()
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        opacity: 0.72
                        text: CompositionEditorVm.insertModeLabel
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: General.spacingSmall
                        visible: CompositionEditorVm.insertModeActive

                        ToolButton {
                            text: "Before"
                            checkable: true
                            checked: CompositionEditorVm.insertModeLabel.indexOf("before") !== -1
                            onClicked: CompositionEditorVm.beginInsertBefore()
                        }

                        ToolButton {
                            text: "After"
                            checkable: true
                            checked: CompositionEditorVm.insertModeLabel.indexOf("after") !== -1
                            onClicked: CompositionEditorVm.beginInsertAfter()
                        }

                        ToolButton {
                            text: "Cancel"
                            onClicked: CompositionEditorVm.cancelInsertMode()
                        }
                    }

                    Frame {
                        Layout.fillWidth: true
                        padding: 0
                        background: Rectangle {
                            radius: General.radiusSmall
                            color: ColorPalette.surface
                            border.color: ColorPalette.border
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Repeater {
                                model: CompositionEditorVm.editorModes

                                delegate: ToolButton {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    implicitWidth: 0
                                    checkable: true
                                    checked: CompositionEditorVm.editorMode === modelData
                                    text: modelData
                                    font.pixelSize: 12
                                    leftPadding: 8
                                    rightPadding: 8
                                    onClicked: CompositionEditorVm.editorMode = modelData
                                }
                            }
                        }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        width: parent.width
                        currentIndex: Math.max(0, CompositionEditorVm.editorModes.indexOf(CompositionEditorVm.editorMode))

                        ColumnLayout {
                            Layout.fillWidth: true
                            width: parent.width
                            spacing: General.spacingSmall

                            TextField {
                                Layout.fillWidth: true
                                text: CompositionEditorVm.blockSearchText
                                placeholderText: "Search block id"
                                onTextEdited: CompositionEditorVm.blockSearchText = text
                            }

                            TextField {
                                Layout.fillWidth: true
                                readOnly: true
                                text: CompositionEditorVm.blockRefBlockId
                                placeholderText: "Selected block id"
                            }

                            Frame {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 160
                                padding: 0
                                background: Rectangle {
                                    radius: General.radiusSmall
                                    color: ColorPalette.surface
                                    border.color: ColorPalette.border
                                }

                                ListView {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    clip: true
                                    model: CompositionEditorVm.filteredBlockIds

                                    delegate: ItemDelegate {
                                        required property string modelData

                                        width: ListView.view.width
                                        text: modelData
                                        highlighted: CompositionEditorVm.blockRefBlockId === modelData
                                        onClicked: CompositionEditorVm.blockRefBlockId = modelData
                                    }
                                }
                            }

                            TextField {
                                Layout.fillWidth: true
                                text: CompositionEditorVm.blockRefVersion
                                placeholderText: "1.0"
                                onTextEdited: CompositionEditorVm.blockRefVersion = text
                            }

                            TextArea {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 120
                                text: CompositionEditorVm.blockRefParams
                                placeholderText: "name=value, lang=en"
                                wrapMode: TextEdit.Wrap
                                font.family: General.monospaceFamily
                                font.pixelSize: SessionVm.previewFontSize
                                onTextChanged: if (text !== CompositionEditorVm.blockRefParams) CompositionEditorVm.blockRefParams = text
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            width: parent.width
                            spacing: General.spacingSmall

                            TextArea {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 220
                                text: CompositionEditorVm.staticText
                                placeholderText: "Static text block"
                                wrapMode: TextEdit.Wrap
                                font.family: General.monospaceFamily
                                font.pixelSize: SessionVm.previewFontSize
                                onTextChanged: if (text !== CompositionEditorVm.staticText) CompositionEditorVm.staticText = text
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            width: parent.width
                            spacing: General.spacingSmall

                            ComboBox {
                                Layout.fillWidth: true
                                model: CompositionEditorVm.separatorOptions
                                currentIndex: Math.max(0, CompositionEditorVm.separatorOptions.indexOf(CompositionEditorVm.separatorType))
                                onActivated: CompositionEditorVm.separatorType = currentText
                            }

                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                opacity: 0.72
                                text: "Separators define structural spacing between neighboring blocks."
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    width: parent.width
                    wrapMode: Text.WordWrap
                    opacity: 0.72
                    text: CompositionEditorVm.hasSelection
                          ? (CompositionEditorVm.insertModeActive
                               ? "Set the new fragment below and press Insert."
                               : "Apply updates to the selected card or drag cards by the six-dot handle.")
                          : "Add cards from the toolbar, then drag them into the right order."
                }
            }
        }
    }
}
