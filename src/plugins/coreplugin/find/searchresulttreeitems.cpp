// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "searchresulttreeitems.h"

namespace Core {
namespace Internal {

SearchResultTreeItem::SearchResultTreeItem(const SearchResultItem &item,
                                           SearchResultTreeItem *parent)
  : item(item),
  m_parent(parent),
  m_isGenerated(false),
  m_checkState(item.selectForReplacement() ? Qt::Checked : Qt::Unchecked)
{
}

SearchResultTreeItem::~SearchResultTreeItem()
{
    clearChildren();
}

bool SearchResultTreeItem::isLeaf() const
{
    return childrenCount() == 0 && parent() != nullptr;
}

Qt::CheckState SearchResultTreeItem::checkState() const
{
    return m_checkState;
}

void SearchResultTreeItem::setCheckState(Qt::CheckState checkState)
{
    m_checkState = checkState;
}

void SearchResultTreeItem::clearChildren()
{
    qDeleteAll(m_children);
    m_children.clear();
}

int SearchResultTreeItem::childrenCount() const
{
    return m_children.count();
}

int SearchResultTreeItem::rowOfItem() const
{
    return (m_parent ? m_parent->m_children.indexOf(const_cast<SearchResultTreeItem*>(this)):0);
}

SearchResultTreeItem* SearchResultTreeItem::childAt(int index) const
{
    return m_children.at(index);
}

SearchResultTreeItem *SearchResultTreeItem::parent() const
{
    return m_parent;
}

static bool lessThanByText(SearchResultTreeItem *a, const QString &b)
{
    return a->item.lineText() < b;
}

int SearchResultTreeItem::insertionIndex(const QString &text, SearchResultTreeItem **existingItem) const
{
    QList<SearchResultTreeItem *>::const_iterator insertionPosition =
            std::lower_bound(m_children.begin(), m_children.end(), text, lessThanByText);
    if (existingItem) {
        if (insertionPosition != m_children.end() && (*insertionPosition)->item.lineText() == text)
            (*existingItem) = (*insertionPosition);
        else
            *existingItem = nullptr;
    }
    return insertionPosition - m_children.begin();
}

int SearchResultTreeItem::insertionIndex(const SearchResultItem &item, SearchResultTreeItem **existingItem) const
{
    return insertionIndex(item.lineText(), existingItem);
}

void SearchResultTreeItem::insertChild(int index, SearchResultTreeItem *child)
{
    m_children.insert(index, child);
}

void SearchResultTreeItem::insertChild(int index, const SearchResultItem &item)
{
    auto child = new SearchResultTreeItem(item, this);
    insertChild(index, child);
}

void SearchResultTreeItem::appendChild(const SearchResultItem &item)
{
    insertChild(m_children.count(), item);
}

} // namespace Internal
} // namespace Core
