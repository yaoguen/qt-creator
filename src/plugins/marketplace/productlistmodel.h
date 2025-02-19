// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/welcomepagehelper.h>

#include <QQueue>
#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

namespace Marketplace {
namespace Internal {

class ProductGridView;
class ProductItemDelegate;

class ProductItem : public Core::ListItem
{
public:
    QString handle;
};

class ProductListModel : public Core::ListModel
{
public:
    explicit ProductListModel(QObject *parent);
    void appendItems(const QList<Core::ListItem *> &items);
    const QList<Core::ListItem *> items() const;
    void updateModelIndexesForUrl(const QString &url);

protected:
    QPixmap fetchPixmapAndUpdatePixmapCache(const QString &url) const override;
};

struct Section
{
    friend bool operator<(const Section &lhs, const Section &rhs)
    {
        if (lhs.priority < rhs.priority)
            return true;
        return lhs.priority > rhs.priority ? false : lhs.name < rhs.name;
    }

    friend bool operator==(const Section &lhs, const Section &rhs)
    {
        return lhs.priority == rhs.priority && lhs.name == rhs.name;
    }

    QString name;
    int priority;
};

class SectionedProducts : public QStackedWidget
{
    Q_OBJECT
public:
    explicit SectionedProducts(QWidget *parent);
    ~SectionedProducts() override;
    void updateCollections();
    void queueImageForDownload(const QString &url);
    void setColumnCount(int columns);
    void setSearchString(const QString &searchString);

signals:
    void errorOccurred(int errorCode, const QString &errorString);
    void toggleProgressIndicator(bool show);
    void tagClicked(const QString &tag);

private:
    void onFetchCollectionsFinished(QNetworkReply *reply);
    void onFetchSingleCollectionFinished(QNetworkReply *reply);
    void fetchCollectionsContents();

    void fetchNextImage();
    void onImageDownloadFinished(QNetworkReply *reply);
    void addNewSection(const Section &section, const QList<Core::ListItem *> &items);
    void onTagClicked(const QString &tag);

    QList<Core::ListItem *> items();

    QQueue<QString> m_pendingCollections;
    QSet<QString> m_pendingImages;
    QMap<QString, QString> m_collectionTitles;
    QMap<Section, ProductListModel *> m_productModels;
    QMap<Section, Core::GridView *> m_gridViews;
    Core::GridView *m_allProductsView = nullptr;
    Core::ListModelFilter *m_filteredAllProductsModel = nullptr;
    ProductItemDelegate *m_productDelegate = nullptr;
    bool m_isDownloadingImage = false;
    int m_columnCount = 1;
};

} // namespace Internal
} // namespace Marketplace

Q_DECLARE_METATYPE(Marketplace::Internal::ProductItem *)
