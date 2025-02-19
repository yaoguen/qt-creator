// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "googletest.h"

#include <refactoringclientinterface.h>

#include <QTextCharFormat>

class MockSyntaxHighlighter
{
public:
    MOCK_METHOD3(setFormat,
                 void (int start, int count, const QTextCharFormat &format));
};
