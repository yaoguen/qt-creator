// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/find/searchresultitem.h>
#include <cppeditor/cursorineditor.h>
#include <utils/link.h>

#include <QObject>

#include <optional>

QT_BEGIN_NAMESPACE
class QTextCursor;
QT_END_NAMESPACE

namespace Core { class SearchResult; }
namespace TextEditor { class TextDocument; }

namespace ClangCodeModel::Internal {
class ClangdClient;

class ClangdFindReferences : public QObject
{
    Q_OBJECT
public:
    ClangdFindReferences(ClangdClient *client, TextEditor::TextDocument *document,
                         const QTextCursor &cursor, const QString &searchTerm,
                         const std::optional<QString> &replacement, bool categorize);
    ClangdFindReferences(ClangdClient *client, const Utils::Link &link, Core::SearchResult *search,
                         const Utils::LinkHandler &callback);
    ~ClangdFindReferences();

signals:
    void foundReferences(const QList<Core::SearchResultItem> &items);
    void done();

private:
    class Private;
    class CheckUnusedData;
    Private * const d;
};

class ClangdFindLocalReferences : public QObject
{
    Q_OBJECT
public:
    explicit ClangdFindLocalReferences(ClangdClient *client, TextEditor::TextDocument *document,
                                       const QTextCursor &cursor,
                                       const CppEditor::RenameCallback &callback);
    ~ClangdFindLocalReferences();

signals:
    void done();

private:
    class Private;
    Private * const d;
};

} // namespace ClangCodeModel::Internal
