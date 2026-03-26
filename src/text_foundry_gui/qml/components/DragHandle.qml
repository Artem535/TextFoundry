import QtQuick
import QtQuick.Layouts
import TextFoundry

Item {
    id: root

    implicitWidth: 14
    implicitHeight: 22

    GridLayout {
        anchors.centerIn: parent
        columns: 2
        rowSpacing: 3
        columnSpacing: 3

        Repeater {
            model: 6

            delegate: Rectangle {
                implicitWidth: 3
                implicitHeight: 3
                radius: 2
                color: Qt.alpha(root.palette.windowText, 0.42)
            }
        }
    }
}
