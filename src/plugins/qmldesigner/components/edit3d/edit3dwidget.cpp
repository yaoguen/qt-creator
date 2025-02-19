// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "edit3dwidget.h"
#include "designdocumentview.h"
#include "edit3dactions.h"
#include "edit3dcanvas.h"
#include "edit3dview.h"
#include "edit3dvisibilitytogglesmenu.h"
#include "metainfo.h"
#include "modelnodeoperations.h"
#include "nodeabstractproperty.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "qmlvisualnode.h"
#include "timelineactions.h"
#include "viewmanager.h"

#include <auxiliarydataproperties.h>
#include <designeractionmanager.h>
#include <import.h>
#include <nodeinstanceview.h>
#include <seekerslider.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <toolbox.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QMimeData>
#include <QVBoxLayout>

namespace QmlDesigner {

Edit3DWidget::Edit3DWidget(Edit3DView *view)
    : m_view(view)
{
    setAcceptDrops(true);

    Core::Context context(Constants::C_QMLEDITOR3D);
    m_context = new Core::IContext(this);
    m_context->setContext(context);
    m_context->setWidget(this);

    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);

    auto fillLayout = new QVBoxLayout(this);
    fillLayout->setContentsMargins(0, 0, 0, 0);
    fillLayout->setSpacing(0);
    setLayout(fillLayout);

    SeekerSlider *seeker = new SeekerSlider(this);
    seeker->setEnabled(false);

    // Initialize toolbar
    m_toolBox = new ToolBox(seeker, this);
    fillLayout->addWidget(m_toolBox.data());

    // Iterate through view actions. A null action indicates a separator and a second null action
    // after separator indicates an exclusive group.
    auto handleActions = [this, &context](const QVector<Edit3DAction *> &actions, QMenu *menu, bool left) {
        bool previousWasSeparator = true;
        QActionGroup *group = nullptr;
        QActionGroup *proxyGroup = nullptr;
        for (auto action : actions) {
            if (action) {
                QAction *a = action->action();
                if (group)
                    group->addAction(a);
                if (menu) {
                    menu->addAction(a);
                } else {
                    addAction(a);
                    if (left)
                        m_toolBox->addLeftSideAction(a);
                    else
                        m_toolBox->addRightSideAction(a);
                }
                previousWasSeparator = false;

                // Register action as creator command to make it configurable
                Core::Command *command = Core::ActionManager::registerAction(
                            a, action->menuId().constData(), context);
                command->setDefaultKeySequence(a->shortcut());
                if (proxyGroup)
                    proxyGroup->addAction(command->action());
                // Menu actions will have custom tooltips
                if (menu)
                    a->setToolTip(command->stringWithAppendedShortcut(a->toolTip()));
                else
                    command->augmentActionWithShortcutToolTip(a);

                // Clear action shortcut so it doesn't conflict with command's override action
                a->setShortcut({});
            } else {
                if (previousWasSeparator) {
                    group = new QActionGroup(this);
                    proxyGroup = new QActionGroup(this);
                    previousWasSeparator = false;
                } else {
                    group = nullptr;
                    proxyGroup = nullptr;
                    auto separator = new QAction(this);
                    separator->setSeparator(true);
                    if (menu) {
                        menu->addAction(separator);
                    } else {
                        addAction(separator);
                        if (left)
                            m_toolBox->addLeftSideAction(separator);
                        else
                            m_toolBox->addRightSideAction(separator);
                    }
                    previousWasSeparator = true;
                }
            }
        }
    };

    handleActions(view->leftActions(), nullptr, true);
    handleActions(view->rightActions(), nullptr, false);

    m_visibilityTogglesMenu = new Edit3DVisibilityTogglesMenu(this);
    handleActions(view->visibilityToggleActions(), m_visibilityTogglesMenu, false);

    m_backgroundColorMenu = new QMenu(this);
    m_backgroundColorMenu->setToolTipsVisible(true);

    handleActions(view->backgroundColorActions(), m_backgroundColorMenu, false);

    createContextMenu();

    view->setSeeker(seeker);
    seeker->setToolTip(QLatin1String("Seek particle system time when paused."));

