pragma Singleton

import QtQuick

Item {
    id: root
    visible: false

    readonly property string solidFamily: solidLoader.name
    readonly property string regularFamily: regularLoader.name
    readonly property string brandsFamily: brandsLoader.name

    readonly property url blocksSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/cubes.svg"
    readonly property url compositionsSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/layer-group.svg"
    readonly property url renderSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/eye.svg"
    readonly property url settingsSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/gear.svg"
    readonly property url backSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/arrow-left.svg"
    readonly property url saveSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/floppy-disk.svg"
    readonly property url addSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/plus.svg"
    readonly property url editSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/edit.svg"
    readonly property url reloadSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/arrows-rotate.svg"
    readonly property url deprecateSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/ban.svg"
    readonly property url copySvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/copy.svg"
    readonly property url clearSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/eraser.svg"
    readonly property url closeSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/xmark.svg"
    readonly property url textSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/font.svg"
    readonly property url separatorSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/grip-lines.svg"
    readonly property url applySvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/check.svg"
    readonly property url cutSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/scissors.svg"
    readonly property url pasteSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/paste.svg"
    readonly property url removeSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/trash.svg"
    readonly property url cleanSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/broom.svg"
    readonly property url broomSvg: cleanSvg
    readonly property url aiAssistSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/wand-magic-sparkles.svg"
    readonly property url sliceSvg: "qrc:/qt/qml/TextFoundry/resource/icons/solid/scissors.svg"

    readonly property string blocks: "\uf1b3"
    readonly property string compositions: "\uf0e8"
    readonly property string render: "\uf06e"
    readonly property string settings: "\uf013"
    readonly property string back: "\uf060"
    readonly property string save: "\uf0c7"

    FontLoader {
        id: solidLoader
        source: "qrc:/qt/qml/TextFoundry/resource/font/Font Awesome 7 Free-Solid-900.otf"
    }

    FontLoader {
        id: regularLoader
        source: "qrc:/qt/qml/TextFoundry/resource/font/Font Awesome 7 Free-Regular-400.otf"
    }

    FontLoader {
        id: brandsLoader
        source: "qrc:/qt/qml/TextFoundry/resource/font/Font Awesome 7 Brands-Regular-400.otf"
    }
}
