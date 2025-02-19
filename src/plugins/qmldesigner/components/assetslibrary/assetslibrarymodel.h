// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QIcon>
#include <QPair>
#include <QSet>

namespace Utils { class FileSystemWatcher; }

namespace QmlDesigner {

class SynchronousImageCache;
class AssetsLibraryDir;

class AssetsLibraryModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isEmpty READ isEmpty WRITE setIsEmpty NOTIFY isEmptyChanged)

public:
    AssetsLibraryModel(Utils::FileSystemWatcher *fileSystemWatcher, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    void refresh();
    void setRootPath(const QString &path);
    void setSearchText(const QString &searchText);

    bool isEmpty() const;

    static const QStringList &supportedImageSuffixes();
    static const QStringList &supportedFragmentShaderSuffixes();
    static const QStringList &supportedShaderSuffixes();
    static const QStringList &supportedFontSuffixes();
    static const QStringList &supportedAudioSuffixes();
    static const QStringList &supportedVideoSuffixes();
    static const QStringList &supportedTexture3DSuffixes();
    static const QStringList &supportedEffectMakerSuffixes();
    static const QSet<QString> &supportedSuffixes();

    const QSet<QString> &previewableSuffixes() const;

    static void saveExpandedState(bool expanded, const QString &assetPath);
    static bool loadExpandedState(const QString &assetPath);

    static bool isEffectQmlExist(const QString &effectName);

    enum class DirExpandState {
        SomeExpanded,
        AllExpanded,
        AllCollapsed
    };
    Q_ENUM(DirExpandState)

    Q_INVOKABLE void toggleExpandAll(bool expand);
    Q_INVOKABLE DirExpandState getAllExpandedState() const;
    Q_INVOKABLE void deleteFiles(const QStringList &filePaths);
    Q_INVOKABLE bool renameFolder(const QString &folderPath, const QString &newName);
    Q_INVOKABLE void addNewFolder(const QString &folderPath);
    Q_INVOKABLE void deleteFolder(const QString &folderPath);
    Q_INVOKABLE QObject *rootDir() const;

signals:
    void isEmptyChanged();

private:

    void setIsEmpty(bool empty);

    QHash<QString, QPair<QDateTime, QIcon>> m_iconCache;

    QString m_searchText;
    Utils::FileSystemWatcher *m_fileSystemWatcher = nullptr;
    AssetsLibraryDir *m_assetsDir = nullptr;
    bool m_isEmpty = true;

    QHash<int, QByteArray> m_roleNames;
    inline static QHash<QString, bool> m_expandedStateHash; // <assetPath, isExpanded>
};

} // namespace QmlDesigner
