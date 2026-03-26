pragma Singleton
import QtQuick

Item {
    visible: false

    readonly property string monospaceFamily: firaCodeLoader.name

    property int windowWidth: 1440
    property int windowHeight: 920

    property int pageMargin: 20
    property int sectionMargin: 16

    property int spacingSmall: 8
    property int spacingMedium: 12
    property int spacingLarge: 16

    property int paddingSmall: 10
    property int paddingMedium: 12
    property int paddingLarge: 14

    property int headerHeight: 52
    property int headerPadding: 12
    property int footerPadding: 10

    property int radiusSmall: 8
    property int radiusMedium: 10

    property int fontSmall: 18
    property int fontBody: 16
    property int fontMedium: 20
    property int fontLarge: 26

    property int blocksTreeWidth: 420
    property int sidebarWidth: 208
    property int treeIndent: 18

    FontLoader {
        id: firaCodeLoader
        source: "qrc:/qt/qml/TextFoundry/resource/font/FiraCodeNerdFont-Regular.ttf"
    }
}
