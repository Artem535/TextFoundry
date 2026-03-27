import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Page {
    id: root

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
                spacing: General.paddingSmall

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: General.spacingSmall

                    Label {
                        text: "Compositions"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: General.spacingSmall

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.addSvg
                            labelText: "New"
                            onClicked: CompositionEditorVm.openCreateEditor()
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
                            iconSource: Icons.deprecateSvg
                            labelText: "Deprecate"
                            enabled: CompositionsVm.selectedCompositionId.length > 0
                            onClicked: CompositionsVm.deprecateSelected()
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        SvgToolButton {
                            compact: true
                            iconSource: Icons.reloadSvg
                            labelText: "Reload"
                            onClicked: CompositionsVm.reload()
                        }
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: CompositionsVm.compositionIds
                    currentIndex: Math.max(0, CompositionsVm.compositionIds.indexOf(CompositionsVm.selectedCompositionId))

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
                    text: "Details"
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
                            columns: 4
                            rowSpacing: General.spacingMedium
                            columnSpacing: General.spacingLarge

                            DetailField {
                                label: "Id"
                                value: CompositionsVm.selectedCompositionId
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Version"
                                    font.bold: true
                                }

                                ComboBox {
                                    Layout.fillWidth: true
                                    model: CompositionsVm.selectedVersionOptions
                                    enabled: CompositionsVm.selectedVersions.length > 0
                                    currentIndex: Math.max(0, CompositionsVm.selectedVersions.indexOf(CompositionsVm.selectedVersion))
                                    onActivated: CompositionsVm.selectCompositionVersion(CompositionsVm.selectedVersions[currentIndex])
                                }
                            }

                            DetailField {
                                label: "State"
                                value: CompositionsVm.selectedState
                            }

                            DetailField {
                                label: "Fragments"
                                value: CompositionsVm.selectedFragmentCount
                            }
                        }

                        DetailField {
                            label: "Description"
                            value: CompositionsVm.selectedDescription
                            placeholder: "No description"
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
                                labelText: CompositionsVm.normalizing ? "Normalizing..." : "Normalize Composition"
                                enabled: !CompositionsVm.normalizing
                                         && CompositionsVm.normalizationAvailable
                                         && CompositionsVm.selectedCompositionId.length > 0
                                onClicked: CompositionsVm.normalizeSelected()
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: CompositionsVm.normalizationStatusText
                            wrapMode: Text.WordWrap
                            opacity: 0.72
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4

                            Label {
                                text: "Content"
                                font.bold: true
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

                                    delegate: Label {
                                        required property string modelData
                                        width: ListView.view.width
                                        text: modelData
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }

                        Label {
                            text: CompositionsVm.statusText
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            opacity: 0.72
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
}
