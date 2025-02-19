// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "modeleditorfactory.h"

#include "modeleditor_constants.h"
#include "actionhandler.h"
#include "modeleditor.h"

#include <QCoreApplication>

namespace ModelEditor {
namespace Internal {

ModelEditorFactory::ModelEditorFactory(UiController *uiController, ActionHandler *actionHandler)
{
    setId(Constants::MODEL_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::MODEL_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::MIME_TYPE_MODEL);
    setEditorCreator([uiController, actionHandler] { return new ModelEditor(uiController, actionHandler); });
}

} // namespace Internal
} // namespace ModelEditor
