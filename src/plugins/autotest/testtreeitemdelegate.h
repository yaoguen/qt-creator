// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QStyledItemDelegate>

namespace Autotest {
namespace Internal {

class TestTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TestTreeItemDelegate(QObject *parent = nullptr);

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

} // namespace Internal
} // namespace Autotest
