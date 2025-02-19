// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlitemnode.h>

#include <QAbstractListModel>
#include <QColor>
#include <QtQml>
#include <enumeration.h>

class GradientModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QVariant anchorBackendProperty READ anchorBackend WRITE setAnchorBackend NOTIFY anchorBackendChanged)
    Q_PROPERTY(QString gradientPropertyName READ gradientPropertyName WRITE setGradientPropertyName)
    Q_PROPERTY(QString gradientTypeName READ gradientTypeName WRITE setGradientTypeName NOTIFY gradientTypeChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY gradientCountChanged)
    Q_PROPERTY(bool hasGradient READ hasGradient NOTIFY hasGradientChanged)

public:
    explicit GradientModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE int addStop(qreal position, const QColor &color);
    Q_INVOKABLE void addGradient();

    Q_INVOKABLE void setColor(int index, const QColor &color);
    Q_INVOKABLE void setPosition(int index, qreal positition);

    Q_INVOKABLE QColor getColor(int index) const;
    Q_INVOKABLE qreal getPosition(int index) const;

    Q_INVOKABLE void removeStop(int index);
    Q_INVOKABLE void deleteGradient();

    Q_INVOKABLE void lock();
    Q_INVOKABLE void unlock();

    static void registerDeclarativeType();

    Q_INVOKABLE qreal readGradientProperty(const QString &property) const;
    Q_INVOKABLE QString readGradientOrientation() const;

    Q_INVOKABLE void setGradientProperty(const QString &propertyName, qreal value);
    Q_INVOKABLE void setGradientOrientation(Qt::Orientation value);

    Q_INVOKABLE void setPresetByID(int presetID);
    Q_INVOKABLE void setPresetByStops(const QList<qreal> &stopsPositions,
                                      const QList<QString> &stopsColors,
                                      int stopsCount,
                                      bool saveTransaction = true);

    Q_INVOKABLE void savePreset();

    Q_INVOKABLE void updateGradient();

signals:
    void anchorBackendChanged();
    void hasGradientChanged();
    void gradientTypeChanged();
    void gradientCountChanged();

private:
    void setupModel();
    void setAnchorBackend(const QVariant &anchorBackend);
    QVariant anchorBackend() const {return QVariant(); }
    QString gradientPropertyName() const;
    void setGradientPropertyName(const QString &name);
    QString gradientTypeName() const;
    void setGradientTypeName(const QString &name);
    bool hasGradient() const;
    bool locked() const;
    QmlDesigner::ModelNode createGradientNode();
    QmlDesigner::ModelNode createGradientStopNode();
    void deleteGradientNode(bool saveTransaction);

private:
    QmlDesigner::QmlItemNode m_itemNode;
    QString m_gradientPropertyName;
    QString m_gradientTypeName = {"Gradient"};
    bool m_locked = false;
    bool hasShapesImport() const;
    void ensureShapesImport();
    void setupGradientProperties(const QmlDesigner::ModelNode &gradient);
    QmlDesigner::Model *model() const;
    QmlDesigner::AbstractView *view() const;
    void resetPuppet();
};

QML_DECLARE_TYPE(GradientModel)
