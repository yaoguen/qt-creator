// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "uvtargetdrivermodel.h"

#include <baremetal/baremetaltr.h>

#include <QFile>
#include <QTextStream>

using namespace Utils;

namespace BareMetal::Internal::Uv {

constexpr char cpuDllKey[] = "CPUDLL";
constexpr char driverKey[] = "TDRV";

struct Dll
{
    int index = -1;
    QString mnemonic;
    QString path;
    QString content;
};
using Dlls = std::vector<Dll>;

// Parse the lines in a forms like:
// "CPUDLL0=SARM.DLL(TDRV16,TDRV17,TDRV18)"
// "TDRV1=BIN\ULP2CM3.DLL("ULINK Pro Cortex Debugger")"
static Dll extractDll(const QString &line, const QString &pattern)
{
    const int equalIndex = line.indexOf('=');
    const int leftBracketIndex = line.indexOf('(', equalIndex + 1);
    const int rightBracketIndex = line.indexOf(')', leftBracketIndex + 1);
    if (equalIndex < 0 || leftBracketIndex < 0 || rightBracketIndex < 0)
        return {};

    Dll dll;
    dll.index = line.mid(pattern.size(), equalIndex - pattern.size()).toInt();
    dll.mnemonic = line.mid(0, equalIndex).trimmed();
    dll.path = line.mid(equalIndex + 1, leftBracketIndex - equalIndex - 1).trimmed();
    dll.content = line.mid(leftBracketIndex + 1,
                           rightBracketIndex - leftBracketIndex - 1).trimmed();
    return dll;
}

static bool collectCpuDllsAndDrivers(QIODevice *file, Dlls &allCpuDlls, Dlls &allDrivers)
{
    QTextStream in(file);
    bool armadsFound = false;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!armadsFound) {
            if (line == "[ARMADS]")
                armadsFound = true;
        } else if (line.startsWith('[') && line.endsWith(']')) {
            break;
        } else if (line.startsWith(cpuDllKey)) {
            const Dll dll = extractDll(line, cpuDllKey);
            if (dll.index >= 0)
                allCpuDlls.push_back(dll);
        } else if (line.startsWith(driverKey)) {
            const Dll dll = extractDll(line, driverKey);
            if (dll.index >= 0)
                allDrivers.push_back(dll);
        }
    }
    return armadsFound;
}

// DriverSelectionItem

class DriverSelectionItem final : public TreeItem
{
public:
    enum Column { PathColumn };
    explicit DriverSelectionItem(int index = -1)
        : m_index(index)
    {}

    DriverSelection toSelection() const
    {
        DriverSelection selection;
        selection.index = m_index;
        selection.dll = m_dll;
        selection.name = m_name;
        selection.cpuDlls = m_cpuDlls;
        return selection;
    }

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole && column == PathColumn)
            return m_name;
        return {};
    }

    const int m_index;
    QString m_name;
    QString m_dll;
    QStringList m_cpuDlls;
};

// DriverSelectionModel

DriverSelectionModel::DriverSelectionModel(QObject *parent)
    : TreeModel<DriverSelectionItem>(parent)
{
    setHeader({Tr::tr("Path")});
}

void DriverSelectionModel::fillDrivers(const FilePath &toolsIniFile,
                                       const QStringList &supportedDrivers)
{
    clear();

    QFile f(toolsIniFile.toString());
    if (!f.open(QIODevice::ReadOnly))
        return;

    Dlls allCpuDlls;
    Dlls allDrivers;
    if (!collectCpuDllsAndDrivers(&f, allCpuDlls, allDrivers))
        return;

    for (const Dll &dll : std::as_const(allDrivers)) {
        if (!supportedDrivers.contains(dll.path))
            continue;
        const auto item = new DriverSelectionItem(dll.index);
        item->m_dll = dll.path;
        item->m_name = dll.content;
        for (const Dll &cpu : std::as_const(allCpuDlls)) {
            const QStringList mnemonics = cpu.content.split(',');
            if (mnemonics.contains(dll.mnemonic))
                item->m_cpuDlls.push_back(cpu.path);
        }
        rootItem()->appendChild(item);
    }
}

// DriverSelectionView

DriverSelectionView::DriverSelectionView(QWidget *parent)
    : TreeView(parent)
{
    setRootIsDecorated(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void DriverSelectionView::setCurrentSelection(const QString &driverDll)
{
    const auto selectionModel = qobject_cast<DriverSelectionModel *>(model());
    if (!selectionModel)
        return;

    const DriverSelectionItem *item = selectionModel->findNonRootItem(
                [driverDll](const DriverSelectionItem *item) {
        return item->m_dll == driverDll;
    });
    if (!item)
        return;

    const QModelIndex index = selectionModel->indexForItem(item);
    setCurrentIndex(index);
}

void DriverSelectionView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    if (!current.isValid())
        return;
    const auto selectionModel = qobject_cast<DriverSelectionModel *>(model());
    if (!selectionModel)
        return;
    const DriverSelectionItem *item = selectionModel->itemForIndex(current);
    if (!item)
        return;
    const auto selection = item->toSelection();
    if (selection.index >= 0)
        emit driverSelected(selection);
}

} // BareMetal::Internal::Uv