    QObject::connect(seeker, &SeekerSlider::positionChanged, [seeker, view]() {
        view->emitView3DAction(View3DActionType::ParticlesSeek, seeker->position());
    });

    // Onboarding label contains instructions for new users how to get 3D content into the project
    m_onboardingLabel = new QLabel(this);
    QString labelText =
            tr("Your file does not import Qt Quick 3D.<br><br>"
               "To create a 3D view, add the"
               " <b>QtQuick3D</b>"
               " module in the"
               " <b>Components</b>"
               " view or click"
               " <a href=\"#add_import\"><span style=\"text-decoration:none;color:%1\">here</span></a>"
               ".<br><br>"
               "To import 3D assets, select"
               " <b>+</b>"
               " in the"
               " <b>Assets</b>"
               " view.");
    m_onboardingLabel->setText(labelText.arg(Utils::creatorTheme()->color(Utils::Theme::TextColorLink).name()));
    m_onboardingLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    connect(m_onboardingLabel, &QLabel::linkActivated, this, &Edit3DWidget::linkActivated);
    fillLayout->addWidget(m_onboardingLabel.data());

    // Canvas is used to render the actual edit 3d view
    m_canvas = new Edit3DCanvas(this);
    fillLayout->addWidget(m_canvas.data());
    showCanvas(false);
}

void Edit3DWidget::createContextMenu()
{
    m_contextMenu = new QMenu(this);

    m_editComponentAction = m_contextMenu->addAction(tr("Edit Component"), [&] {
        DocumentManager::goIntoComponent(m_view->singleSelectedModelNode());
    });

    m_editMaterialAction = m_contextMenu->addAction(tr("Edit Material"), [&] {
        SelectionContext selCtx(m_view);
        selCtx.setTargetNode(m_contextMenuTarget);
        ModelNodeOperations::editMaterial(selCtx);
    });

    m_contextMenu->addSeparator();

    m_duplicateAction = m_contextMenu->addAction(tr("Duplicate"), [&] {
        QmlDesignerPlugin::instance()->currentDesignDocument()->duplicateSelected();
    });

    m_copyAction = m_contextMenu->addAction(tr("Copy"), [&] {
        QmlDesignerPlugin::instance()->currentDesignDocument()->copySelected();
    });

    m_pasteAction = m_contextMenu->addAction(tr("Paste"), [&] {
        QmlDesignerPlugin::instance()->currentDesignDocument()->pasteToPosition(m_contextMenuPos3d);
    });

    m_deleteAction = m_contextMenu->addAction(tr("Delete"), [&] {
        view()->executeInTransaction("Edit3DWidget::createContextMenu", [&] {
            for (ModelNode &node : m_view->selectedModelNodes())
                node.destroy();
        });
    });

    m_contextMenu->addSeparator();

    m_fitSelectedAction = m_contextMenu->addAction(tr("Fit Selected Items to View"), [&] {
        view()->emitView3DAction(View3DActionType::FitToView, true);
    });

    m_alignCameraAction = m_contextMenu->addAction(tr("Align Camera to View"), [&] {
        view()->emitView3DAction(View3DActionType::AlignCamerasToView, true);
    });

    m_alignViewAction = m_contextMenu->addAction(tr("Align View to Camera"), [&] {
        view()->emitView3DAction(View3DActionType::AlignViewToCamera, true);
    });

    m_contextMenu->addSeparator();

    m_selectParentAction = m_contextMenu->addAction(tr("Select Parent"), [&] {
        ModelNode parentNode = ModelNode::lowestCommonAncestor(view()->selectedModelNodes());
        if (!parentNode.isValid())
            return;

        if (!parentNode.isRootNode() && view()->isSelectedModelNode(parentNode))
            parentNode = parentNode.parentProperty().parentModelNode();

        view()->setSelectedModelNode(parentNode);
    });

    m_contextMenu->addSeparator();
}

