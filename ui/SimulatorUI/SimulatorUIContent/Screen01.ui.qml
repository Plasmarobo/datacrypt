

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import SimulatorUI

Rectangle {
    id: rectangle
    width: 1024
    height: 640
    color: "#8b8b8b"

    RowLayout {
        id: rowLayout
        Layout.fillHeight: true
        Layout.fillWidth: true
        width: parent.width
        Layout.minimumHeight: 400

        DatacryptColumn {
            id: dc1
            property string btn_label: "A"
        }

        DatacryptColumn {
            id: dc2
            property string btn_label: "B"
        }

        DatacryptColumn {
            id: dc3
            property string btn_label: "C"
        }

        DatacryptColumn {
            id: dc4
            property string btn_label: "D"
        }
    }

    RowLayout {
        id: rowLayout1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: rowLayout.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: 0
        anchors.bottomMargin: 0

        TextArea {
            id: textArea
            Layout.fillHeight: true
            Layout.fillWidth: true
            placeholderText: qsTr("Text Area")
        }
    }
}
