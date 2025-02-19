// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "changenumberdialog.h"

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QIntValidator>
#include <QLineEdit>

using namespace Perforce::Internal;

ChangeNumberDialog::ChangeNumberDialog(QWidget *parent)
    : QDialog(parent)
    , m_lineEdit(new QLineEdit(this))
{
    setWindowTitle(tr("Change Number"));

    m_lineEdit->setValidator(new QIntValidator(0, 1000000, this));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Utils::Layouting;

    Column {
        Row { tr("Change number:"), m_lineEdit },
        buttonBox
    }.attachTo(this);

    resize(320, 75);
}

int ChangeNumberDialog::number() const
{
    bool ok = false;
    const int number = m_lineEdit->text().toInt(&ok);
    return ok ? number : -1;
}