bool Edit3DWidget::isPasteAvailable() const
{
    if (TimelineActions::clipboardContainsKeyframes())
        return false;

    auto pasteModel(DesignDocumentView::pasteToModel(view()->externalDependencies()));
    if (!pasteModel)
        return false;

    DesignDocumentView docView{view()->externalDependencies()};
    pasteModel->attachView(&docView);
    auto rootNode = docView.rootModelNode();

    if (rootNode.type() == "empty")
        return false;

    QList<ModelNode> allNodes;
    if (rootNode.id() == "__multi__selection__")
        allNodes << rootNode.directSubModelNodes();
    else
        allNodes << rootNode;

    bool hasNon3DNode = std::any_of(allNodes.begin(), allNodes.end(), [](const ModelNode &node) {
        return !node.metaInfo().isQtQuick3DNode();
    });

    if (hasNon3DNode)
        return false;

    return true;
}

// Called by the view to update the "create" sub-menu when the Quick3D entries are ready.
void Edit3DWidget::updateCreateSubMenu(const QStringList &keys,
                                       const QHash<QString, QList<ItemLibraryEntry>> &entriesMap)
{
    if (!m_contextMenu)
        return;

    if (m_createSubMenu) {
        m_contextMenu->removeAction(m_createSubMenu->menuAction());
        m_createSubMenu->deleteLater();
    }

    m_nameToEntry.clear();
    m_createSubMenu = m_contextMenu->addMenu(tr("Create"));

    for (const QString &cat : keys) {
        QList<ItemLibraryEntry> entries = entriesMap.value(cat);
        if (entries.isEmpty())
            continue;

        QMenu *catMenu = m_createSubMenu->addMenu(cat);

        std::sort(entries.begin(), entries.end(), [](const ItemLibraryEntry &a, const ItemLibraryEntry &b) {
            return a.name() < b.name();
        });

        for (const ItemLibraryEntry &entry : std::as_const(entries)) {
            QAction *action = catMenu->addAction(entry.name(), this, &Edit3DWidget::onCreateAction);
            action->setData(entry.name());
            m_nameToEntry.insert(entry.name(), entry);
        }
    }
}

// Action triggered from the "create" sub-menu
void Edit3DWidget::onCreateAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action || !m_view || !m_view->model())
        return;

    m_view->executeInTransaction(__FUNCTION__, [&] {
        ItemLibraryEntry entry = m_nameToEntry.value(action->data().toString());
        Import import = Import::createLibraryImport(entry.requiredImport(),
                                                    QString::number(entry.majorVersion())
                                                    + QLatin1Char('.')
                                                    + QString::number(entry.minorVersion()));
        if (!m_view->model()->hasImport(import, true, true))
            m_view->model()->changeImports({import}, {});

        int activeScene = -1;
        auto data = m_view->rootModelNode().auxiliaryData(active3dSceneProperty);
        if (data)
            activeScene = data->toInt();
        auto modelNode = QmlVisualNode::createQml3DNode(m_view, entry,
                                                        activeScene, m_contextMenuPos3d).modelNode();
        QTC_ASSERT(modelNode.isValid(), return);
        m_view->setSelectedModelNode(modelNode);

        // if added node is a Model, assign it a material
        if (modelNode.metaInfo().isQtQuick3DModel())
            m_view->assignMaterialTo3dModel(modelNode);
    });
}

void Edit3DWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_view)
        QmlDesignerPlugin::contextHelp(callback, m_view->contextHelpId());
    else
        callback({});
}

void Edit3DWidget::showCanvas(bool show)
{
    if (!show) {
        QImage emptyImage;
        m_canvas->updateRenderImage(emptyImage);
    }
    m_canvas->setVisible(show);
    m_onboardingLabel->setVisible(!show);
}

QMenu *Edit3DWidget::visibilityTogglesMenu() const
{
    return m_visibilityTogglesMenu.data();
}

void Edit3DWidget::showVisibilityTogglesMenu(bool show, const QPoint &pos)
{
    if (m_visibilityTogglesMenu.isNull())
        return;
    if (show)
        m_visibilityTogglesMenu->popup(pos);
    else
        m_visibilityTogglesMenu->close();
}

QMenu *Edit3DWidget::backgroundColorMenu() const
{
    return m_backgroundColorMenu.data();
}

