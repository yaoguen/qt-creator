// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    id: myRect
    width: 200
    height: 200
    Text {
        id: textElement
        x: 66
        y: 93
        text: "Base State"
    }
    states: [
        State {
            name: "State1"
            PropertyChanges {
                target: myRect
                color: "blue"
            }
            PropertyChanges {
                target: text
                text: "State1"
            }
        },
        State {
            name: "State2"
            PropertyChanges {
                target: myRect
                color: "gray"
            }
            PropertyChanges {
                target: text
                text: "State2"
            }
        }
    ]

    Image {
        id: image1
        x: 41
        y: 46
        source: "images/qtcreator.png"
    }
}
