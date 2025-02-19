// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "floatingwidgettitlebar.h"

#include "ads_globals.h"
#include "elidinglabel.h"
#include "floatingdockcontainer.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>

#include <iostream>

namespace ADS {

using TabLabelType = ElidingLabel;
using CloseButtonType = QToolButton;

/**
 * @brief Private data class of public interface CFloatingWidgetTitleBar
 */
class FloatingWidgetTitleBarPrivate
{
public:
    FloatingWidgetTitleBar *q; ///< public interface class
    QLabel *m_iconLabel = nullptr;
    TabLabelType *m_titleLabel = nullptr;
    CloseButtonType *m_closeButton = nullptr;
    FloatingDockContainer *m_floatingWidget = nullptr;
    eDragState m_dragState = DraggingInactive;

    FloatingWidgetTitleBarPrivate(FloatingWidgetTitleBar *parent)
        : q(parent)
    {}

    /**
      * Creates the complete layout including all controls
      */
    void createLayout();
};

void FloatingWidgetTitleBarPrivate::createLayout()
{
    m_titleLabel = new TabLabelType();
    m_titleLabel->setElideMode(Qt::ElideRight);
    m_titleLabel->setText("DockWidget->windowTitle()");
    m_titleLabel->setObjectName("floatingTitleLabel");
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_closeButton = new CloseButtonType();
    m_closeButton->setObjectName("floatingTitleCloseButton");
    m_closeButton->setAutoRaise(true);
    internal::setButtonIcon(m_closeButton,
                            QStyle::SP_TitleBarCloseButton,
                            ADS::FloatingWidgetCloseIcon);
    m_closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_closeButton->setIconSize(QSize(11, 11));
    m_closeButton->setFixedSize(QSize(17, 17));
    m_closeButton->setVisible(true);
    m_closeButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect(m_closeButton,
                     &QPushButton::clicked,
                     q,
                     &FloatingWidgetTitleBar::closeRequested);

    QFontMetrics fontMetrics(m_titleLabel->font());
    int spacing = qRound(fontMetrics.height() / 4.0);

    // Fill the layout
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->setContentsMargins(6, 0, 0, 0);
    layout->setSpacing(0);
    q->setLayout(layout);
    layout->addWidget(m_titleLabel, 1);
    layout->addSpacing(spacing);
    layout->addWidget(m_closeButton);
    layout->addSpacing(1);
    layout->setAlignment(Qt::AlignCenter);

    m_titleLabel->setVisible(true);
}

FloatingWidgetTitleBar::FloatingWidgetTitleBar(FloatingDockContainer *parent)
    : QFrame(parent)
    , d(new FloatingWidgetTitleBarPrivate(this))
{
    d->m_floatingWidget = parent;
    d->createLayout();
}

FloatingWidgetTitleBar::~FloatingWidgetTitleBar()
{
    delete d;
}

void FloatingWidgetTitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->m_dragState = DraggingFloatingWidget;
        d->m_floatingWidget->startDragging(event->pos(), d->m_floatingWidget->size(), this);
        return;
    }
    Super::mousePressEvent(event);
}

void FloatingWidgetTitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    d->m_dragState = DraggingInactive;
    if (d->m_floatingWidget)
        d->m_floatingWidget->finishDragging();

    Super::mouseReleaseEvent(event);
}

void FloatingWidgetTitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || DraggingInactive == d->m_dragState) {
        d->m_dragState = DraggingInactive;
        Super::mouseMoveEvent(event);
        return;
    }

    // move floating window
    if (DraggingFloatingWidget == d->m_dragState) {
        d->m_floatingWidget->moveFloating();
        Super::mouseMoveEvent(event);
        return;
    }
    Super::mouseMoveEvent(event);
}

void FloatingWidgetTitleBar::enableCloseButton(bool enable)
{
    d->m_closeButton->setEnabled(enable);
}

void FloatingWidgetTitleBar::setTitle(const QString &text)
{
    d->m_titleLabel->setText(text);
}

void FloatingWidgetTitleBar::updateStyle()
{
    internal::repolishStyle(this, internal::RepolishDirectChildren);
}

} // namespace ADS
