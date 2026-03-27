import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Page {
    id: root

    readonly property bool renderStacked: contentFrame.width < 1150

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

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.reloadSvg
                        labelText: "Reload"
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
            id: contentFrame
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

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: root.renderStacked ? 1 : 2
                    rowSpacing: General.spacingMedium
                    columnSpacing: General.spacingMedium

                    ScrollView {
                        id: paramsPane
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: root.renderStacked ? -1 : 520
                        Layout.minimumWidth: root.renderStacked ? 0 : 460
                        Layout.maximumWidth: root.renderStacked ? Number.POSITIVE_INFINITY : 580
                        Layout.preferredHeight: root.renderStacked ? 520 : -1
                        clip: true

                        ColumnLayout {
                            width: root.renderStacked
                                   ? Math.max(0, paramsPane.availableWidth)
                                   : Math.max(paramsPane.availableWidth, 440)
                            spacing: General.spacingMedium

                            Rectangle {
                                id: paramsCard
                                Layout.fillWidth: true
                                radius: General.radiusSmall
                                color: ColorPalette.fieldBackground
                                border.color: ColorPalette.borderStrong
                                implicitHeight: controlsColumn.implicitHeight + General.paddingMedium * 2

                                ColumnLayout {
                                    id: controlsColumn
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: General.paddingMedium
                                    spacing: General.spacingMedium

                                    GridLayout {
                                        Layout.fillWidth: true
                                        columns: root.renderStacked ? 1 : 2
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

                                    GridLayout {
                                        Layout.fillWidth: true
                                        columns: root.renderStacked ? 1 : 2
                                        rowSpacing: General.spacingMedium
                                        columnSpacing: General.spacingLarge

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Label {
                                                text: "Tone"
                                                font.bold: true
                                            }

                                            TextField {
                                                Layout.fillWidth: true
                                                text: RenderVm.tone
                                                placeholderText: "formal"
                                                onTextEdited: RenderVm.tone = text
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
                                            spacing: 4

                                            Label {
                                                text: "Tense"
                                                font.bold: true
                                            }

                                            TextField {
                                                Layout.fillWidth: true
                                                text: RenderVm.tense
                                                placeholderText: "present"
                                                onTextEdited: RenderVm.tense = text
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: "Verb tense to prefer in the rewrite: present, past, future."
                                                wrapMode: Text.WordWrap
                                                opacity: 0.72
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Label {
                                                text: "Target Language"
                                                font.bold: true
                                            }

                                            TextField {
                                                Layout.fillWidth: true
                                                text: RenderVm.targetLanguage
                                                placeholderText: "en"
                                                onTextEdited: RenderVm.targetLanguage = text
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: "Language for the rewritten text, for example en or ru."
                                                wrapMode: Text.WordWrap
                                                opacity: 0.72
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Label {
                                                text: "Person"
                                                font.bold: true
                                            }

                                            TextField {
                                                Layout.fillWidth: true
                                                text: RenderVm.person
                                                placeholderText: "second"
                                                onTextEdited: RenderVm.person = text
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
                                            spacing: 4

                                            Label {
                                                text: "Rewrite Strength"
                                                font.bold: true
                                            }

                                            ComboBox {
                                                Layout.fillWidth: true
                                                model: ["light", "medium", "strong"]
                                                currentIndex: Math.max(0, model.indexOf(RenderVm.rewriteStrength))
                                                onActivated: RenderVm.rewriteStrength = currentText
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: "How far the rewrite may go. For cosmetic cleanup this should usually stay on light."
                                                wrapMode: Text.WordWrap
                                                opacity: 0.72
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Label {
                                                text: "Audience"
                                                font.bold: true
                                            }

                                            TextField {
                                                Layout.fillWidth: true
                                                text: RenderVm.audience
                                                placeholderText: "end-user"
                                                onTextEdited: RenderVm.audience = text
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: "Who the rewritten text is for: end-user, developer, executive."
                                                wrapMode: Text.WordWrap
                                                opacity: 0.72
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Label {
                                                text: "Locale"
                                                font.bold: true
                                            }

                                            TextField {
                                                Layout.fillWidth: true
                                                text: RenderVm.locale
                                                placeholderText: "en-US"
                                                onTextEdited: RenderVm.locale = text
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
                                            spacing: 4

                                            Label {
                                                text: "Terminology"
                                                font.bold: true
                                            }

                                            ComboBox {
                                                Layout.fillWidth: true
                                                model: ["strict", "balanced", "flexible"]
                                                currentIndex: Math.max(0, model.indexOf(RenderVm.terminologyRigidity))
                                                onActivated: RenderVm.terminologyRigidity = currentText
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
                                        columns: root.renderStacked ? 1 : 2
                                        rowSpacing: General.spacingMedium
                                        columnSpacing: General.spacingLarge

                                        CheckBox {
                                            text: "Preserve Formatting"
                                            checked: RenderVm.preserveFormatting
                                            onToggled: RenderVm.preserveFormatting = checked
                                        }

                                        CheckBox {
                                            text: "Preserve Examples"
                                            checked: RenderVm.preserveExamples
                                            onToggled: RenderVm.preserveExamples = checked
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: General.spacingSmall

                                        Label {
                                            text: "Preview Mode"
                                            font.bold: true
                                        }

                                        ComboBox {
                                            Layout.fillWidth: root.renderStacked
                                            Layout.preferredWidth: root.renderStacked ? -1 : 220
                                            model: RenderVm.previewModes
                                            currentIndex: Math.max(0, RenderVm.previewModes.indexOf(RenderVm.previewMode))
                                            onActivated: RenderVm.previewMode = currentText
                                        }
                                    }

                                    Flow {
                                        Layout.fillWidth: true
                                        spacing: General.spacingSmall

                                        SvgToolButton {
                                            iconSource: Icons.renderSvg
                                            labelText: "Render"
                                            onClicked: RenderVm.render()
                                        }

                                        SvgToolButton {
                                            iconSource: Icons.aiAssistSvg
                                            labelText: RenderVm.normalizing ? "Normalizing..." : "Normalize"
                                            enabled: !RenderVm.normalizing
                                                     && RenderVm.normalizationAvailable
                                            onClicked: RenderVm.normalize()
                                        }

                                        SvgToolButton {
                                            iconSource: Icons.aiAssistSvg
                                            labelText: "Re-normalize"
                                            enabled: !RenderVm.normalizing
                                                     && RenderVm.normalizationAvailable
                                            onClicked: RenderVm.renormalize()
                                        }

                                        SvgToolButton {
                                            iconSource: Icons.copySvg
                                            labelText: "Copy Render"
                                            onClicked: RenderVm.copyRender()
                                        }

                                        SvgToolButton {
                                            iconSource: Icons.copySvg
                                            labelText: "Copy Raw"
                                            onClicked: RenderVm.copyRaw()
                                        }

                                        SvgToolButton {
                                            iconSource: Icons.clearSvg
                                            labelText: "Clear"
                                            onClicked: RenderVm.clear()
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: General.spacingSmall

                                        Label {
                                            text: "Status"
                                            font.bold: true
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: RenderVm.statusText
                                            wrapMode: Text.WordWrap
                                            color: ColorPalette.textPrimary
                                            opacity: 0.72
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: General.spacingSmall

                                        Label {
                                            text: "Normalization"
                                            font.bold: true
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: RenderVm.normalizationStatusText
                                            wrapMode: Text.WordWrap
                                            color: RenderVm.normalizationStale
                                                   ? ColorPalette.primary
                                                   : ColorPalette.textPrimary
                                            opacity: RenderVm.normalizationStale ? 1.0 : 0.72
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: root.renderStacked ? 0 : 420
                        Layout.preferredHeight: root.renderStacked ? 520 : -1
                        radius: General.radiusSmall
                        color: ColorPalette.fieldBackground
                        border.color: ColorPalette.borderStrong

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: General.paddingMedium
                            spacing: 4

                            Label {
                                text: "Preview"
                                font.bold: true
                            }

                            CodePreview {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: RenderVm.displayedOutputText
                                definition: "Markdown"
                            }
                        }
                    }
                }
            }
        }
    }
}
