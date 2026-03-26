import QtQuick
import TextFoundry

Text {
    id: root

    property string glyph: ""
    property bool regular: false

    text: glyph
    font.family: regular ? Icons.regularFamily : Icons.solidFamily
    font.pixelSize: General.fontBody
    color: ColorPalette.onSurface
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
}
