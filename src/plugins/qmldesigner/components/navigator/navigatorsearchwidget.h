// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace QmlDesigner {

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    LineEdit(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void updateClearButton(const QString &text);

private:
    QToolButton *clearButton;
};

class NavigatorSearchWidget : public QWidget
{
    Q_OBJECT

public:
    NavigatorSearchWidget(QWidget *parent = nullptr);

    void clear();

signals:
    void textChanged(const QString &text);

private:
    LineEdit *m_textField;
};

} //QmlDesigner
