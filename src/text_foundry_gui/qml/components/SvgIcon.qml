import QtQuick
import QtQuick.Effects

Item {
    id: root

    property url source
    property color color: "white"
    property int iconWidth: 16
    property int iconHeight: 16

    implicitWidth: iconWidth
    implicitHeight: iconHeight
    width: iconWidth
    height: iconHeight

    Image {
        id: sourceImage
        anchors.fill: parent
        source: root.source
        sourceSize.width: root.iconWidth
        sourceSize.height: root.iconHeight
        fillMode: Image.PreserveAspectFit
        visible: false
        smooth: true
        mipmap: true
    }

    MultiEffect {
        anchors.fill: sourceImage
        source: sourceImage
        colorization: 1.0
        colorizationColor: root.color
    }
}
