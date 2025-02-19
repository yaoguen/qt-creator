// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cpplocalrenaming.h"

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/fontsettings.h>

#include <utils/qtcassert.h>

/*!
    \class CppEditor::Internal::CppLocalRenaming
    \brief A helper class of CppEditorWidget that implements renaming local usages.

    \internal

    Local use selections must be first set/updated with updateLocalUseSelections().
    Afterwards the local renaming can be started with start(). The CppEditorWidget
    can then delegate work related to the local renaming mode to the handle*
    functions.

    \sa CppEditor::Internal::CppEditorWidget
 */

namespace {

void modifyCursorSelection(QTextCursor &cursor, int position, int anchor)
{
    cursor.setPosition(anchor);
    cursor.setPosition(position, QTextCursor::KeepAnchor);
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

CppLocalRenaming::CppLocalRenaming(TextEditor::TextEditorWidget *editorWidget)
    : m_editorWidget(editorWidget)
    , m_modifyingSelections(false)
    , m_renameSelectionChanged(false)
    , m_firstRenameChangeExpected(false)
{
    forgetRenamingSelection();
}

void CppLocalRenaming::updateSelectionsForVariableUnderCursor(
        const QList<QTextEdit::ExtraSelection> &selections)
{
    if (isActive())
        return;

    m_selections = selections;
}

bool CppLocalRenaming::start()
{
    stop();

    if (findRenameSelection(m_editorWidget->textCursor().position())) {
        updateRenamingSelectionFormat(textCharFormat(TextEditor::C_OCCURRENCES_RENAME));
        m_firstRenameChangeExpected = true;
        updateEditorWidgetWithSelections();
        return true;
    }

    return false;
}

bool CppLocalRenaming::handlePaste()
{
    if (!isActive())
        return false;

    startRenameChange();
    m_editorWidget->TextEditorWidget::paste();
    finishRenameChange();
    return true;
}

bool CppLocalRenaming::handleCut()
{
    if (!isActive())
        return false;

    startRenameChange();
    m_editorWidget->TextEditorWidget::cut();
    finishRenameChange();
    return true;
}

bool CppLocalRenaming::handleSelectAll()
{
    if (!isActive())
        return false;

    QTextCursor cursor = m_editorWidget->textCursor();
    if (!isWithinRenameSelection(cursor.position()))
        return false;

    modifyCursorSelection(cursor, renameSelectionBegin(), renameSelectionEnd());
    m_editorWidget->setTextCursor(cursor);
    return true;
}

bool CppLocalRenaming::isActive() const
{
    return m_renameSelectionIndex != -1;
}

bool CppLocalRenaming::handleKeyPressEvent(QKeyEvent *e)
{
    if (!isActive())
        return false;

    QTextCursor cursor = m_editorWidget->textCursor();
    const int cursorPosition = cursor.position();
    const QTextCursor::MoveMode moveMode = (e->modifiers() & Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;

    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
        stop();
        e->accept();
        return true;
    case Qt::Key_Home: {
        // Send home to start of name when within the name and not at the start
        if (renameSelectionBegin() < cursorPosition  && cursorPosition <= renameSelectionEnd()) {
            cursor.setPosition(renameSelectionBegin(), moveMode);
            m_editorWidget->setTextCursor(cursor);
            e->accept();
            return true;
        }
        break;
    }
    case Qt::Key_End: {
        // Send end to end of name when within the name and not at the end
        if (renameSelectionBegin() <= cursorPosition && cursorPosition < renameSelectionEnd()) {
            cursor.setPosition(renameSelectionEnd(), moveMode);
            m_editorWidget->setTextCursor(cursor);
            e->accept();
            return true;
        }
        break;
    }
    case Qt::Key_Backspace: {
        if (cursorPosition == renameSelectionBegin() && !cursor.hasSelection()) {
            // Eat backspace at start of name when there is no selection
            e->accept();
            return true;
        }
        break;
    }
    case Qt::Key_Delete: {
        if (cursorPosition == renameSelectionEnd() && !cursor.hasSelection()) {
            // Eat delete at end of name when there is no selection
            e->accept();
            return true;
        }
        break;
    }
    default: {
        break;
    }
    } // switch

    startRenameChange();

    const bool wantEditBlock = isWithinRenameSelection(cursorPosition);
    const int undoSizeBeforeEdit = m_editorWidget->document()->availableUndoSteps();
    if (wantEditBlock) {
        if (m_firstRenameChangeExpected) // Change inside rename selection
            cursor.beginEditBlock();
        else
            cursor.joinPreviousEditBlock();
    }
    emit processKeyPressNormally(e);
    if (wantEditBlock) {
        cursor.endEditBlock();
        if (m_firstRenameChangeExpected
                // QTCREATORBUG-16350
                && m_editorWidget->document()->availableUndoSteps() != undoSizeBeforeEdit) {
            m_firstRenameChangeExpected = false;
        }
    }
    finishRenameChange();
    return true;
}

bool CppLocalRenaming::encourageApply()
{
    if (!isActive())
        return false;
    finishRenameChange();
    return true;
}

QTextEdit::ExtraSelection &CppLocalRenaming::renameSelection()
{
    return m_selections[m_renameSelectionIndex];
}

void CppLocalRenaming::updateRenamingSelectionCursor(const QTextCursor &cursor)
{
    QTC_ASSERT(isActive(), return);
    renameSelection().cursor = cursor;
}

void CppLocalRenaming::updateRenamingSelectionFormat(const QTextCharFormat &format)
{
    QTC_ASSERT(isActive(), return);
    renameSelection().format = format;
}

void CppLocalRenaming::forgetRenamingSelection()
{
    m_renameSelectionIndex = -1;
}

bool CppLocalRenaming::isWithinSelection(const QTextEdit::ExtraSelection &selection, int position)
{
    return selection.cursor.selectionStart() <= position
           && position <= selection.cursor.selectionEnd();
}

bool CppLocalRenaming::isWithinRenameSelection(int position)
{
    return isWithinSelection(renameSelection(), position);
}

bool CppLocalRenaming::isSameSelection(int cursorPosition) const
{
    if (!isActive())
        return false;

    const QTextEdit::ExtraSelection &sel = m_selections[m_renameSelectionIndex];
    return isWithinSelection(sel, cursorPosition);
}

bool CppLocalRenaming::findRenameSelection(int cursorPosition)
{
    for (int i = 0, total = m_selections.size(); i < total; ++i) {
        const QTextEdit::ExtraSelection &sel = m_selections.at(i);
        if (isWithinSelection(sel, cursorPosition)) {
            m_renameSelectionIndex = i;
            return true;
        }
    }

    return false;
}

void CppLocalRenaming::changeOtherSelectionsText(const QString &text)
{
    for (int i = 0, total = m_selections.size(); i < total; ++i) {
        if (i == m_renameSelectionIndex)
            continue;

        QTextEdit::ExtraSelection &selection = m_selections[i];
        const int pos = selection.cursor.selectionStart();
        selection.cursor.removeSelectedText();
        selection.cursor.insertText(text);
        selection.cursor.setPosition(pos, QTextCursor::KeepAnchor);
    }
}

void CppLocalRenaming::onContentsChangeOfEditorWidgetDocument(int position,
                                                              int charsRemoved,
                                                              int charsAdded)
{
    Q_UNUSED(charsRemoved)

    if (!isActive() || m_modifyingSelections)
        return;

    if (position + charsAdded == renameSelectionBegin()) // Insert at beginning, expand cursor
        modifyCursorSelection(renameSelection().cursor, position, renameSelectionEnd());

    // Keep in mind that cursor position and anchor move automatically
    m_renameSelectionChanged = isWithinRenameSelection(position)
            && isWithinRenameSelection(position + charsAdded);

    if (!m_renameSelectionChanged)
        stop();
}

void CppLocalRenaming::startRenameChange()
{
    m_renameSelectionChanged = false;
}

void CppLocalRenaming::updateEditorWidgetWithSelections()
{
    m_editorWidget->setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection,
                                       m_selections);
}

QTextCharFormat CppLocalRenaming::textCharFormat(TextEditor::TextStyle category) const
{
    return m_editorWidget->textDocument()->fontSettings().toTextCharFormat(category);
}

void CppLocalRenaming::finishRenameChange()
{
    if (!m_renameSelectionChanged)
        return;

    m_modifyingSelections = true;

    QTextCursor cursor = m_editorWidget->textCursor();
    cursor.joinPreviousEditBlock();

    modifyCursorSelection(cursor, renameSelectionBegin(), renameSelectionEnd());
    updateRenamingSelectionCursor(cursor);
    changeOtherSelectionsText(cursor.selectedText());
    updateEditorWidgetWithSelections();

    cursor.endEditBlock();

    m_modifyingSelections = false;
}

void CppLocalRenaming::stop()
{
    if (!isActive())
        return;

    updateRenamingSelectionFormat(textCharFormat(TextEditor::C_OCCURRENCES));
    updateEditorWidgetWithSelections();
    forgetRenamingSelection();

    emit finished();
}

} // namespace Internal
} // namespace CppEditor
