// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "pendingchangesdialog.h"

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QIntValidator>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>

using namespace Perforce::Internal;

PendingChangesDialog::PendingChangesDialog(const QString &data, QWidget *parent)
    : QDialog(parent)
    , m_listWidget(new QListWidget(this))
{
    setWindowTitle(tr("P4 Pending Changes"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    QPushButton *submitButton = buttonBox->addButton(tr("Submit"), QDialogButtonBox::AcceptRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (!data.isEmpty()) {
        const QRegularExpression r(QLatin1String("Change\\s(\\d+?).*?\\s\\*?pending\\*?\\s(.+?)\n"));
        QListWidgetItem *item;
        QRegularExpressionMatchIterator it = r.globalMatch(data);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            item = new QListWidgetItem(tr("Change %1: %2").arg(match.captured(1),
                                                               match.captured(2).trimmed()),
                                       m_listWidget);
            item->setData(Qt::UserRole, match.captured(1).trimmed());
        }
    }
    m_listWidget->setSelectionMode(QListWidget::SingleSelection);
    if (m_listWidget->count()) {
        m_listWidget->setCurrentRow(0);
        submitButton->setEnabled(true);
    } else {
        submitButton->setEnabled(false);
    }

    using namespace Utils::Layouting;

    Column {
        m_listWidget,
        buttonBox
    }.attachTo(this);

    resize(320, 250);
}

int PendingChangesDialog::changeNumber() const
{
    QListWidgetItem *item = m_listWidget->item(m_listWidget->currentRow());
    if (!item)
        return -1;
    bool ok = true;
    const int number = item->data(Qt::UserRole).toInt(&ok);
    return ok ? number : -1;
}
