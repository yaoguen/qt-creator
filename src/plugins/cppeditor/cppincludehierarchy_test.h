// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal::Tests {

class IncludeHierarchyTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data();
    void test();
};

} // namespace CppEditor::Internal::Tests
