// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "formeditoritem.h"
#include "rewritertransaction.h"
#include "snapper.h"
#include "formeditorview.h"

#include <QPointer>
#include <QGraphicsItem>
#include <QHash>
#include <QPointF>
#include <QRectF>

namespace QmlDesigner {

class ModelNodeChangeSet;
class Model;

class MoveManipulator
{
public:
    enum State {
        UseCurrentState,
        UseBaseState
    };

    enum ReparentFlag {
        DoNotEnforceReparent,
        EnforceReparent
    };

    MoveManipulator(LayerItem *layerItem, FormEditorView *view);
    ~MoveManipulator();
    void setItems(const QList<FormEditorItem*> &itemList);
    void setItem(FormEditorItem* item);
    void synchronizeInstanceParent(const QList<FormEditorItem*> &itemList);
    void synchronizeParent(const QList<FormEditorItem*> &itemList, const ModelNode &parentNode);

    void begin(const QPointF& beginPoint);
    void update(const QPointF& updatePoint, Snapper::Snapping useSnapping, State stateToBeManipulated = UseCurrentState);
    void reparentTo(FormEditorItem *newParent, ReparentFlag flag = DoNotEnforceReparent);
    void end();
    void end(Snapper::Snapping useSnapping);

    void moveBy(double deltaX, double deltaY);

    void beginRewriterTransaction();
    void endRewriterTransaction();

    QPointF beginPoint() const;

    void clear();

    bool isActive() const;

protected:
    void setOpacityForAllElements(qreal opacity);

    QPointF findSnappingOffset(const QHash<FormEditorItem*, QRectF> &boundingRectHash);

    void deleteSnapLines();

    QHash<FormEditorItem*, QRectF> tanslatedBoundingRects(const QHash<FormEditorItem*, QRectF> &boundingRectHash,
                                                          const QPointF& offset,
                                                          const QTransform &transform);
    QPointF calculateBoundingRectMovementOffset(const QPointF& updatePoint);
    QRectF boundingRect(FormEditorItem* item, const QPointF& updatePoint);

    void generateSnappingLines(const QHash<FormEditorItem*, QRectF> &boundingRectHash);

    bool itemsCanReparented() const;

    void adjustAnchoringOfItem(FormEditorItem *item);

    void setDirectUpdateInNodeInstances(bool directUpdate);

private:
    Snapper m_snapper;
    QPointer<LayerItem> m_layerItem;
    QPointer<FormEditorView> m_view;
    QList<FormEditorItem*> m_itemList;
    QHash<FormEditorItem*, QRectF> m_beginItemRectInSceneSpaceHash;
    QHash<FormEditorItem*, QPointF> m_beginPositionInSceneSpaceHash;
    QPointF m_beginPoint;
    QHash<FormEditorItem*, double> m_beginTopMarginHash;
    QHash<FormEditorItem*, double> m_beginLeftMarginHash;
    QHash<FormEditorItem*, double> m_beginRightMarginHash;
    QHash<FormEditorItem*, double> m_beginBottomMarginHash;
    QHash<FormEditorItem*, double> m_beginHorizontalCenterHash;
    QHash<FormEditorItem*, double> m_beginVerticalCenterHash;
    QList<QGraphicsItem*> m_graphicsLineList;
    bool m_isActive;
    RewriterTransaction m_rewriterTransaction;
    QPointF m_lastPosition;
};

} // namespace QmlDesigner
