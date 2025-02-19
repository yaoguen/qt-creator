// Copyright (C) 2020 Miklos Marton <martonmiklosqdev@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <utils/outputformatter.h>

#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace VcsBase {

class VcsOutputLineParser : public Utils::OutputLineParser
{
    Q_OBJECT
public:
    VcsOutputLineParser();
    void fillLinkContextMenu(QMenu *menu, const Utils::FilePath &workingDirectory, const QString &href);
    bool handleVcsLink(const Utils::FilePath &workingDirectory, const QString &href);

private:
    Result handleLine(const QString &text, Utils::OutputFormat format) override;

    const QRegularExpression m_regexp;
};

}
