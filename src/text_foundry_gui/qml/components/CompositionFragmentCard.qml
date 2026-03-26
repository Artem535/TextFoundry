import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

Item {
    id: root

    required property int rowIndex
    required property int itemCount
    required property string kind
    required property string kindLabel
    required property string title
    required property string meta
    required property string body
    required property int selectedIndex
    required property int dropPreviewIndex
    required property Item dragContainer
    required property var targetResolver
    required property var moveCallback
    required property var dragPreviewCallback
    required property var selectCallback
    required property var cutCallback
    required property var pasteBeforeCallback
    required property var pasteAfterCallback
    required property var removeCallback
    required property bool canPaste

    width: parent ? parent.width : 0
    implicitHeight: cardHost.height
    z: dragArea.drag.active ? 1000 : 1

    Item {
        id: cardHost
        width: parent.width
        height: Math.max(144, cardContent.implicitHeight + General.paddingMedium * 2)
        z: dragArea.drag.active ? 1000 : 0
        scale: dragArea.drag.active ? 1.01 : 1.0
        layer.enabled: dragArea.drag.active

        Rectangle {
            anchors.fill: parent
            radius: General.radiusMedium
            color: root.selectedIndex === root.rowIndex
                   ? Qt.alpha(ColorPalette.selection, 0.14)
                   : ColorPalette.surface
            border.color: root.selectedIndex === root.rowIndex
                          ? ColorPalette.primary
                          : Qt.alpha(ColorPalette.borderStrong, 0.9)
            border.width: 1
        }

        Rectangle {
            visible: root.dropPreviewIndex === root.rowIndex && !dragArea.drag.active
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: -4
            height: 3
            radius: 2
            color: ColorPalette.primary
        }

        RowLayout {
            id: cardContent
            anchors.fill: parent
            anchors.margins: General.paddingMedium
            spacing: General.spacingMedium

            Item {
                Layout.preferredWidth: 32
                Layout.fillHeight: true

                DragHandle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    drag.target: cardHost
                    drag.axis: Drag.YAxis
                    onPressed: root.selectCallback(root.rowIndex)
                    onPositionChanged: {
                        if (!pressed || root.itemCount <= 1) {
                            return
                        }
                        const point = cardHost.mapToItem(root.dragContainer, 0, cardHost.y + cardHost.height / 2)
                        root.dragPreviewCallback(root.targetResolver(point.y))
                    }
                    onReleased: {
                        if (root.itemCount > 1) {
                            const point = cardHost.mapToItem(root.dragContainer, 0, cardHost.y + cardHost.height / 2)
                            const targetIndex = root.targetResolver(point.y)
                            if (targetIndex !== root.rowIndex) {
                                root.moveCallback(root.rowIndex, targetIndex)
                            }
                        }
                        root.dragPreviewCallback(-1)
                        cardHost.y = 0
                    }
                    onCanceled: {
                        root.dragPreviewCallback(-1)
                        cardHost.y = 0
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: General.spacingSmall

                    Label {
                        Layout.fillWidth: true
                        text: root.title
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        radius: General.radiusSmall
                        color: Qt.alpha(ColorPalette.selection, 0.16)
                        border.color: Qt.alpha(ColorPalette.borderStrong, 0.85)
                        implicitWidth: kindText.implicitWidth + 14
                        implicitHeight: kindText.implicitHeight + 8

                        Label {
                            id: kindText
                            anchors.centerIn: parent
                            text: root.kindLabel
                            font.pixelSize: 12
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.meta
                    opacity: 0.72
                    elide: Text.ElideRight
                    font.family: General.monospaceFamily
                    font.pixelSize: Math.max(11, SessionVm.previewFontSize - 1)
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: General.radiusSmall
                    color: Qt.alpha(ColorPalette.fieldBackground, 0.82)
                    border.color: Qt.alpha(ColorPalette.border, 0.9)

                    Label {
                        anchors.fill: parent
                        anchors.margins: General.paddingSmall
                        text: root.body
                        wrapMode: Text.WordWrap
                        verticalAlignment: Text.AlignTop
                        font.family: General.monospaceFamily
                        font.pixelSize: SessionVm.previewFontSize
                    }
                }
            }
        }
    }

    TapHandler {
        onTapped: root.selectCallback(root.rowIndex)
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: function(eventPoint) {
            root.selectCallback(root.rowIndex)
            contextMenu.popup(eventPoint.position.x, eventPoint.position.y)
        }
    }

    Menu {
        id: contextMenu

        MenuItem {
            text: "Cut"
            onTriggered: root.cutCallback(root.rowIndex)
        }

        MenuSeparator {}

        MenuItem {
            text: "Paste Before"
            enabled: root.canPaste
            onTriggered: root.pasteBeforeCallback(root.rowIndex)
        }

        MenuItem {
            text: "Paste After"
            enabled: root.canPaste
            onTriggered: root.pasteAfterCallback(root.rowIndex)
        }

        MenuSeparator {}

        MenuItem {
            text: "Remove"
            onTriggered: root.removeCallback(root.rowIndex)
        }
    }
}
