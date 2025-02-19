// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "delegates.h"

#include "backendmodel.h"
#include "connectionmodel.h"
#include "bindingmodel.h"
#include "dynamicpropertiesmodel.h"
#include "connectionview.h"
#include "nodemetainfo.h"

#include <bindingproperty.h>

#include <utils/qtcassert.h>

#include <QStyleFactory>
#include <QItemEditorFactory>
#include <QDebug>

namespace QmlDesigner {

namespace  Internal {

QStringList prependOnForSignalHandler(const QStringList &signalNames)
{
    QStringList signalHandlerNames;
    for (const QString &signalName : signalNames) {
        QString signalHandlerName = signalName;
        if (!signalHandlerName.isEmpty()) {
            QChar firstChar = signalHandlerName.at(0).toUpper();
            signalHandlerName[0] = firstChar;
            signalHandlerName.prepend(QLatin1String("on"));
            signalHandlerNames.append(signalHandlerName);
        }
    }
    return signalHandlerNames;
}

PropertiesComboBox::PropertiesComboBox(QWidget *parent) : QComboBox(parent)
{
    setEditable(true);
    setValidator(new QRegularExpressionValidator(QRegularExpression(QLatin1String("[a-z|A-Z|0-9|._-]*")), this));
}

QString PropertiesComboBox::text() const
{
    return currentText();
}

void PropertiesComboBox::setText(const QString &text)
{
    setEditText(text);
}

void PropertiesComboBox::disableValidator()
{
    setValidator(nullptr);
}

ConnectionComboBox::ConnectionComboBox(QWidget *parent) : PropertiesComboBox(parent)
{
}

QString ConnectionComboBox::text() const
{
    int index = findText(currentText());
    if (index > -1) {
        QVariant variantData = itemData(index);
        if (variantData.isValid())
            return variantData.toString();
    }

    return currentText();
}

ConnectionEditorDelegate::ConnectionEditorDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
{
}

void ConnectionEditorDelegate::paint(QPainter *painter,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;
    QStyledItemDelegate::paint(painter, opt, index);
}

BindingDelegate::BindingDelegate(QWidget *parent) : ConnectionEditorDelegate(parent)
{
    static QItemEditorFactory *factory = nullptr;
    if (factory == nullptr) {
        factory = new QItemEditorFactory;
        QItemEditorCreatorBase *creator
                = new QItemEditorCreator<PropertiesComboBox>("text");
        factory->registerEditor(QVariant::String, creator);
    }

    setItemEditorFactory(factory);
}

QWidget *BindingDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
        QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);

        const auto model = qobject_cast<const BindingModel*>(index.model());
        if (!model) {
            qWarning() << "BindingDelegate::createEditor no model";
            return widget;
        }
        if (!model->connectionView()) {
            qWarning() << "BindingDelegate::createEditor no connection view";
            return widget;
        }

        model->connectionView()->allModelNodes();

        auto bindingComboBox = qobject_cast<PropertiesComboBox*>(widget);
        if (!bindingComboBox) {
            qWarning() << "BindingDelegate::createEditor no bindingComboBox";
            return widget;
        }

        BindingProperty bindingProperty = model->bindingPropertyForRow(index.row());

        switch (index.column()) {
        case BindingModel::TargetModelNodeRow:
            return nullptr; //no editor
        case BindingModel::TargetPropertyNameRow: {
            bindingComboBox->addItems(model->possibleTargetProperties(bindingProperty));
        } break;
        case BindingModel::SourceModelNodeRow: {
            //common items
            for (const ModelNode &modelNode : model->connectionView()->allModelNodes()) {
                if (!modelNode.id().isEmpty()) {
                    bindingComboBox->addItem(modelNode.id());
                }
            }
            //singletons:
            if (RewriterView* rv = model->connectionView()->rewriterView()) {
                for (const QmlTypeData &data : rv->getQMLTypes()) {
                    if (!data.typeName.isEmpty()) {
                        bindingComboBox->addItem(data.typeName);
                    }
                }
            }
            //parent:
            if (!bindingProperty.parentModelNode().isRootNode())
                bindingComboBox->addItem(QLatin1String("parent"));
        } break;
        case BindingModel::SourcePropertyNameRow: {
            bindingComboBox->addItems(model->possibleSourceProperties(bindingProperty));
            bindingComboBox->disableValidator();
        } break;
        default: qWarning() << "BindingDelegate::createEditor column" << index.column();
        }

