import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Frame {
    id: root

    property string text: ""
    property string definition: "Markdown"

    padding: General.paddingMedium

    background: Rectangle {
        radius: General.radiusSmall
        color: ColorPalette.fieldBackground
        border.color: ColorPalette.borderStrong
    }

    ScrollView {
        id: codeScroll
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        TextEdit {
            id: textEdit
            width: codeScroll.availableWidth
            text: root.text
            readOnly: true
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            font.family: General.monospaceFamily
            font.pixelSize: SessionVm.previewFontSize
            color: syntaxHighlighter.textColor.valid
                   ? syntaxHighlighter.textColor
                   : ColorPalette.textPrimary
            selectedTextColor: syntaxHighlighter.selectedTextColor.valid
                               ? syntaxHighlighter.selectedTextColor
                               : ColorPalette.textPrimary
            selectionColor: syntaxHighlighter.selectionColor.valid
                            ? syntaxHighlighter.selectionColor
                            : root.palette.highlight
        }
    }

    SyntaxHighlighter {
        id: syntaxHighlighter
        textEdit: textEdit
        definition: root.definition
        darkTheme: ColorPalette.darkTheme
    }
}
