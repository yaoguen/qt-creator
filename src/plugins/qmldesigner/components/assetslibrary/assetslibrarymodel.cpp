// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "assetslibrarymodel.h"

#include "assetslibrarydir.h"
#include "modelnodeoperations.h"

#include <designersettings.h>
#include <documentmanager.h>
#include <synchronousimagecache.h>
#include <theme.h>
#include <utils/hdrimage.h>
#include <qmldesignerplugin.h>
#include <modelnodeoperations.h>

#include <coreplugin/icore.h>

#include <utils/filesystemwatcher.h>
#include <utils/stylehelper.h>

#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFont>
#include <QImageReader>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QMetaProperty>
#include <QPainter>
#include <QRawFont>
#include <QRegularExpression>

static Q_LOGGING_CATEGORY(assetsLibraryBenchmark, "qtc.assetsLibrary.setRoot", QtWarningMsg)

namespace QmlDesigner {

AssetsLibraryModel::AssetsLibraryModel(Utils::FileSystemWatcher *fileSystemWatcher, QObject *parent)
    : QAbstractListModel(parent)
    , m_fileSystemWatcher(fileSystemWatcher)
{
    // add role names
    int role = 0;
    const QMetaObject meta = AssetsLibraryDir::staticMetaObject;
    for (int i = meta.propertyOffset(); i < meta.propertyCount(); ++i)
        m_roleNames.insert(role++, meta.property(i).name());
}

void AssetsLibraryModel::setSearchText(const QString &searchText)
{
    if (m_searchText != searchText) {
        m_searchText = searchText;
        refresh();
    }
}

void AssetsLibraryModel::saveExpandedState(bool expanded, const QString &assetPath)
{
    m_expandedStateHash.insert(assetPath, expanded);
}

bool AssetsLibraryModel::loadExpandedState(const QString &assetPath)
{
    return m_expandedStateHash.value(assetPath, true);
}

bool AssetsLibraryModel::isEffectQmlExist(const QString &effectName)
{
    Utils::FilePath effectsResDir = ModelNodeOperations::getEffectsDirectory();
    Utils::FilePath qmlPath = effectsResDir.resolvePath(effectName + "/" + effectName + ".qml");
    return qmlPath.exists();
}

AssetsLibraryModel::DirExpandState AssetsLibraryModel::getAllExpandedState() const
{
    const auto keys = m_expandedStateHash.keys();
    bool allExpanded = true;
    bool allCollapsed = true;
    for (const QString &assetPath : keys) {
        bool expanded = m_expandedStateHash.value(assetPath);

        if (expanded)
            allCollapsed = false;
        if (!expanded)
            allExpanded = false;

        if (!allCollapsed && !allExpanded)
            break;
    }

    return allExpanded ? DirExpandState::AllExpanded : allCollapsed ? DirExpandState::AllCollapsed
           : DirExpandState::SomeExpanded;
}

void AssetsLibraryModel::toggleExpandAll(bool expand)
{
    std::function<void(AssetsLibraryDir *)> expandDirRecursive;
    expandDirRecursive = [&](AssetsLibraryDir *currAssetsDir) {
        if (currAssetsDir->dirDepth() > 0) {
            currAssetsDir->setDirExpanded(expand);
            saveExpandedState(expand, currAssetsDir->dirPath());
        }

        const QList<AssetsLibraryDir *> childDirs = currAssetsDir->childAssetsDirs();
        for (const auto childDir : childDirs)
            expandDirRecursive(childDir);
    };

    beginResetModel();
    expandDirRecursive(m_assetsDir);
    endResetModel();
}

void AssetsLibraryModel::deleteFiles(const QStringList &filePaths)
{
    bool askBeforeDelete = QmlDesignerPlugin::settings().value(
                DesignerSettingsKey::ASK_BEFORE_DELETING_ASSET).toBool();
    bool assetDelete = true;

    if (askBeforeDelete) {
        QMessageBox msg(QMessageBox::Question, tr("Confirm Delete File"),
                        tr("File%1 might be in use. Delete anyway?\n\n%2")
                            .arg(filePaths.size() > 1 ? QChar('s') : QChar())
                            .arg(filePaths.join('\n').remove(DocumentManager::currentProjectDirPath()
                                                             .toString().append('/'))),
                        QMessageBox::No | QMessageBox::Yes);
        QCheckBox cb;
        cb.setText(tr("Do not ask this again"));
        msg.setCheckBox(&cb);
        int ret = msg.exec();

        if (ret == QMessageBox::No)
            assetDelete = false;

        if (cb.isChecked())
            QmlDesignerPlugin::settings().insert(DesignerSettingsKey::ASK_BEFORE_DELETING_ASSET, false);
    }

    if (assetDelete) {
        for (const QString &filePath : filePaths) {
            if (!QFile::exists(filePath)) {
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     tr("Failed to Locate File"),
                                     tr("Could not find \"%1\".").arg(filePath));
            } else if (!QFile::remove(filePath)) {
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     tr("Failed to Delete File"),
                                     tr("Could not delete \"%1\".").arg(filePath));
            }
        }
    }
}

