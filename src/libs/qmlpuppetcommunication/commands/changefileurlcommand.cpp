// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "changefileurlcommand.h"

#include <QDebug>

namespace QmlDesigner {

QDebug operator <<(QDebug debug, const ChangeFileUrlCommand &command)
{
    return debug.nospace() << "ChangeFileUrlCommand("
                           << "fileUrl: " << command.fileUrl << ")";
}

} // namespace QmlDesigner
