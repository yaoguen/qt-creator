// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mesontools.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

namespace MesonProjectManager {
namespace Internal {

class NinjaToolKitAspect final : public ProjectExplorer::KitAspect
{
public:
    NinjaToolKitAspect();

    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *k) const final;
    void setup(ProjectExplorer::Kit *k) final;
    void fix(ProjectExplorer::Kit *k) final;
    ItemList toUserOutput(const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *) const final;

    static void setNinjaTool(ProjectExplorer::Kit *kit, Utils::Id id);
    static Utils::Id ninjaToolId(const ProjectExplorer::Kit *kit);

    static inline decltype(auto) ninjaTool(const ProjectExplorer::Kit *kit)
    {
        return MesonTools::ninjaWrapper(NinjaToolKitAspect::ninjaToolId(kit));
    }
    static inline bool isValid(const ProjectExplorer::Kit *kit)
    {
        auto tool = ninjaTool(kit);
        return (tool && tool->isValid());
    }
};

} // namespace Internal
} // namespace MesonProjectManager