bool AssetsLibraryModel::renameFolder(const QString &folderPath, const QString &newName)
{
    QDir dir{folderPath};
    QString oldName = dir.dirName();

    if (oldName == newName)
        return true;

    dir.cdUp();

    saveExpandedState(loadExpandedState(folderPath), dir.absoluteFilePath(newName));

    return dir.rename(oldName, newName);
}

void AssetsLibraryModel::addNewFolder(const QString &folderPath)
{
    QString iterPath = folderPath;
    QRegularExpression rgx("\\d+$"); // matches a number at the end of a string
    QDir dir{folderPath};

    while (dir.exists()) {
        // if the folder name ends with a number, increment it
        QRegularExpressionMatch match = rgx.match(iterPath);
        if (match.hasMatch()) { // ends with a number
            QString numStr = match.captured(0);
            int num = match.captured(0).toInt();

            // get number of padding zeros, ex: for "005" = 2
            int nPaddingZeros = 0;
            for (; nPaddingZeros < numStr.size() && numStr[nPaddingZeros] == '0'; ++nPaddingZeros);

            ++num;

            // if the incremented number's digits increased, decrease the padding zeros
            if (std::fmod(std::log10(num), 1.0) == 0)
                --nPaddingZeros;

            iterPath = folderPath.mid(0, match.capturedStart())
                         + QString('0').repeated(nPaddingZeros)
                         + QString::number(num);
        } else {
            iterPath = folderPath + '1';
        }

        dir.setPath(iterPath);
    }

    dir.mkpath(iterPath);
}

void AssetsLibraryModel::deleteFolder(const QString &folderPath)
{
    QDir{folderPath}.removeRecursively();
}

QObject *AssetsLibraryModel::rootDir() const
{
    return m_assetsDir;
}

bool AssetsLibraryModel::isEmpty() const
{
    return m_isEmpty;
}

void AssetsLibraryModel::setIsEmpty(bool empty)
{
    if (m_isEmpty != empty) {
        m_isEmpty = empty;
        emit isEmptyChanged();
    }
}

QVariant AssetsLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid index requested: " << QString::number(index.row());
        return {};
    }

    if (m_roleNames.contains(role))
        return m_assetsDir ? m_assetsDir->property(m_roleNames.value(role)) : QVariant("");

    qWarning() << Q_FUNC_INFO << "Invalid role requested: " << QString::number(role);
    return {};
}

int AssetsLibraryModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return 1;
}

QHash<int, QByteArray> AssetsLibraryModel::roleNames() const
{
    return m_roleNames;
}

// called when a directory is changed to refresh the model for this directory
void AssetsLibraryModel::refresh()
{
    setRootPath(m_assetsDir->dirPath());
}

