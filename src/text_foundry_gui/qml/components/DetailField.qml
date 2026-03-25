import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TextFoundry

ColumnLayout {
    id: root

    property string label: ""
    property string value: ""
    property string placeholder: "—"
    property bool fillWidth: true

    Layout.fillWidth: fillWidth
    spacing: 4

    Label {
        text: root.label
        font.bold: true
    }

    Label {
        text: root.value.length > 0 ? root.value : root.placeholder
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        opacity: root.value.length > 0 ? 1.0 : 0.72
    }
}
