import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

ColumnLayout {
    id: control

    property bool showPresets: false
    property int gridColumns: width < 360 ? 1 : 2
    property string tone: ""
    property string tense: ""
    property string targetLanguage: ""
    property string person: ""
    property string rewriteStrength: "light"
    property string audience: ""
    property string locale: ""
    property string terminologyRigidity: "strict"
    property bool preserveFormatting: true
    property bool preserveExamples: true

    signal toneEdited(string value)
    signal tenseEdited(string value)
    signal targetLanguageEdited(string value)
    signal personEdited(string value)
    signal rewriteStrengthEdited(string value)
    signal audienceEdited(string value)
    signal localeEdited(string value)
    signal terminologyRigidityEdited(string value)
    signal preserveFormattingEdited(bool value)
    signal preserveExamplesEdited(bool value)
    signal formalEnglishPresetRequested()
    signal warmRussianPresetRequested()
    signal neutralThirdPresetRequested()
    signal clearStyleRequested()

    spacing: General.spacingMedium

    ColumnLayout {
        Layout.fillWidth: true
        spacing: General.spacingSmall
        visible: control.showPresets

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
                onClicked: control.formalEnglishPresetRequested()
            }

            SvgToolButton {
                compact: true
                iconSource: Icons.aiAssistSvg
                labelText: "Warm RU"
                onClicked: control.warmRussianPresetRequested()
            }

            SvgToolButton {
                compact: true
                iconSource: Icons.aiAssistSvg
                labelText: "Neutral 3rd"
                onClicked: control.neutralThirdPresetRequested()
            }

            SvgToolButton {
                compact: true
                iconSource: Icons.clearSvg
                labelText: "Clear Style"
                onClicked: control.clearStyleRequested()
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: control.gridColumns
        rowSpacing: General.spacingSmall
        columnSpacing: General.spacingLarge

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Tone"
                font.bold: true
            }

            TextField {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.tone
                placeholderText: "formal"
                onTextEdited: control.toneEdited(text)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Tense"
                font.bold: true
            }

            TextField {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.tense
                placeholderText: "present"
                onTextEdited: control.tenseEdited(text)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Target Language"
                font.bold: true
            }

            TextField {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.targetLanguage
                placeholderText: "en"
                onTextEdited: control.targetLanguageEdited(text)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Person"
                font.bold: true
            }

            TextField {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.person
                placeholderText: "second"
                onTextEdited: control.personEdited(text)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Rewrite Strength"
                font.bold: true
            }

            ComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                model: ["light", "medium", "strong"]
                currentIndex: Math.max(0, model.indexOf(control.rewriteStrength))
                onActivated: control.rewriteStrengthEdited(currentText)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Audience"
                font.bold: true
            }

            TextField {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.audience
                placeholderText: "end-user"
                onTextEdited: control.audienceEdited(text)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Locale"
                font.bold: true
            }

            TextField {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: control.locale
                placeholderText: "en-US"
                onTextEdited: control.localeEdited(text)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.horizontalStretchFactor: 1
            spacing: 4

            Label {
                text: "Terminology"
                font.bold: true
            }

            ComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                model: ["strict", "balanced", "flexible"]
                currentIndex: Math.max(0, model.indexOf(control.terminologyRigidity))
                onActivated: control.terminologyRigidityEdited(currentText)
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: control.gridColumns
        rowSpacing: General.spacingSmall
        columnSpacing: General.spacingLarge

        CheckBox {
            text: "Preserve Formatting"
            checked: control.preserveFormatting
            onToggled: control.preserveFormattingEdited(checked)
        }

        CheckBox {
            text: "Preserve Examples"
            checked: control.preserveExamples
            onToggled: control.preserveExamplesEdited(checked)
        }
    }
}