void AssetsLibraryModel::setRootPath(const QString &path)
{
    QElapsedTimer time;
    if (assetsLibraryBenchmark().isInfoEnabled())
        time.start();

    qCInfo(assetsLibraryBenchmark) << "start:" << time.elapsed();

    static const QStringList ignoredTopLevelDirs {"imports", "asset_imports"};

    m_fileSystemWatcher->clear();

    std::function<bool(AssetsLibraryDir *, int, bool)> parseDir;
    parseDir = [this, &parseDir](AssetsLibraryDir *currAssetsDir, int currDepth, bool recursive) {
        m_fileSystemWatcher->addDirectory(currAssetsDir->dirPath(), Utils::FileSystemWatcher::WatchAllChanges);

        QDir dir(currAssetsDir->dirPath());
        dir.setNameFilters(supportedSuffixes().values());
        dir.setFilter(QDir::Files);
        QDirIterator itFiles(dir);
        bool isEmpty = true;
        while (itFiles.hasNext()) {
            QString filePath = itFiles.next();
            QString fileName = filePath.split('/').last();
            if (m_searchText.isEmpty() || fileName.contains(m_searchText, Qt::CaseInsensitive)) {
                currAssetsDir->addFile(filePath);
                m_fileSystemWatcher->addFile(filePath, Utils::FileSystemWatcher::WatchAllChanges);
                isEmpty = false;
            }
        }

        if (recursive) {
            dir.setNameFilters({});
            dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
            QDirIterator itDirs(dir);

            while (itDirs.hasNext()) {
                QDir subDir = itDirs.next();
                if (currDepth == 1 && ignoredTopLevelDirs.contains(subDir.dirName()))
                    continue;

                auto assetsDir = new AssetsLibraryDir(subDir.path(), currDepth,
                                                      loadExpandedState(subDir.path()), currAssetsDir);
                currAssetsDir->addDir(assetsDir);
                saveExpandedState(loadExpandedState(assetsDir->dirPath()), assetsDir->dirPath());
                isEmpty &= parseDir(assetsDir, currDepth + 1, true);
            }
        }

        if (!m_searchText.isEmpty() && isEmpty)
            currAssetsDir->setDirVisible(false);

        return isEmpty;
    };

    qCInfo(assetsLibraryBenchmark) << "directories parsed:" << time.elapsed();

    if (m_assetsDir)
        delete m_assetsDir;

    beginResetModel();
    m_assetsDir = new AssetsLibraryDir(path, 0, true, this);
    bool hasProject = !QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().isEmpty();
    bool isEmpty = parseDir(m_assetsDir, 1, hasProject);
    setIsEmpty(isEmpty);

    bool noAssets = m_searchText.isEmpty() && isEmpty;
    // noAssets: the model has no asset files (project has no assets added)
    // isEmpty: the model has no asset files (assets could exist but are filtered out)

    m_assetsDir->setDirVisible(!noAssets); // if there are no assets, hide all empty asset folders
    endResetModel();

    qCInfo(assetsLibraryBenchmark) << "model reset:" << time.elapsed();
}

const QStringList &AssetsLibraryModel::supportedImageSuffixes()
{
    static QStringList retList;
    if (retList.isEmpty()) {
        const QList<QByteArray> suffixes = QImageReader::supportedImageFormats();
        for (const QByteArray &suffix : suffixes)
            retList.append("*." + QString::fromUtf8(suffix));
    }
    return retList;
}

const QStringList &AssetsLibraryModel::supportedFragmentShaderSuffixes()
{
    static const QStringList retList {"*.frag", "*.glsl", "*.glslf", "*.fsh"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedShaderSuffixes()
{
    static const QStringList retList {"*.frag", "*.vert",
                                      "*.glsl", "*.glslv", "*.glslf",
                                      "*.vsh", "*.fsh"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedFontSuffixes()
{
    static const QStringList retList {"*.ttf", "*.otf"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedAudioSuffixes()
{
    static const QStringList retList {"*.wav", "*.mp3"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedVideoSuffixes()
{
    static const QStringList retList {"*.mp4"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedTexture3DSuffixes()
{
    // These are file types only supported by 3D textures
    static QStringList retList {"*.hdr", "*.ktx"};
    return retList;
}

const QStringList &AssetsLibraryModel::supportedEffectMakerSuffixes()
{
    // These are file types only supported by Effect Maker
    static QStringList retList {"*.qep"};
    return retList;
}

const QSet<QString> &AssetsLibraryModel::supportedSuffixes()
{
    static QSet<QString> allSuffixes;
    if (allSuffixes.isEmpty()) {
        auto insertSuffixes = [](const QStringList &suffixes) {
            for (const auto &suffix : suffixes)
                allSuffixes.insert(suffix);
        };
        insertSuffixes(supportedImageSuffixes());
        insertSuffixes(supportedShaderSuffixes());
        insertSuffixes(supportedFontSuffixes());
        insertSuffixes(supportedAudioSuffixes());
        insertSuffixes(supportedVideoSuffixes());
        insertSuffixes(supportedTexture3DSuffixes());
        insertSuffixes(supportedEffectMakerSuffixes());
    }
    return allSuffixes;
}

const QSet<QString> &AssetsLibraryModel::previewableSuffixes() const
{
    static QSet<QString> previewableSuffixes;
    if (previewableSuffixes.isEmpty()) {
        auto insertSuffixes = [](const QStringList &suffixes) {
            for (const auto &suffix : suffixes)
                previewableSuffixes.insert(suffix);
        };
        insertSuffixes(supportedFontSuffixes());
    }
    return previewableSuffixes;
}

} // namespace QmlDesigner
