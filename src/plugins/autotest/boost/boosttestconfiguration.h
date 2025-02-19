// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../testconfiguration.h"

namespace Autotest {
namespace Internal {

class BoostTestConfiguration : public DebuggableTestConfiguration
{
public:
    explicit BoostTestConfiguration(ITestFramework *framework)
        : DebuggableTestConfiguration(framework) {}
    TestOutputReader *outputReader(const QFutureInterface<TestResultPtr> &fi,
                                   Utils::QtcProcess *app) const override;
    QStringList argumentsForTestRunner(QStringList *omitted = nullptr) const override;
    Utils::Environment filteredEnvironment(const Utils::Environment &original) const override;
};

} // namespace Internal
} // namespace Autotest
