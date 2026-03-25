import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Page {
    background: Rectangle {
        color: ColorPalette.background
    }

    RowLayout {
        anchors.fill: parent
        spacing: General.spacingLarge

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
                spacing: General.paddingSmall

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Compositions"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Reload"
                        onClicked: RenderVm.reload()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: RenderVm.compositionIds
                    currentIndex: Math.max(0, RenderVm.compositionIds.indexOf(RenderVm.selectedCompositionId))

                    delegate: ItemDelegate {
                        required property string modelData
                        required property int index

                        width: ListView.view.width
                        text: modelData
                        highlighted: ListView.isCurrentItem
                        onClicked: {
                            ListView.view.currentIndex = index
                            RenderVm.selectedCompositionId = modelData
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

                Label {
                    text: "Render"
                    color: ColorPalette.primary
                    font.bold: true
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
                            columns: 2
                            rowSpacing: General.spacingMedium
                            columnSpacing: General.spacingLarge

                            DetailField {
                                label: "Composition"
                                value: RenderVm.selectedCompositionId
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Version"
                                    font.bold: true
                                }

                                TextField {
                                    Layout.fillWidth: true
                                    text: RenderVm.versionText
                                    placeholderText: "latest"
                                    onTextEdited: RenderVm.versionText = text
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: "Runtime Params"
                                font.bold: true
                            }

                            TextArea {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 120
                                text: RenderVm.paramsText
                                placeholderText: "name=value, other=123"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: if (text !== RenderVm.paramsText) RenderVm.paramsText = text
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                text: "Render"
                                onClicked: RenderVm.render()
                            }

                            Button {
                                text: "Copy Render"
                                onClicked: RenderVm.copyRender()
                            }

                            Button {
                                text: "Copy Raw"
                                onClicked: RenderVm.copyRaw()
                            }

                            Button {
                                text: "Clear"
                                onClicked: RenderVm.clear()
                            }

                            Item { Layout.fillWidth: true }
                        }

                        Label {
                            text: RenderVm.statusText
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            opacity: 0.72
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4

                            Label {
                                text: "Output"
                                font.bold: true
                            }

                            CodePreview {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: RenderVm.outputText
                                definition: "Markdown"
                            }
                        }
                    }
                }
            }
        }
    }
}
