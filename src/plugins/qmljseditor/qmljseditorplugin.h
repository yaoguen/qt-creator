// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Utils { class JsonSchemaManager; }

namespace QmlJSEditor {
class QuickToolBar;
namespace Internal {

class QmlJSQuickFixAssistProvider;

class QmlJSEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlJSEditor.json")

public:
    QmlJSEditorPlugin();
    ~QmlJSEditorPlugin() final;

    static QmlJSQuickFixAssistProvider *quickFixAssistProvider();
    static Utils::JsonSchemaManager *jsonManager();
    static QuickToolBar *quickToolBar();

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;

    class QmlJSEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace QmlJSEditor