        connect(bindingComboBox, &QComboBox::activated, this, [=] {
            auto delegate = const_cast<BindingDelegate*>(this);
            emit delegate->commitData(bindingComboBox);
        });

        return widget;
}

DynamicPropertiesDelegate::DynamicPropertiesDelegate(QWidget *parent) : ConnectionEditorDelegate(parent)
{
//    static QItemEditorFactory *factory = 0;
//        if (factory == 0) {
//            factory = new QItemEditorFactory;
//            QItemEditorCreatorBase *creator
//                = new QItemEditorCreator<DynamicPropertiesComboBox>("text");
//            factory->registerEditor(QVariant::String, creator);
//        }

//        setItemEditorFactory(factory);
}

QWidget *DynamicPropertiesDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
        QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);

        const auto model = qobject_cast<const DynamicPropertiesModel*>(index.model());
        if (!model) {
            qWarning() << "BindingDelegate::createEditor no model";
            return widget;
        }

        if (!model->view()) {
            qWarning() << "BindingDelegate::createEditor no connection view";
            return widget;
        }
        model->view()->allModelNodes();

        switch (index.column()) {
        case DynamicPropertiesModel::TargetModelNodeRow: {
            return nullptr; //no editor
        };
        case DynamicPropertiesModel::PropertyNameRow: {
            return QStyledItemDelegate::createEditor(parent, option, index);
        };
        case DynamicPropertiesModel::PropertyTypeRow: {

            auto dynamicPropertiesComboBox = new PropertiesComboBox(parent);
            connect(dynamicPropertiesComboBox, &QComboBox::activated, this, [=] {
                auto delegate = const_cast<DynamicPropertiesDelegate*>(this);
                emit delegate->commitData(dynamicPropertiesComboBox);
            });

            dynamicPropertiesComboBox->addItem(QLatin1String("alias"));
            dynamicPropertiesComboBox->addItem(QLatin1String("Item"));
            dynamicPropertiesComboBox->addItem(QLatin1String("real"));
            dynamicPropertiesComboBox->addItem(QLatin1String("int"));
            dynamicPropertiesComboBox->addItem(QLatin1String("string"));
            dynamicPropertiesComboBox->addItem(QLatin1String("bool"));
            dynamicPropertiesComboBox->addItem(QLatin1String("url"));
            dynamicPropertiesComboBox->addItem(QLatin1String("color"));
            dynamicPropertiesComboBox->addItem(QLatin1String("variant"));
            dynamicPropertiesComboBox->addItem(QLatin1String("TextureInput"));
            dynamicPropertiesComboBox->addItem(QLatin1String("vector2d"));
            dynamicPropertiesComboBox->addItem(QLatin1String("vector3d"));
            dynamicPropertiesComboBox->addItem(QLatin1String("vector4d"));
            return dynamicPropertiesComboBox;
        };
        case DynamicPropertiesModel::PropertyValueRow: {
            return QStyledItemDelegate::createEditor(parent, option, index);
        };
        default: qWarning() << "BindingDelegate::createEditor column" << index.column();
        }

        return nullptr;
}

ConnectionDelegate::ConnectionDelegate(QWidget *parent) : ConnectionEditorDelegate(parent)
{
    static QItemEditorFactory *factory = nullptr;
    if (factory == nullptr) {
        factory = new QItemEditorFactory;
        QItemEditorCreatorBase *creator
                = new QItemEditorCreator<ConnectionComboBox>("text");
        factory->registerEditor(QVariant::String, creator);
    }

    setItemEditorFactory(factory);
}

static QString nameForAction(const QString &input)
{
    QStringList list = input.split('.');
    return list.first();
}

