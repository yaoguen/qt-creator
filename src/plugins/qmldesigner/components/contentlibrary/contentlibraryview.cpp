// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "contentlibraryview.h"

#include "bindingproperty.h"
#include "contentlibrarybundleimporter.h"
#include "contentlibrarywidget.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialsmodel.h"
#include "contentlibrarytexturesmodel.h"
#include "modelnodeoperations.h"
#include "nodelistproperty.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <coreplugin/messagebox.h>
#include <enumeration.h>
#include <utils/algorithm.h>

#ifndef QMLDESIGNER_TEST
#include <projectexplorer/kit.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#endif

namespace QmlDesigner {

ContentLibraryView::ContentLibraryView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
{}

ContentLibraryView::~ContentLibraryView()
{}

bool ContentLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo ContentLibraryView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new ContentLibraryWidget();

        connect(m_widget, &ContentLibraryWidget::bundleMaterialDragStarted, this,
                [&] (QmlDesigner::ContentLibraryMaterial *mat) {
            m_draggedBundleMaterial = mat;
        });
        connect(m_widget, &ContentLibraryWidget::addTextureRequested, this,
                [&] (const QString texPath, ContentLibraryWidget::AddTextureMode mode) {
            executeInTransaction("ContentLibraryView::widgetInfo", [&] {
                // copy image to project
                AddFilesResult result = ModelNodeOperations::addImageToProject({texPath}, "images", false);

                if (result == AddFilesResult::Failed) {
                    Core::AsynchronousMessageBox::warning(tr("Failed to Add Texture"),
                                                          tr("Could not add %1 to project.").arg(texPath));
                    return;
                }

                if (mode == ContentLibraryWidget::AddTextureMode::Image)
                    return;

                // create a texture from the image
                ModelNode matLib = materialLibraryNode();
                if (!matLib.isValid())
                    return;

                NodeMetaInfo metaInfo = model()->metaInfo("QtQuick3D.Texture");
                ModelNode newTexNode = createModelNode("QtQuick3D.Texture", metaInfo.majorVersion(),
                                                                            metaInfo.minorVersion());
                newTexNode.validId();
                VariantProperty sourceProp = newTexNode.variantProperty("source");
                sourceProp.setValue(QLatin1String("images/%1").arg(texPath.split('/').last()));
                matLib.defaultNodeListProperty().reparentHere(newTexNode);

                // assign the texture as scene environment's light probe
                if (mode == ContentLibraryWidget::AddTextureMode::LightProbe && m_activeSceneEnv.isValid()) {
                    BindingProperty lightProbeProp = m_activeSceneEnv.bindingProperty("lightProbe");
                    lightProbeProp.setExpression(newTexNode.id());
                    VariantProperty bgModeProp = m_activeSceneEnv.variantProperty("backgroundMode");
                    bgModeProp.setValue(QVariant::fromValue(Enumeration("SceneEnvironment", "SkyBox")));
                }
            });
        });

        ContentLibraryMaterialsModel *materialsModel = m_widget->materialsModel().data();

        connect(materialsModel, &ContentLibraryMaterialsModel::applyToSelectedTriggered, this,
                [&] (ContentLibraryMaterial *bundleMat, bool add) {
            if (m_selectedModels.isEmpty())
                return;

            m_bundleMaterialTargets = m_selectedModels;
            m_bundleMaterialAddToSelected = add;

            ModelNode defaultMat = getBundleMaterialDefaultInstance(bundleMat->type());
            if (defaultMat.isValid())
                applyBundleMaterialToDropTarget(defaultMat);
            else
                m_widget->materialsModel()->addToProject(bundleMat);
        });

        connect(materialsModel, &ContentLibraryMaterialsModel::bundleMaterialImported, this,
                [&] (const QmlDesigner::NodeMetaInfo &metaInfo) {
            applyBundleMaterialToDropTarget({}, metaInfo);
            updateBundleMaterialsImportedState();
        });

        connect(materialsModel, &ContentLibraryMaterialsModel::bundleMaterialAboutToUnimport, this,
                [&] (const QmlDesigner::TypeName &type) {
            // delete instances of the bundle material that is about to be unimported
            executeInTransaction("ContentLibraryView::widgetInfo", [&] {
                ModelNode matLib = materialLibraryNode();
                if (!matLib.isValid())
                    return;

                Utils::reverseForeach(matLib.directSubModelNodes(), [&](const ModelNode &mat) {
                    if (mat.isValid() && mat.type() == type)
                        QmlObjectNode(mat).destroy();
                });
            });
        });

