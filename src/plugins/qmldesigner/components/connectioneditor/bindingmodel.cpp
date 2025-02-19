// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bindingmodel.h"

#include "connectionview.h"

#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <bindingproperty.h>
#include <variantproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>
#include <rewriterview.h>

#include <QMessageBox>
#include <QTimer>

namespace QmlDesigner {

namespace Internal {

BindingModel::BindingModel(ConnectionView *parent)
    : QStandardItemModel(parent)
    , m_connectionView(parent)
{
    connect(this, &QStandardItemModel::dataChanged, this, &BindingModel::handleDataChanged);
}

void BindingModel::resetModel()
{
    beginResetModel();
    clear();
    setHorizontalHeaderLabels(
        QStringList({tr("Item"), tr("Property"), tr("Source Item"), tr("Source Property")}));

    if (connectionView()->isAttached()) {
        for (const ModelNode &modelNode : connectionView()->selectedModelNodes())
            addModelNode(modelNode);
    }

    endResetModel();
}

void BindingModel::bindingChanged(const BindingProperty &bindingProperty)
{
    m_handleDataChanged = false;

    QList<ModelNode> selectedNodes = connectionView()->selectedModelNodes();
    if (!selectedNodes.contains(bindingProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForBinding(bindingProperty);

        if (rowNumber == -1) {
            addBindingProperty(bindingProperty);
        } else {
            updateBindingProperty(rowNumber);
        }
    }

    m_handleDataChanged = true;
}

void BindingModel::bindingRemoved(const BindingProperty &bindingProperty)
{
    m_handleDataChanged = false;

    QList<ModelNode> selectedNodes = connectionView()->selectedModelNodes();
    if (!selectedNodes.contains(bindingProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForBinding(bindingProperty);
        removeRow(rowNumber);
    }

    m_handleDataChanged = true;
}

void BindingModel::selectionChanged([[maybe_unused]] const QList<ModelNode> &selectedNodes)
{
    m_handleDataChanged = false;
    resetModel();
    m_handleDataChanged = true;
}

ConnectionView *BindingModel::connectionView() const
{
    return m_connectionView;
}

BindingProperty BindingModel::bindingPropertyForRow(int rowNumber) const
{

    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2).toString();

    ModelNode  modelNode = connectionView()->modelNodeForInternalId(internalId);

    if (modelNode.isValid())
        return modelNode.bindingProperty(targetPropertyName.toLatin1());

    return BindingProperty();
}

QStringList BindingModel::possibleTargetProperties(const BindingProperty &bindingProperty) const
{
    const ModelNode modelNode = bindingProperty.parentModelNode();

    if (!modelNode.isValid()) {
        qWarning() << " BindingModel::possibleTargetPropertiesForRow invalid model node";
        return QStringList();
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();

    if (metaInfo.isValid()) {
        const auto properties = metaInfo.properties();
        QStringList writableProperties;
        writableProperties.reserve(static_cast<int>(properties.size()));
        for (const auto &property : properties) {
            if (property.isWritable())
                writableProperties.push_back(QString::fromUtf8(property.name()));
        }

        return writableProperties;
    }

    return QStringList();
}

QStringList BindingModel::possibleSourceProperties(const BindingProperty &bindingProperty) const
{
    const QString expression = bindingProperty.expression();
    const QStringList stringlist = expression.split(QLatin1String("."));
    QStringList possibleProperties;

    NodeMetaInfo type;

    if (auto metaInfo = bindingProperty.parentModelNode().metaInfo(); metaInfo.isValid())
        type = metaInfo.property(bindingProperty.name()).propertyType();
    else
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for target node";

    const QString &id = stringlist.constFirst();

    ModelNode modelNode = getNodeByIdOrParent(id, bindingProperty.parentModelNode());

    if (!modelNode.isValid()) {
        //if it's not a valid model node, maybe it's a singleton
        if (RewriterView* rv = connectionView()->rewriterView()) {
            for (const QmlTypeData &data : rv->getQMLTypes()) {
                if (!data.typeName.isEmpty() && data.typeName == id) {
                    NodeMetaInfo metaInfo = connectionView()->model()->metaInfo(data.typeName.toUtf8());

                    if (metaInfo.isValid()) {
                        for (const auto &property : metaInfo.properties()) {
                            //without check for now
                            possibleProperties.push_back(QString::fromUtf8(property.name()));
                        }

                        return possibleProperties;
                    }
                }
            }
        }

        qWarning() << " BindingModel::possibleSourcePropertiesForRow invalid model node";
        return QStringList();
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();

    for (const VariantProperty &variantProperty : modelNode.variantProperties()) {
        if (variantProperty.isDynamic())
            possibleProperties << QString::fromUtf8(variantProperty.name());
    }

    for (const BindingProperty &bindingProperty : modelNode.bindingProperties()) {
        if (bindingProperty.isDynamic())
            possibleProperties << QString::fromUtf8((bindingProperty.name()));
    }

    if (metaInfo.isValid())  {
        for (const auto &property : metaInfo.properties()) {
            if (property.propertyType() == type) //### todo proper check
                possibleProperties.push_back(QString::fromUtf8(property.name()));
        }
    } else {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for source node";
    }

    return possibleProperties;
}

void BindingModel::deleteBindindByRow(int rowNumber)
{
      BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

      if (bindingProperty.isValid()) {
        bindingProperty.parentModelNode().removeProperty(bindingProperty.name());
      }

      resetModel();
}

static PropertyName unusedProperty(const ModelNode &modelNode)
{
    PropertyName propertyName = "none";
    if (modelNode.metaInfo().isValid()) {
        for (const auto &property : modelNode.metaInfo().properties()) {
            if (property.isWritable() && !modelNode.hasProperty(propertyName))
                return property.name();
        }
    }

    return propertyName;
}

void BindingModel::addBindingForCurrentNode()
{
    if (connectionView()->selectedModelNodes().count() == 1) {
        const ModelNode modelNode = connectionView()->selectedModelNodes().constFirst();
        if (modelNode.isValid()) {
            try {
                modelNode.bindingProperty(unusedProperty(modelNode)).setExpression(QLatin1String("none.none"));
            } catch (RewritingException &e) {
                m_exceptionError = e.description();
                QTimer::singleShot(200, this, &BindingModel::handleException);
            }
        }
    } else {
        qWarning() << " BindingModel::addBindingForCurrentNode not one node selected";
    }
}

void BindingModel::addBindingProperty(const BindingProperty &property)
{
    QStandardItem *idItem;
    QStandardItem *targetPropertyNameItem;
    QStandardItem *sourceIdItem;
    QStandardItem *sourcePropertyNameItem;

    QString idLabel = property.parentModelNode().id();
    if (idLabel.isEmpty())
        idLabel = property.parentModelNode().simplifiedTypeName();
    idItem = new QStandardItem(idLabel);
    updateCustomData(idItem, property);
    targetPropertyNameItem = new QStandardItem(QString::fromUtf8(property.name()));
    QList<QStandardItem*> items;

    items.append(idItem);
    items.append(targetPropertyNameItem);

    QString sourceNodeName;
    QString sourcePropertyName;
    getExpressionStrings(property, &sourceNodeName, &sourcePropertyName);

    sourceIdItem = new QStandardItem(sourceNodeName);
    sourcePropertyNameItem = new QStandardItem(sourcePropertyName);

    items.append(sourceIdItem);
    items.append(sourcePropertyNameItem);
    appendRow(items);
}

void BindingModel::updateBindingProperty(int rowNumber)
{
    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

    if (bindingProperty.isValid()) {
        QString targetPropertyName = QString::fromUtf8(bindingProperty.name());
        updateDisplayRole(rowNumber, TargetPropertyNameRow, targetPropertyName);
        QString sourceNodeName;
        QString sourcePropertyName;
        getExpressionStrings(bindingProperty, &sourceNodeName, &sourcePropertyName);
        updateDisplayRole(rowNumber, SourceModelNodeRow, sourceNodeName);
        updateDisplayRole(rowNumber, SourcePropertyNameRow, sourcePropertyName);
    }
}

void BindingModel::addModelNode(const ModelNode &modelNode)
{
    const QList<BindingProperty> bindingProperties = modelNode.bindingProperties();
    for (const BindingProperty &bindingProperty : bindingProperties) {
        addBindingProperty(bindingProperty);
    }
}

void BindingModel::updateExpression(int row)
{
    const QString sourceNode = data(index(row, SourceModelNodeRow)).toString().trimmed();
    const QString sourceProperty = data(index(row, SourcePropertyNameRow)).toString().trimmed();

    QString expression;
    if (sourceProperty.isEmpty()) {
        expression = sourceNode;
    } else {
        expression = sourceNode + QLatin1String(".") + sourceProperty;
    }

    connectionView()->executeInTransaction("BindingModel::updateExpression", [this, row, expression](){
        BindingProperty bindingProperty = bindingPropertyForRow(row);
        bindingProperty.setExpression(expression.trimmed());
    });
}

void BindingModel::updatePropertyName(int rowNumber)
{
    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

    const PropertyName newName = data(index(rowNumber, TargetPropertyNameRow)).toString().toUtf8();
    const QString expression = bindingProperty.expression();
    const PropertyName dynamicPropertyType = bindingProperty.dynamicTypeName();
    ModelNode targetNode = bindingProperty.parentModelNode();

    if (!newName.isEmpty()) {
        RewriterTransaction transaction =
            connectionView()->beginRewriterTransaction(QByteArrayLiteral("BindingModel::updatePropertyName"));
        try {
            if (bindingProperty.isDynamic()) {
                targetNode.bindingProperty(newName).setDynamicTypeNameAndExpression(dynamicPropertyType, expression);
            } else {
                targetNode.bindingProperty(newName).setExpression(expression);
            }
            targetNode.removeProperty(bindingProperty.name());
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            m_exceptionError = e.description();
            QTimer::singleShot(200, this, &BindingModel::handleException);
        }

        QStandardItem* idItem = item(rowNumber, 0);
        BindingProperty newBindingProperty = targetNode.bindingProperty(newName);
        updateCustomData(idItem, newBindingProperty);

    } else {
        qWarning() << "BindingModel::updatePropertyName invalid property name";
    }
}

ModelNode BindingModel::getNodeByIdOrParent(const QString &id, const ModelNode &targetNode) const
{
    ModelNode modelNode;

    if (id != QLatin1String("parent")) {
        modelNode = connectionView()->modelNodeForId(id);
    } else {
        if (targetNode.hasParentProperty()) {
            modelNode = targetNode.parentProperty().parentModelNode();
        }
    }
    return modelNode;
}

void BindingModel::updateCustomData(QStandardItem *item, const BindingProperty &bindingProperty)
{
    item->setData(bindingProperty.parentModelNode().internalId(), Qt::UserRole + 1);
    item->setData(bindingProperty.name(), Qt::UserRole + 2);
}

int BindingModel::findRowForBinding(const BindingProperty &bindingProperty)
{
    for (int i=0; i < rowCount(); i++) {
        if (compareBindingProperties(bindingPropertyForRow(i), bindingProperty))
            return i;
    }
    //not found
    return -1;
}

bool BindingModel::getExpressionStrings(const BindingProperty &bindingProperty, QString *sourceNode, QString *sourceProperty)
{
    //### todo we assume no expressions yet

    const QString expression = bindingProperty.expression();

    if (true) {
        const QStringList stringList = expression.split(QLatin1String("."));

        *sourceNode = stringList.constFirst();

        QString propertyName;

        for (int i=1; i < stringList.count(); i++) {
            propertyName += stringList.at(i);
            if (i != stringList.count() - 1)
                propertyName += QLatin1String(".");
        }
        *sourceProperty = propertyName;
    }
    return true;
}

void BindingModel::updateDisplayRole(int row, int columns, const QString &string)
{
    QModelIndex modelIndex = index(row, columns);
    if (data(modelIndex).toString() != string)
        setData(modelIndex, string);
}

void BindingModel::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_handleDataChanged)
        return;

    if (topLeft != bottomRight) {
        qWarning() << "BindingModel::handleDataChanged multi edit?";
        return;
    }

    m_lock = true;

    int currentColumn = topLeft.column();
    int currentRow = topLeft.row();

    switch (currentColumn) {
    case TargetModelNodeRow: {
        //updating user data
    } break;
    case TargetPropertyNameRow: {
        updatePropertyName(currentRow);
    } break;
    case SourceModelNodeRow: {
        updateExpression(currentRow);
    } break;
    case SourcePropertyNameRow: {
        updateExpression(currentRow);
    } break;

    default: qWarning() << "BindingModel::handleDataChanged column" << currentColumn;
    }

    m_lock = false;
}

void BindingModel::handleException()
{
    QMessageBox::warning(nullptr, tr("Error"), m_exceptionError);
    resetModel();
}

} // namespace Internal

} // namespace QmlDesigner
