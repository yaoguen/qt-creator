// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakecommandbuilder.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>

#include <utils/qtcprocess.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h> // Compile-time only

#include <QRegularExpression>
#include <QStandardPaths>

using namespace ProjectExplorer;
using namespace Utils;

namespace IncrediBuild {
namespace Internal {

QList<Utils::Id> CMakeCommandBuilder::migratableSteps() const
{
    return {CMakeProjectManager::Constants::CMAKE_BUILD_STEP_ID};
}

FilePath CMakeCommandBuilder::defaultCommand() const
{
    const QString defaultCMake = "cmake";
    const QString cmake = QStandardPaths::findExecutable(defaultCMake);
    return FilePath::fromString(cmake.isEmpty() ? defaultCMake : cmake);
}

QString CMakeCommandBuilder::defaultArguments() const
{
    // Build folder or "."
    QString buildDir;
    BuildConfiguration *buildConfig = buildStep()->buildConfiguration();
    if (buildConfig)
        buildDir = buildConfig->buildDirectory().toString();

    if (buildDir.isEmpty())
        buildDir = ".";

    return Utils::ProcessArgs::joinArgs({"--build", buildDir, "--target", "all"});
}

QString CMakeCommandBuilder::setMultiProcessArg(QString args)
{
    QRegularExpression regExp("\\s*\\-j\\s+\\d+");
    args.remove(regExp);
    args.append(" -- -j 200");

    return args;
}

} // namespace Internal
} // namespace IncrediBuild