        connect(materialsModel, &ContentLibraryMaterialsModel::bundleMaterialUnimported, this,
                &ContentLibraryView::updateBundleMaterialsImportedState);
    }

    return createWidgetInfo(m_widget.data(),
                            "ContentLibrary",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Content Library"));
}

void ContentLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_hasQuick3DImport = model->hasImport("QtQuick3D");

    m_widget->materialsModel()->setHasMaterialRoot(rootModelNode().metaInfo().isQtQuick3DMaterial());
    m_widget->materialsModel()->setHasQuick3DImport(m_hasQuick3DImport);

    updateBundleMaterialsQuick3DVersion();
    updateBundleMaterialsImportedState();
}

void ContentLibraryView::modelAboutToBeDetached(Model *model)
{

    AbstractView::modelAboutToBeDetached(model);
}

void ContentLibraryView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    Q_UNUSED(addedImports)
    Q_UNUSED(removedImports)

    updateBundleMaterialsQuick3DVersion();

    bool hasQuick3DImport = model()->hasImport("QtQuick3D");

    if (hasQuick3DImport == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = hasQuick3DImport;
    m_widget->materialsModel()->setHasQuick3DImport(m_hasQuick3DImport);
}

void ContentLibraryView::active3DSceneChanged(qint32 sceneId)
{
    m_activeSceneEnv = {};
    bool sceneEnvExists = false;
    if (sceneId != -1) {
        ModelNode activeScene = active3DSceneNode();
        if (activeScene.isValid()) {
            ModelNode view3D;
            if (activeScene.metaInfo().isQtQuick3DView3D()) {
                view3D = activeScene;
            } else {
                ModelNode sceneParent = activeScene.parentProperty().parentModelNode();
                if (sceneParent.metaInfo().isQtQuick3DView3D())
                    view3D = sceneParent;
            }

            if (view3D.isValid()) {
                m_activeSceneEnv = modelNodeForId(view3D.bindingProperty("environment").expression());
                if (m_activeSceneEnv.isValid())
                    sceneEnvExists = true;
            }
        }
    }

    m_widget->texturesModel()->setHasSceneEnv(sceneEnvExists);
    m_widget->environmentsModel()->setHasSceneEnv(sceneEnvExists);
}

void ContentLibraryView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                              const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList)

    m_selectedModels = Utils::filtered(selectedNodeList, [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DModel();
    });

    m_widget->materialsModel()->setHasModelSelection(!m_selectedModels.isEmpty());
}

void ContentLibraryView::customNotification(const AbstractView *view, const QString &identifier,
                                            const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    Q_UNUSED(data)

    if (view == this)
        return;

    if (identifier == "drop_bundle_material") {
        ModelNode matLib = materialLibraryNode();
        if (!matLib.isValid())
            return;

        m_bundleMaterialTargets = nodeList;

        ModelNode defaultMat = getBundleMaterialDefaultInstance(m_draggedBundleMaterial->type());
        if (defaultMat.isValid()) {
            if (m_bundleMaterialTargets.isEmpty()) // if no drop target, create a duplicate material
                createMaterial(defaultMat.metaInfo());
            else
                applyBundleMaterialToDropTarget(defaultMat);
        } else {
            m_widget->materialsModel()->addToProject(m_draggedBundleMaterial);
        }

        m_draggedBundleMaterial = nullptr;
    }
}

