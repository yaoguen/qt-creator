// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/icodestylepreferencesfactory.h>

namespace Nim {

class NimCodeStylePreferencesFactory : public TextEditor::ICodeStylePreferencesFactory
{
public:
    NimCodeStylePreferencesFactory();

    Utils::Id languageId() override;
    QString displayName() override;
    TextEditor::ICodeStylePreferences *createCodeStyle() const override;
    TextEditor::CodeStyleEditorWidget *createEditor(TextEditor::ICodeStylePreferences *settings,
                                                    ProjectExplorer::Project *project,
                                                    QWidget *parent) const override;
    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override;
    QString snippetProviderGroupId() const override;
    QString previewText() const override;
};

} // namespace Nim
