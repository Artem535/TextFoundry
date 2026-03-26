import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry
import org.kde.syntaxhighlighting

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
            color: root.palette.windowText
            selectedTextColor: root.palette.highlightedText
            selectionColor: root.palette.highlight
        }
    }

    SyntaxHighlighter {
        textEdit: textEdit
        definition: root.definition
        theme: Repository.defaultTheme(ColorPalette.darkTheme
                                       ? Repository.DarkTheme
                                       : Repository.LightTheme)
    }
}
