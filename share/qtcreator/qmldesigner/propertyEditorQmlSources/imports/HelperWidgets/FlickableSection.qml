// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    caption: qsTr("Flickable")
    anchors.left: parent.left
    anchors.right: parent.right

    SectionLayout {
        PropertyLabel {
            text: qsTr("Interactive")
            tooltip: qsTr("Allows users to drag or flick a flickable component.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.interactive.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.interactive
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Flick direction")
            blockedByTemplate: !backendValues.flickableDirection.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.flickableDirection
                model: ["AutoFlickDirection", "AutoFlickIfNeeded", "HorizontalFlick", "VerticalFlick", "HorizontalAndVerticalFlick"]
                scope: "Flickable"
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Behavior")
            tooltip: qsTr("Whether the surface may be dragged beyond the Flickable's boundaries, or overshoot the Flickable's boundaries when flicked.")
            blockedByTemplate: !backendValues.boundsBehavior.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.boundsBehavior
                model: ["StopAtBounds", "DragOverBounds", "OvershootBounds", "DragAndOvershootBounds"]
                scope: "Flickable"
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Movement")
            tooltip: qsTr("Whether the Flickable will give a feeling that the edges of the view are soft, rather than a hard physical boundary.")
            blockedByTemplate: !backendValues.boundsMovement.isAvailable
        }

        SecondColumnLayout {
            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                backendValue: backendValues.boundsMovement
                model: ["FollowBoundsBehavior", "StopAtBounds"]
                scope: "Flickable"
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Max. velocity")
            tooltip: qsTr("Maximum flick velocity")
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.maximumFlickVelocity
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Deceleration")
            tooltip: qsTr("Flick deceleration")
            blockedByTemplate: !backendValues.flickDeceleration.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.flickDeceleration
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Press delay")
            tooltip: qsTr("Time to delay delivering a press to children of the Flickable in milliseconds.")
            blockedByTemplate: !backendValues.pressDelay.isAvailable
        }

        SecondColumnLayout {
            SpinBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.pressDelay
                minimumValue: 0
                maximumValue: 2000
                decimals: 0
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Pixel aligned")
            tooltip: qsTr("Sets the alignment of contentX and contentY to pixels (true) or subpixels (false).")
            blockedByTemplate: !backendValues.pixelAligned.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.pixelAligned.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.pixelAligned
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Synchronous drag")
            tooltip: qsTr("If set to true, then when the mouse or touchpoint moves far enough to begin dragging\n"
                          + "the content, the content will jump, such that the content pixel which was under the\n"
                          + "cursor or touchpoint when pressed remains under that point.")
            blockedByTemplate: !backendValues.synchronousDrag.isAvailable
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.synchronousDrag.valueToString
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: backendValues.synchronousDrag
                enabled: backendValue.isAvailable
            }

            ExpandingSpacer {}
        }

    }
}
