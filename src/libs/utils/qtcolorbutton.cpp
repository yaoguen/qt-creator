// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qtcolorbutton.h"

#include <QMimeData>
#include <QApplication>
#include <QColorDialog>
#include <QDragEnterEvent>
#include <QPainter>
#include <QDrag>

namespace Utils {

class QtColorButtonPrivate: public QObject
{
    Q_OBJECT
    QtColorButton *q_ptr;
    Q_DECLARE_PUBLIC(QtColorButton)
public slots:
    void slotEditColor();

public:
    QColor shownColor() const;
    QPixmap generatePixmap() const;

    QColor m_color;
#ifndef QT_NO_DRAGANDDROP
    QColor m_dragColor;
    QPoint m_dragStart;
    bool m_dragging;
#endif
    bool m_backgroundCheckered;
    bool m_alphaAllowed;
    bool m_dialogOpen;
};

void QtColorButtonPrivate::slotEditColor()
{
    QColorDialog::ColorDialogOptions options;
    if (m_alphaAllowed)
        options |= QColorDialog::ShowAlphaChannel;
    emit q_ptr->colorChangeStarted();
    m_dialogOpen = true;
    const QColor newColor = QColorDialog::getColor(m_color, q_ptr, QString(), options);
    m_dialogOpen = false;
    if (!newColor.isValid() || newColor == q_ptr->color()) {
        emit q_ptr->colorUnchanged();
        return;
    }
    q_ptr->setColor(newColor);
    emit q_ptr->colorChanged(m_color);
}

QColor QtColorButtonPrivate::shownColor() const
{
#ifndef QT_NO_DRAGANDDROP
    if (m_dragging)
        return m_dragColor;
#endif
    return m_color;
}

QPixmap QtColorButtonPrivate::generatePixmap() const
{
    QPixmap pix(24, 24);

    int pixSize = 20;
    QBrush br(shownColor());

    QPixmap pm(2 * pixSize, 2 * pixSize);
    QPainter pmp(&pm);
    pmp.fillRect(0, 0, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::lightGray);
    pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::darkGray);
    pmp.fillRect(0, 0, 2 * pixSize, 2 * pixSize, shownColor());
    br = QBrush(pm);

    QPainter p(&pix);
    int corr = 1;
    QRect r = pix.rect().adjusted(corr, corr, -corr, -corr);
    p.setBrushOrigin((r.width() % pixSize + pixSize) / 2 + corr, (r.height() % pixSize + pixSize) / 2 + corr);
    p.fillRect(r, br);

    p.fillRect(r.width() / 4 + corr, r.height() / 4 + corr,
               r.width() / 2, r.height() / 2,
               QColor(shownColor().rgb()));
    p.drawRect(pix.rect().adjusted(0, 0, -1, -1));

    return pix;
}

///////////////

QtColorButton::QtColorButton(QWidget *parent)
    : QToolButton(parent)
{
    d_ptr = new QtColorButtonPrivate;
    d_ptr->q_ptr = this;
    d_ptr->m_dragging = false;
    d_ptr->m_backgroundCheckered = true;
    d_ptr->m_alphaAllowed = true;
    d_ptr->m_dialogOpen = false;

    setAcceptDrops(true);

    connect(this, &QtColorButton::clicked, d_ptr, &QtColorButtonPrivate::slotEditColor);
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
}

QtColorButton::~QtColorButton()
{
    delete d_ptr;
}

void QtColorButton::setColor(const QColor &color)
{
    if (d_ptr->m_color == color)
        return;
    d_ptr->m_color = color;
    update();
}

QColor QtColorButton::color() const
{
    return d_ptr->m_color;
}

void QtColorButton::setBackgroundCheckered(bool checkered)
{
    if (d_ptr->m_backgroundCheckered == checkered)
        return;
    d_ptr->m_backgroundCheckered = checkered;
    update();
}

bool QtColorButton::isBackgroundCheckered() const
{
    return d_ptr->m_backgroundCheckered;
}

void QtColorButton::setAlphaAllowed(bool allowed)
{
    d_ptr->m_alphaAllowed = allowed;
}

