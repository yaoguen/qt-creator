// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace TextEditor {

class HighlighterSettings;

class HighlighterSettingsPage final : public Core::IOptionsPage
{
public:
    HighlighterSettingsPage();
    ~HighlighterSettingsPage() override;

    QWidget *widget() override;
    void apply() override;
    void finish() override;

    const HighlighterSettings &highlighterSettings() const;

private:
    class HighlighterSettingsPagePrivate *d;
};

} // TextEditor
