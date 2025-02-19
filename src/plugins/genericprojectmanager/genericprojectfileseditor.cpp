// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "genericprojectfileseditor.h"
#include "genericprojectconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textdocument.h>

#include <QCoreApplication>

using namespace TextEditor;

namespace GenericProjectManager {
namespace Internal {

//
// ProjectFilesFactory
//

ProjectFilesFactory::ProjectFilesFactory()
{
    setId(Constants::FILES_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", ".files Editor"));
    addMimeType("application/vnd.qtcreator.generic.files");
    addMimeType("application/vnd.qtcreator.generic.includes");
    addMimeType("application/vnd.qtcreator.generic.config");
    addMimeType("application/vnd.qtcreator.generic.cxxflags");
    addMimeType("application/vnd.qtcreator.generic.cflags");

    setDocumentCreator([]() { return new TextDocument(Constants::FILES_EDITOR_ID); });
    setEditorActionHandlers(TextEditorActionHandler::None);
}

} // namespace Internal
} // namespace GenericProjectManager
