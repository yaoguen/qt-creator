// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "timelineabstractrenderer_p.h"

namespace Timeline {

TimelineAbstractRenderer::TimelineAbstractRendererPrivate::TimelineAbstractRendererPrivate() :
    selectedItem(-1), selectionLocked(true), model(nullptr), notes(nullptr), zoomer(nullptr), modelDirty(false),
    rowHeightsDirty(false), notesDirty(false)
{
}

TimelineAbstractRenderer::TimelineAbstractRendererPrivate::~TimelineAbstractRendererPrivate()
{
    // Nothing to delete here as all the pointer members are owned by other classes.
}

TimelineAbstractRenderer::TimelineAbstractRenderer(TimelineAbstractRendererPrivate &dd,
                                                   QQuickItem *parent) :
    QQuickItem(parent), d_ptr(&dd)
{
    setFlag(ItemHasContents);
}

int TimelineAbstractRenderer::selectedItem() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->selectedItem;
}

void TimelineAbstractRenderer::setSelectedItem(int itemIndex)
{
    Q_D(TimelineAbstractRenderer);
    if (d->selectedItem != itemIndex) {
        d->selectedItem = itemIndex;
        update();
        emit selectedItemChanged(itemIndex);
    }
}

TimelineAbstractRenderer::TimelineAbstractRenderer(QQuickItem *parent) : QQuickItem(parent),
    d_ptr(new TimelineAbstractRendererPrivate)
{
    setFlag(ItemHasContents);
}

TimelineAbstractRenderer::~TimelineAbstractRenderer()
{
    Q_D(TimelineAbstractRenderer);
    delete d;
}

bool TimelineAbstractRenderer::selectionLocked() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->selectionLocked;
}

void TimelineAbstractRenderer::setSelectionLocked(bool locked)
{
    Q_D(TimelineAbstractRenderer);
    if (d->selectionLocked != locked) {
        d->selectionLocked = locked;
        update();
        emit selectionLockedChanged(locked);
    }
}

TimelineModel *TimelineAbstractRenderer::model() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->model;
}

void TimelineAbstractRenderer::setModel(TimelineModel *model)
{
    Q_D(TimelineAbstractRenderer);
    if (d->model == model)
        return;

    if (d->model) {
        disconnect(d->model, &TimelineModel::expandedChanged, this, &QQuickItem::update);
        disconnect(d->model, &TimelineModel::hiddenChanged, this, &QQuickItem::update);
        disconnect(d->model, &TimelineModel::expandedRowHeightChanged,
                   this, &TimelineAbstractRenderer::setRowHeightsDirty);
        disconnect(d->model, &TimelineModel::contentChanged,
                   this, &TimelineAbstractRenderer::setModelDirty);
        disconnect(d->model, &QObject::destroyed, this, nullptr);
        d->renderPasses.clear();
    }

    d->model = model;

    if (d->model) {
        connect(d->model, &TimelineModel::expandedChanged, this, &QQuickItem::update);
        connect(d->model, &TimelineModel::hiddenChanged, this, &QQuickItem::update);
        connect(d->model, &TimelineModel::expandedRowHeightChanged,
                this, &TimelineAbstractRenderer::setRowHeightsDirty);
        connect(d->model, &TimelineModel::contentChanged,
                this, &TimelineAbstractRenderer::setModelDirty);
        connect(d->model, &QObject::destroyed, this, [this, d]() {
            // Weak pointers are supposed to be notified before the destroyed() signal is sent.
            Q_ASSERT(d->model.isNull());
            d->renderPasses.clear();
            setModelDirty();
            emit modelChanged(d->model);
        });
        d->renderPasses = d->model->supportedRenderPasses();
    }

    setModelDirty();
    emit modelChanged(d->model);
}

TimelineNotesModel *TimelineAbstractRenderer::notes() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->notes;
}

void TimelineAbstractRenderer::setNotes(TimelineNotesModel *notes)
{
    Q_D(TimelineAbstractRenderer);
    if (d->notes == notes)
        return;

    if (d->notes) {
        disconnect(d->notes, &TimelineNotesModel::changed,
                   this, &TimelineAbstractRenderer::setNotesDirty);
        disconnect(d->notes, &QObject::destroyed, this, nullptr);
    }

    d->notes = notes;
    if (d->notes) {
        connect(d->notes, &TimelineNotesModel::changed,
                this, &TimelineAbstractRenderer::setNotesDirty);
        connect(d->notes, &QObject::destroyed, this, [this, d]() {
            // Weak pointers are supposed to be notified before the destroyed() signal is sent.
            Q_ASSERT(d->notes.isNull());
            setNotesDirty();
            emit notesChanged(d->notes);
        });
    }

    setNotesDirty();
    emit notesChanged(d->notes);
}

TimelineZoomControl *TimelineAbstractRenderer::zoomer() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->zoomer;
}

void TimelineAbstractRenderer::setZoomer(TimelineZoomControl *zoomer)
{
    Q_D(TimelineAbstractRenderer);
    if (zoomer != d->zoomer) {
        if (d->zoomer) {
            disconnect(d->zoomer, &TimelineZoomControl::windowChanged, this, &QQuickItem::update);
            disconnect(d->zoomer, &QObject::destroyed, this, nullptr);
        }
        d->zoomer = zoomer;
        if (d->zoomer) {
            connect(d->zoomer, &TimelineZoomControl::windowChanged, this, &QQuickItem::update);
            connect(d->zoomer, &QObject::destroyed, this, [this, d]() {
                // Weak pointers are supposed to be notified before the destroyed() signal is sent.
                Q_ASSERT(d->zoomer.isNull());
                emit zoomerChanged(d->zoomer);
                update();
            });
        }
        emit zoomerChanged(zoomer);
        update();
    }
}

bool TimelineAbstractRenderer::modelDirty() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->modelDirty;
}

bool TimelineAbstractRenderer::notesDirty() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->notesDirty;
}

bool TimelineAbstractRenderer::rowHeightsDirty() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->rowHeightsDirty;
}

void TimelineAbstractRenderer::setModelDirty()
{
    Q_D(TimelineAbstractRenderer);
    if (!d->modelDirty) {
        d->modelDirty = true;
        update();
    }
}

void TimelineAbstractRenderer::setRowHeightsDirty()
{
    Q_D(TimelineAbstractRenderer);
    if (!d->rowHeightsDirty) {
        d->rowHeightsDirty = true;
        update();
    }
}

void TimelineAbstractRenderer::setNotesDirty()
{
    Q_D(TimelineAbstractRenderer);
    if (!d->notesDirty) {
        d->notesDirty = true;
        update();
    }
}

// Reset the dirty flags, delete the old node (if given), and return 0
QSGNode *TimelineAbstractRenderer::updatePaintNode(QSGNode *oldNode,
                                                   UpdatePaintNodeData *updatePaintNodeData)
{
    Q_D(TimelineAbstractRenderer);
    d->modelDirty = false;
    d->rowHeightsDirty = false;
    d->notesDirty = false;
    return QQuickItem::updatePaintNode(oldNode, updatePaintNodeData);
}

} // namespace Timeline
