import QtQuick
import QtQuick.Controls
Button {
    id: control
    implicitHeight: 38
    font.pixelSize: 12
    font.family: "serif"
    contentItem: Text { text: control.text; font: control.font; color: control.enabled ? "#F5DADD" : "#937D80"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
    background: Rectangle {
        radius: 13
        color: control.down ? "#55111A" : (control.hovered ? "#380A11" : "#200609")
        border.color: control.hovered ? "#F06C76" : "#A93A48"
        border.width: 1
        Rectangle { anchors.fill: parent; anchors.margins: 3; radius: 10; color: "transparent"; border.width: 1; border.color: "#704044"; opacity: .55 }
    }
}
