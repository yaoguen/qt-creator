// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import QtQuick.Controls

import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    signal showContextMenu()

    visible: modelData.bundleMaterialVisible

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton && !materialsModel.importerRunning)
                rootView.startDragMaterial(modelData, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Item { width: 1; height: 5 } // spacer

        Image {
            id: img

            width: root.width - 10
            height: img.width
            anchors.horizontalCenter: parent.horizontalCenter
            source: modelData.bundleMaterialIcon
            cache: false

            Rectangle { // circular indicator for imported bundle materials
                width: 10
                height: 10
                radius: 5
                anchors.right: img.right
                anchors.top: img.top
                anchors.margins: 5
                color: "#00ff00"
                border.color: "#555555"
                border.width: 1
                visible: modelData.bundleMaterialImported

                ToolTip {
                    visible: indicatorMouseArea.containsMouse
                    text: qsTr("Material is imported to project")
                    delay: 1000
                }

                MouseArea {
                    id: indicatorMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }

            IconButton {
                icon: StudioTheme.Constants.plus
                tooltip: qsTr("Add an instance to project")
                buttonSize: 22
                property color c: StudioTheme.Values.themeIconColor
                normalColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .2)
                hoverColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .3)
                pressColor: Qt.hsla(c.hslHue, c.hslSaturation, c.hslLightness, .4)
                anchors.right: img.right
                anchors.bottom: img.bottom
                enabled: !materialsModel.importerRunning

                onClicked: {
                    materialsModel.addToProject(modelData)
                }
            }
        }

        TextInput {
            id: matName

            text: modelData.bundleMaterialName

            width: img.width
            clip: true
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: TextInput.AlignHCenter

            font.pixelSize: StudioTheme.Values.myFontSize

            readOnly: true
            selectByMouse: !matName.readOnly

            color: StudioTheme.Values.themeTextColor
            selectionColor: StudioTheme.Values.themeTextSelectionColor
            selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor
        }
    }
}