void ContentLibraryView::applyBundleMaterialToDropTarget(const ModelNode &bundleMat,
                                                         const NodeMetaInfo &metaInfo)
{
    if (!bundleMat.isValid() && !metaInfo.isValid())
        return;

    executeInTransaction("ContentLibraryView::applyBundleMaterialToDropTarget", [&] {
        ModelNode newMatNode = metaInfo.isValid() ? createMaterial(metaInfo) : bundleMat;

        // TODO: unify this logic as it exist elsewhere also
        auto expToList = [](const QString &exp) {
            QString copy = exp;
            copy = copy.remove("[").remove("]");

            QStringList tmp = copy.split(',', Qt::SkipEmptyParts);
            for (QString &str : tmp)
                str = str.trimmed();

            return tmp;
        };

        auto listToExp = [](QStringList &stringList) {
            if (stringList.size() > 1)
                return QString("[" + stringList.join(",") + "]");

            if (stringList.size() == 1)
                return stringList.first();

            return QString();
        };

        for (const ModelNode &target : std::as_const(m_bundleMaterialTargets)) {
            if (target.isValid() && target.metaInfo().isQtQuick3DModel()) {
                QmlObjectNode qmlObjNode(target);
                if (m_bundleMaterialAddToSelected) {
                    QStringList matList = expToList(qmlObjNode.expression("materials"));
                    matList.append(newMatNode.id());
                    QString updatedExp = listToExp(matList);
                    qmlObjNode.setBindingProperty("materials", updatedExp);
                } else {
                    qmlObjNode.setBindingProperty("materials", newMatNode.id());
                }
            }

            m_bundleMaterialTargets = {};
            m_bundleMaterialAddToSelected = false;
        }
    });
}

ModelNode ContentLibraryView::getBundleMaterialDefaultInstance(const TypeName &type)
{
    ModelNode matLib = materialLibraryNode();
    if (!matLib.isValid())
        return {};

    const QList <ModelNode> matLibNodes = matLib.directSubModelNodes();
    for (const ModelNode &mat : matLibNodes) {
        if (mat.isValid() && mat.type() == type) {
            bool isDefault = true;
            const QList<AbstractProperty> props = mat.properties();
            for (const AbstractProperty &prop : props) {
                if (prop.name() != "objectName") {
                    isDefault = false;
                    break;
                }
            }

            if (isDefault)
                return mat;
        }
    }

    return {};
}

ModelNode ContentLibraryView::createMaterial(const NodeMetaInfo &metaInfo)
{
    ModelNode matLib = materialLibraryNode();
    if (!matLib.isValid() || !metaInfo.isValid())
        return {};

    ModelNode newMatNode = createModelNode(metaInfo.typeName(), metaInfo.majorVersion(),
                                                                metaInfo.minorVersion());
    matLib.defaultNodeListProperty().reparentHere(newMatNode);

    static QRegularExpression rgx("([A-Z])([a-z]*)");
    QString newName = QString::fromLatin1(metaInfo.simplifiedTypeName()).replace(rgx, " \\1\\2").trimmed();
    if (newName.endsWith(" Material"))
        newName.chop(9); // remove trailing " Material"
    QString newId = model()->generateIdFromName(newName, "material");
    newMatNode.setIdWithRefactoring(newId);

    VariantProperty objNameProp = newMatNode.variantProperty("objectName");
    objNameProp.setValue(newName);

    return newMatNode;
}

void ContentLibraryView::updateBundleMaterialsImportedState()
{
    using namespace Utils;

    if (!m_widget->materialsModel()->bundleImporter())
        return;

    QStringList importedBundleMats;

    FilePath materialBundlePath = m_widget->materialsModel()->bundleImporter()->resolveBundleImportPath();

    if (materialBundlePath.exists()) {
        importedBundleMats = transform(materialBundlePath.dirEntries({{"*.qml"}, QDir::Files}),
                                       [](const FilePath &f) { return f.fileName().chopped(4); });
    }

    m_widget->materialsModel()->updateImportedState(importedBundleMats);
}

void ContentLibraryView::updateBundleMaterialsQuick3DVersion()
{
    bool hasImport = false;
    int major = -1;
    int minor = -1;
    const QString url {"QtQuick3D"};
    const auto imports = model()->imports();
    for (const auto &import : imports) {
        if (import.url() == url) {
            hasImport = true;
            const int importMajor = import.majorVersion();
            if (major < importMajor) {
                minor = -1;
                major = importMajor;
            }
            if (major == importMajor)
                minor = qMax(minor, import.minorVersion());
        }
    }
#ifndef QMLDESIGNER_TEST
    if (hasImport && major == -1) {
        // Import without specifying version, so we take the kit version
        auto target = ProjectExplorer::SessionManager::startupTarget();
        if (target) {
            QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
            if (qtVersion) {
                major = qtVersion->qtVersion().majorVersion();
                minor = qtVersion->qtVersion().minorVersion();
            }
        }
    }
#endif
    m_widget->materialsModel()->setQuick3DImportVersion(major, minor);
}

} // namespace QmlDesigner
