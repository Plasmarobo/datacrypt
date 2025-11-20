import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    property string btn_label
    property int index

    id: column
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    Layout.minimumHeight: 400
    Layout.fillWidth: true
    Layout.fillHeight: true
    Image {
        id: img_b
        width: 128
        height: 64
        source: "image://SimDisplay/display_big_" + index
        cache: false
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: img_s
        width: 128
        height: 32
        source: "image://SimDisplay/display_small_" + index
        cache: false
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        fillMode: Image.PreserveAspectFit
    }

    Button {
        id: btn
        text: btn_label
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    }

    Switch {
        id: sw
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    }
}
