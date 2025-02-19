// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "gerritserver.h"

#include <utils/filepath.h>

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace Gerrit {
namespace Internal {

class GerritParameters
{
public:
    GerritParameters();

    bool isValid() const;
    bool equals(const GerritParameters &rhs) const;
    void toSettings(QSettings *) const;
    void saveQueries(QSettings *) const;
    void fromSettings(const QSettings *);
    void setPortFlagBySshType();

    friend bool operator==(const GerritParameters &p1, const GerritParameters &p2)
    { return p1.equals(p2); }
    friend bool operator!=(const GerritParameters &p1, const GerritParameters &p2)
    { return !p1.equals(p2); }

    GerritServer server;
    Utils::FilePath ssh;
    Utils::FilePath curl;
    QStringList savedQueries;
    bool https = true;
    QString portFlag;
};

} // namespace Internal
} // namespace Gerrit
