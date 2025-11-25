import QtQuick
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: mainScreen
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
            id: dc0
            objectName: "dc0"
        }

        DatacryptColumn {
            id: dc1
            objectName: "dc1"
        }

        DatacryptColumn {
            id: dc2
            objectName: "dc2"
        }

        DatacryptColumn {
            id: dc3
            objectName: "dc3"
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
            id: textOutput
            objectName: "textOutput"
            Layout.fillHeight: true
            Layout.fillWidth: true
            readOnly: true
            placeholderText: qsTr("")
        }
    }
}
