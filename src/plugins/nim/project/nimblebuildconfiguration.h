// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>

namespace Nim {

class NimbleBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;

    NimbleBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    BuildType buildType() const override;

    bool fromMap(const QVariantMap &map) override;

    QVariantMap toMap() const override;

private:
    void setBuildType(BuildType buildType);

    BuildType m_buildType = ProjectExplorer::BuildConfiguration::Unknown;
};

class NimbleBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    NimbleBuildConfigurationFactory();
};

} // Nim
