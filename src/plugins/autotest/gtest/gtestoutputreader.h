// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../testoutputreader.h"

namespace Autotest {
namespace Internal {

class GTestOutputReader : public TestOutputReader
{
public:
    GTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                      Utils::QtcProcess *testApplication, const Utils::FilePath &buildDirectory,
                      const Utils::FilePath &projectFile);
protected:
    void processOutputLine(const QByteArray &outputLine) override;
    void processStdError(const QByteArray &outputLine) override;
    TestResultPtr createDefaultResult() const override;

private:
    void setCurrentTestCase(const QString &testCase);
    void setCurrentTestSuite(const QString &testSuite);
    void handleDescriptionAndReportResult(TestResultPtr testResult);

    Utils::FilePath m_projectFile;
    QString m_currentTestSuite;
    QString m_currentTestCase;
    QString m_description;
    int m_iteration = 1;
    bool m_testSetStarted = false;
};

} // namespace Internal
} // namespace Autotest