void Edit3DWidget::showBackgroundColorMenu(bool show, const QPoint &pos)
{
    if (m_backgroundColorMenu.isNull())
        return;
    if (show)
        m_backgroundColorMenu->popup(pos);
    else
        m_backgroundColorMenu->close();
}

void Edit3DWidget::showContextMenu(const QPoint &pos, const ModelNode &modelNode, const QVector3D &pos3d)
{
    m_contextMenuTarget = modelNode;
    m_contextMenuPos3d = pos3d;

    const bool isValid = modelNode.isValid();
    const bool isModel = modelNode.metaInfo().isQtQuick3DModel();
    const bool isCamera = isValid && modelNode.metaInfo().isQtQuick3DCamera();
    const bool isSingleComponent = view()->hasSingleSelectedModelNode() && modelNode.isComponent();
    const bool anyNodeSelected = view()->hasSelectedModelNodes();
    const bool selectionExcludingRoot = anyNodeSelected && !view()->rootModelNode().isSelected();

    m_editComponentAction->setEnabled(isSingleComponent);
    m_editMaterialAction->setEnabled(isModel);
    m_duplicateAction->setEnabled(selectionExcludingRoot);
    m_copyAction->setEnabled(selectionExcludingRoot);
    m_pasteAction->setEnabled(isPasteAvailable());
    m_deleteAction->setEnabled(selectionExcludingRoot);
    m_fitSelectedAction->setEnabled(anyNodeSelected);
    m_alignCameraAction->setEnabled(isCamera);
    m_alignViewAction->setEnabled(isCamera);
    m_selectParentAction->setEnabled(selectionExcludingRoot);

    m_contextMenu->popup(mapToGlobal(pos));
}

void Edit3DWidget::linkActivated([[maybe_unused]] const QString &link)
{
    if (m_view)
        m_view->addQuick3DImport();
}

Edit3DCanvas *Edit3DWidget::canvas() const
{
    return m_canvas.data();
}

Edit3DView *Edit3DWidget::view() const
{
    return m_view.data();
}

void Edit3DWidget::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData())
        || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_MATERIAL)
        || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)) {
        dragEnterEvent->acceptProposedAction();
    }
}

void Edit3DWidget::dropEvent(QDropEvent *dropEvent)
{
    const QPointF pos = m_canvas->mapFrom(this, dropEvent->position());

    // handle dropping materials
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_MATERIAL)) {
        QByteArray data = dropEvent->mimeData()->data(Constants::MIME_TYPE_MATERIAL);
        QDataStream stream(data);
        qint32 internalId;
        stream >> internalId;

        if (ModelNode matNode = m_view->modelNodeForInternalId(internalId))
            m_view->dropMaterial(matNode, pos);
        return;
    }

    // handle dropping bundle materials
    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)) {
        m_view->dropBundleMaterial(pos);
        return;
    }

    // handle dropping external assets
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    QHash<QString, QStringList> addedAssets = actionManager.handleExternalAssetsDrop(dropEvent->mimeData());

    view()->executeInTransaction("Edit3DWidget::dropEvent", [&] {
        // add 3D assets to 3d editor (QtQuick3D import will be added if missing)
        ItemLibraryInfo *itemLibInfo = m_view->model()->metaInfo().itemLibraryInfo();

        const QStringList added3DAssets = addedAssets.value(ComponentCoreConstants::add3DAssetsDisplayString);
        for (const QString &assetPath : added3DAssets) {
            QString fileName = QFileInfo(assetPath).baseName();
            fileName = fileName.at(0).toUpper() + fileName.mid(1); // capitalize first letter
            QString type = QString("Quick3DAssets.%1.%1").arg(fileName);
            QList<ItemLibraryEntry> entriesForType = itemLibInfo->entriesForType(type.toLatin1());
            if (!entriesForType.isEmpty()) { // should always be true, but just in case
                QmlVisualNode::createQml3DNode(view(), entriesForType.at(0),
                                               m_canvas->activeScene(), {}, false).modelNode();
            }
        }
    });
}

} // namespace QmlDesigner
