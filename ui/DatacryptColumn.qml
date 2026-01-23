import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.millibyte.displayview 1.0

ColumnLayout {
    property string btn_label
    property int index

    id: column
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    Layout.minimumHeight: 400
    Layout.fillWidth: true
    Layout.fillHeight: true

    signal pressed(bool value)
    signal switched(bool value)

    DisplayView {
        objectName: "image_b"
        width: 128
        height: 64
    }

    DisplayView {
        objectName: "image_s"
        width: 128
        height: 32
    }

    Button {
        id: btn
        text: btn_label
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

        onPressed: {column.pressed(true)}
        onReleased: {column.pressed(false)}
    }

    Switch {
        id: sw
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

        onCheckedChanged: {column.switched(checked)}
    }
}
