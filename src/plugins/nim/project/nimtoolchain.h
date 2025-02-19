// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>

namespace Nim {

class NimToolChain : public ProjectExplorer::ToolChain
{
public:
    NimToolChain();
    explicit NimToolChain(Utils::Id typeId);

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Utils::LanguageExtensions languageExtensions(const QStringList &flags) const final;
    Utils::WarningFlags warningFlags(const QStringList &flags) const final;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &) const override;
    void addToEnvironment(Utils::Environment &env) const final;
    Utils::FilePath makeCommand(const Utils::Environment &env) const final;
    QString compilerVersion() const;
    QList<Utils::OutputLineParser *> createOutputParsers() const final;
    std::unique_ptr<ProjectExplorer::ToolChainConfigWidget> createConfigurationWidget() final;

    bool fromMap(const QVariantMap &data) final;

    static bool parseVersion(const Utils::FilePath &path, std::tuple<int, int, int> &version);

private:
    std::tuple<int, int, int> m_version;
};

} // Nim
