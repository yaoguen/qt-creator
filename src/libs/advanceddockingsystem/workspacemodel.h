// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include <QAbstractTableModel>

#include <functional>

namespace ADS {

class DockManager;
class WorkspaceNameInputDialog;

class WorkspaceModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum { PresetWorkspaceRole = Qt::UserRole + 1, LastWorkspaceRole, ActiveWorkspaceRole };

    explicit WorkspaceModel(DockManager *manager, QObject *parent = nullptr);

    int indexOfWorkspace(const QString &workspace);
    QString workspaceAt(int row) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

signals:
    void workspaceSwitched();
    void workspaceCreated(const QString &workspaceName);

public:
    void resetWorkspaces();
    void newWorkspace(QWidget *parent);
    void cloneWorkspace(QWidget *parent, const QString &workspace);
    void deleteWorkspaces(const QStringList &workspaces);
    void renameWorkspace(QWidget *parent, const QString &workspace);
    void resetWorkspace(const QString &workspace);
    void switchToWorkspace(const QString &workspace);

    void importWorkspace(const QString &workspace);
    void exportWorkspace(const QString &target, const QString &workspace);

private:
    void runWorkspaceNameInputDialog(WorkspaceNameInputDialog *workspaceInputDialog,
                                     std::function<void(const QString &)> createWorkspace);

    QStringList m_sortedWorkspaces;
    DockManager *m_manager;
    int m_currentSortColumn;
    Qt::SortOrder m_currentSortOrder = Qt::AscendingOrder;
};

} // namespace ADS