QWidget *ConnectionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{

    QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);

    const auto connectionModel = qobject_cast<const ConnectionModel*>(index.model());

    auto connectionComboBox = qobject_cast<ConnectionComboBox*>(widget);

    if (!connectionModel) {
        qWarning() << "ConnectionDelegate::createEditor no model";
        return widget;
    }

    if (!connectionModel->connectionView()) {
        qWarning() << "ConnectionDelegate::createEditor no connection view";
        return widget;
    }

    if (!connectionComboBox) {
        qWarning() << "ConnectionDelegate::createEditor no bindingComboBox";
        return widget;
    }

    switch (index.column()) {
    case ConnectionModel::TargetModelNodeRow: {

        auto addMetaInfoProperties = [&](const NodeMetaInfo& itemMetaInfo, QString itemName){
            if (itemMetaInfo.isValid()) {
                for (const auto &property : itemMetaInfo.properties()) {
                    NodeMetaInfo propertyType = property.propertyType();
                    if (propertyType.isValid() && propertyType.isFileComponent()) {
                        if (!property.isEnumType() && !property.isPrivate()
                            && !property.isListProperty() && !property.isPointer()) {
                            if (propertyType.isQtObject()) {
                                connectionComboBox->addItem(itemName + "." + property.name());
                            }
                        }
                    }
                }
            }
        };

        for (const ModelNode &modelNode : connectionModel->connectionView()->allModelNodes()) {
            if (!modelNode.id().isEmpty()) {
                connectionComboBox->addItem(modelNode.id());

                for (const BindingProperty &property : modelNode.bindingProperties()) {
                    if (property.isValid()) {
                        if (property.isAlias()) {
                            connectionComboBox->addItem(modelNode.id()
                                                        + "."
                                                        + QString::fromUtf8(property.name()));
                        }
                    }
                }

                //Components
                if (modelNode.isComponent()) {
                    NodeMetaInfo componentMetaInfo = modelNode.metaInfo();
                    addMetaInfoProperties(componentMetaInfo, modelNode.id());
                }
            }
        }
        //singletons:
        if (RewriterView* rv = connectionModel->connectionView()->rewriterView()) {
            for (const QmlTypeData &data : rv->getQMLTypes()) {
                if (!data.typeName.isEmpty()) {
                    //singleton itself
                    connectionComboBox->addItem(data.typeName);

                    //its properties, mostly looking for aliases:
                    NodeMetaInfo metaInfo = connectionModel->connectionView()->model()->metaInfo(data.typeName.toUtf8());
                    addMetaInfoProperties(metaInfo, data.typeName);
                }
            }
        }
    } break;
    case ConnectionModel::TargetPropertyNameRow: {
        connectionComboBox->addItems(prependOnForSignalHandler(connectionModel->getSignalsForRow(index.row())));
    } break;
    case ConnectionModel::SourceRow: {
        ModelNode rootModelNode = connectionModel->connectionView()->rootModelNode();
        if (QmlItemNode::isValidQmlItemNode(rootModelNode) && !rootModelNode.id().isEmpty()) {

            QString itemText = tr("Change to default state");
            QString source = QString::fromLatin1("%1.state = \"\"").arg(rootModelNode.id());
            connectionComboBox->addItem(itemText, source);
            connectionComboBox->disableValidator();

            for (const QmlModelState &state : QmlItemNode(rootModelNode).states().allStates()) {
                QString itemText = tr("Change state to %1").arg(state.name());
                QString source = QString::fromLatin1("%1.state = \"%2\"")
                                     .arg(rootModelNode.id())
                                     .arg(state.name());
                connectionComboBox->addItem(itemText, source);
            }

            QStringList trigger = connectionModel->getflowActionTriggerForRow(index.row());
            for (const QString &action : trigger) {
                connectionComboBox->addItem(tr("Activate FlowAction %1").arg(nameForAction(action)), action);
            }
        }
        connectionComboBox->disableValidator();
    } break;

    default: qWarning() << "ConnectionDelegate::createEditor column" << index.column();
    }

    connect(connectionComboBox, &QComboBox::activated, this, [=] {
        auto delegate = const_cast<ConnectionDelegate*>(this);
        emit delegate->commitData(connectionComboBox);
    });

    return widget;
}

BackendDelegate::BackendDelegate(QWidget *parent) : ConnectionEditorDelegate(parent)
{
}

QWidget *BackendDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
        const auto model = qobject_cast<const BackendModel*>(index.model());

        model->connectionView()->allModelNodes();

        QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);

        QTC_ASSERT(model, return widget);
        QTC_ASSERT(model->connectionView(), return widget);

        switch (index.column()) {
        case BackendModel::TypeNameColumn: {
            auto backendComboBox = new PropertiesComboBox(parent);
            backendComboBox->addItems(model->possibleCppTypes());
            connect(backendComboBox, &QComboBox::activated, this, [=] {
                auto delegate = const_cast<BackendDelegate*>(this);
                emit delegate->commitData(backendComboBox);
            });
            return backendComboBox;
        };
        case BackendModel::PropertyNameColumn: {
            return widget;
        };
        case BackendModel::IsSingletonColumn: {
            return nullptr;  //no editor
        };
        case BackendModel::IsLocalColumn: {
            return nullptr;  //no editor
        };
        default: qWarning() << "BackendDelegate::createEditor column" << index.column();
        }

        return widget;
}

} // namesapce Internal

} // namespace QmlDesigner
