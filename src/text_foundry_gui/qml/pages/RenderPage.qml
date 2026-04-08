import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Page {
    id: root

    readonly property bool detailsStacked: detailsFrame.width < 1180

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
                spacing: General.spacingMedium

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: General.spacingSmall

                    Label {
                        text: "Compositions"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: "Search compositions..."
                        text: RenderVm.searchText
                        onTextChanged: RenderVm.searchText = text
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
                            text: "Compositions"
                            font.bold: true
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: RenderVm.filteredCompositionIds
                            currentIndex: Math.max(0, RenderVm.filteredCompositionIds.indexOf(RenderVm.selectedCompositionId))

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
                            model: RenderVm.versionEntries

                            delegate: ItemDelegate {
                                required property var modelData

                                width: ListView.view.width
                                padding: General.paddingSmall
                                highlighted: RenderVm.versionText === modelData.version
                                onClicked: RenderVm.versionText = modelData.version

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

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Render"
                        color: ColorPalette.primary
                        font.bold: true
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.renderSvg
                        labelText: "Render"
                        onClicked: RenderVm.render()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.aiAssistSvg
                        labelText: RenderVm.normalizing ? "Normalizing..." : "Normalize"
                        enabled: !RenderVm.normalizing && RenderVm.normalizationAvailable
                        onClicked: RenderVm.normalize()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.aiAssistSvg
                        labelText: "Re-normalize"
                        enabled: !RenderVm.normalizing && RenderVm.normalizationAvailable
                        onClicked: RenderVm.renormalize()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.copySvg
                        labelText: "Copy Render"
                        onClicked: RenderVm.copyRender()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.copySvg
                        labelText: "Copy Raw"
                        onClicked: RenderVm.copyRaw()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.clearSvg
                        labelText: "Clear"
                        onClicked: RenderVm.clear()
                    }

                    SvgToolButton {
                        compact: true
                        iconSource: Icons.reloadSvg
                        labelText: "Reload"
                        onClicked: RenderVm.reload()
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: root.detailsStacked ? 1 : 2
                    rowSpacing: General.spacingMedium
                    columnSpacing: General.spacingMedium

                    Rectangle {
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
                                        label: "Composition"
                                        value: RenderVm.selectedCompositionId
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
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

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.horizontalStretchFactor: 1
                                        spacing: 4

                                        Label {
                                            text: "Preview Mode"
                                            font.bold: true
                                        }

                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: RenderVm.previewModes
                                            currentIndex: Math.max(0, RenderVm.previewModes.indexOf(RenderVm.previewMode))
                                            onActivated: RenderVm.previewMode = currentText
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
                                        Layout.horizontalStretchFactor: 1
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
                                        Layout.horizontalStretchFactor: 1
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
                                        Layout.horizontalStretchFactor: 1
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
                                            text: "How far the rewrite may go. Cosmetic cleanup should usually stay on light."
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
                                        Layout.horizontalStretchFactor: 1
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
                                        Layout.horizontalStretchFactor: 1
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
                                    columns: 2
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
