// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "uvtargetdeviceselection.h"
#include "uvtargetdriverselection.h"

#include <baremetal/idebugserverprovider.h>

#include <projectexplorer/runcontrol.h> // for RunWorker

#include <utils/qtcprocess.h>

namespace Utils { class PathChooser; }

namespace BareMetal {
namespace Internal {

namespace Uv {
class DeviceSelection;
class DeviceSelector;
class DriverSelection;
class DriverSelector;
}

// UvscServerProvider

class UvscServerProvider : public IDebugServerProvider
{
public:
    enum ToolsetNumber {
        UnknownToolsetNumber = -1,
        ArmAdsToolsetNumber = 4 // ARM-ADS toolset
    };

    void setToolsIniFile(const Utils::FilePath &toolsIniFile);
    Utils::FilePath toolsIniFile() const;

    void setDeviceSelection(const Uv::DeviceSelection &deviceSelection);
    Uv::DeviceSelection deviceSelection() const;

    void setDriverSelection(const Uv::DriverSelection &driverSelection);
    Uv::DriverSelection driverSelection() const;

    ToolsetNumber toolsetNumber() const;
    QStringList supportedDrivers() const;

    bool operator==(const IDebugServerProvider &other) const override;

    QVariantMap toMap() const override;

    bool aboutToRun(Debugger::DebuggerRunTool *runTool, QString &errorMessage) const final;
    ProjectExplorer::RunWorker *targetRunner(ProjectExplorer::RunControl *runControl) const final;

    bool isValid() const override;
    QString channelString() const final;

    static QString buildDllRegistryKey(const Uv::DriverSelection &driver);
    static QString adjustFlashAlgorithmProperty(const QString &property);

protected:
    explicit UvscServerProvider(const QString &id);
    explicit UvscServerProvider(const UvscServerProvider &other);

    void setToolsetNumber(ToolsetNumber toolsetNumber);
    void setSupportedDrivers(const QStringList &supportedDrivers);

    Utils::FilePath buildProjectFilePath(Debugger::DebuggerRunTool *runTool) const;
    Utils::FilePath buildOptionsFilePath(Debugger::DebuggerRunTool *runTool) const;

    bool fromMap(const QVariantMap &data) override;

    // uVision specific stuff.
    virtual Utils::FilePath projectFilePath(Debugger::DebuggerRunTool *runTool,
                                            QString &errorMessage) const;
    virtual Utils::FilePath optionsFilePath(Debugger::DebuggerRunTool *runTool,
                                            QString &errorMessage) const = 0;

    Utils::FilePath m_toolsIniFile;
    Uv::DeviceSelection m_deviceSelection;
    Uv::DriverSelection m_driverSelection;

    // Note: Don't store it to the map!
    ToolsetNumber m_toolsetNumber = UnknownToolsetNumber;
    QStringList m_supportedDrivers;

    friend class UvscServerProviderConfigWidget;
};

// UvscServerProviderConfigWidget

class UvscServerProviderConfigWidget : public IDebugServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit UvscServerProviderConfigWidget(UvscServerProvider *provider);
    void apply() override;
    void discard() override;

protected:
    void setToolsIniFile(const Utils::FilePath &toolsIniFile);
    Utils::FilePath toolsIniFile() const;
    void setDeviceSelection(const Uv::DeviceSelection &deviceSelection);
    Uv::DeviceSelection deviceSelection() const;
    void setDriverSelection(const Uv::DriverSelection &driverSelection);
    Uv::DriverSelection driverSelection() const;

    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    Utils::PathChooser *m_toolsIniChooser = nullptr;
    Uv::DeviceSelector *m_deviceSelector = nullptr;
    Uv::DriverSelector *m_driverSelector = nullptr;
};

// UvscServerProviderRunner

class UvscServerProviderRunner final : public ProjectExplorer::RunWorker
{
public:
    explicit UvscServerProviderRunner(ProjectExplorer::RunControl *runControl,
                                      const ProjectExplorer::Runnable &runnable);

private:
    void start() final;
    void stop() final;

    Utils::QtcProcess m_process;
};

} // namespace Internal
} // namespace BareMetal