bool QtColorButton::isAlphaAllowed() const
{
    return d_ptr->m_alphaAllowed;
}

bool QtColorButton::isDialogOpen() const
{
    return d_ptr->m_dialogOpen;
}

void QtColorButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);
    if (!isEnabled())
        return;

    const int pixSize = 10;
    QBrush br(d_ptr->shownColor());
    if (d_ptr->m_backgroundCheckered) {
        QPixmap pm(2 * pixSize, 2 * pixSize);
        QPainter pmp(&pm);
        pmp.fillRect(0, 0, pixSize, pixSize, Qt::white);
        pmp.fillRect(pixSize, pixSize, pixSize, pixSize, Qt::white);
        pmp.fillRect(0, pixSize, pixSize, pixSize, Qt::black);
        pmp.fillRect(pixSize, 0, pixSize, pixSize, Qt::black);
        pmp.fillRect(0, 0, 2 * pixSize, 2 * pixSize, d_ptr->shownColor());
        br = QBrush(pm);
    }

    QPainter p(this);
    const int corr = 5;
    QRect r = rect().adjusted(corr, corr, -corr, -corr);
    p.setBrushOrigin((r.width() % pixSize + pixSize) / 2 + corr, (r.height() % pixSize + pixSize) / 2 + corr);
    p.fillRect(r, br);

    //const int adjX = qRound(r.width() / 4.0);
    //const int adjY = qRound(r.height() / 4.0);
    //p.fillRect(r.adjusted(adjX, adjY, -adjX, -adjY),
    //           QColor(d_ptr->shownColor().rgb()));
    /*
    p.fillRect(r.adjusted(0, r.height() * 3 / 4, 0, 0),
               QColor(d_ptr->shownColor().rgb()));
    p.fillRect(r.adjusted(0, 0, 0, -r.height() * 3 / 4),
               QColor(d_ptr->shownColor().rgb()));
               */
    /*
    const QColor frameColor0(0, 0, 0, qRound(0.2 * (0xFF - d_ptr->shownColor().alpha())));
    p.setPen(frameColor0);
    p.drawRect(r.adjusted(adjX, adjY, -adjX - 1, -adjY - 1));
    */

    const QColor frameColor1(0, 0, 0, 26);
    p.setPen(frameColor1);
    p.drawRect(r.adjusted(1, 1, -2, -2));
    const QColor frameColor2(0, 0, 0, 51);
    p.setPen(frameColor2);
    p.drawRect(r.adjusted(0, 0, -1, -1));
}

void QtColorButton::mousePressEvent(QMouseEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    if (event->button() == Qt::LeftButton)
        d_ptr->m_dragStart = event->pos();
#endif
    QToolButton::mousePressEvent(event);
}

void QtColorButton::mouseMoveEvent(QMouseEvent *event)
{
#ifndef QT_NO_DRAGANDDROP
    if (event->buttons() & Qt::LeftButton &&
            (d_ptr->m_dragStart - event->pos()).manhattanLength() > QApplication::startDragDistance()) {
        auto mime = new QMimeData;
        mime->setColorData(color());
        auto drg = new QDrag(this);
        drg->setMimeData(mime);
        drg->setPixmap(d_ptr->generatePixmap());
        setDown(false);
        event->accept();
        drg->exec(Qt::CopyAction);
        return;
    }
#endif
    QToolButton::mouseMoveEvent(event);
}

#ifndef QT_NO_DRAGANDDROP
void QtColorButton::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime->hasColor())
        return;

    event->accept();
    d_ptr->m_dragColor = qvariant_cast<QColor>(mime->colorData());
    d_ptr->m_dragging = true;
    update();
}

void QtColorButton::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
    d_ptr->m_dragging = false;
    update();
}

void QtColorButton::dropEvent(QDropEvent *event)
{
    event->accept();
    d_ptr->m_dragging = false;
    if (d_ptr->m_dragColor == color())
        return;
    setColor(d_ptr->m_dragColor);
    emit colorChanged(color());
}
#endif

} // namespace Utils

#include "qtcolorbutton.moc"
