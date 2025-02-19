// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "perforcesubmiteditorwidget.h"

#include <utils/layoutbuilder.h>

#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>

using namespace Utils;

namespace Perforce::Internal {

class SubmitPanel : public QGroupBox
{
public:
    SubmitPanel()
        : m_changeNumber(createLabel())
        , m_clientName(createLabel())
        , m_userName(createLabel())
    {
        resize(402, 134);
        setFlat(true);
        setTitle(tr("Submit"));

        using namespace Layouting;

        Form {
            tr("Change:"), m_changeNumber, br,
            tr("Client:"), m_clientName, br,
            tr("User:"), m_userName
        }.attachTo(this);
    }

    QLabel *createLabel()
    {
        QLabel *label = new QLabel(this);
        label->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);
        return label;
    }

    QLabel *m_changeNumber = nullptr;
    QLabel *m_clientName = nullptr;
    QLabel *m_userName = nullptr;
};

PerforceSubmitEditorWidget::PerforceSubmitEditorWidget()
    : m_submitPanel(new SubmitPanel)
{
    insertTopWidget(m_submitPanel);
}

void PerforceSubmitEditorWidget::setData(const QString &change,
                                         const QString &client,
                                         const QString &userName)
{
    m_submitPanel->m_changeNumber->setText(change);
    m_submitPanel->m_clientName->setText(client);
    m_submitPanel->m_userName->setText(userName);
}

} // Perforce::Internal
