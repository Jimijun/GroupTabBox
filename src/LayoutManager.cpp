#include "LayoutManager.h"
#include "GlobalData.h"
#include "UIParam.h"

void LayoutManager::reinitialize(HMONITOR monitor, REAL width_limit)
{
    m_monitor = monitor;
    m_width_limit = width_limit;
    m_widest = 0;
}

// --------------------GridLayoutManager---------------------

void GridLayoutManager::reinitialize(HMONITOR monitor, REAL width_limit)
{
    LayoutManager::reinitialize(monitor, width_limit);
    m_items.clear();
}

const LayoutItem *GridLayoutManager::itemAt(size_t index) const
{
    for (const auto &row : m_items) {
        if (index < row.size())
            return &row[index];
        index -= row.size();
    }
    return nullptr;
}

std::vector<const LayoutItem *> GridLayoutManager::intersectItems(const RectF &rect) const
{
    std::vector<const LayoutItem *> ret;
    if (rect.IsEmptyArea() || m_items.empty() || m_items[0].empty())
        return ret;

    // ignore rows above
    size_t row = 0;
    for (REAL y = 0; row < m_highest.size(); ++row) {
        y += m_highest[row] + globalData()->UI()->gridEdgeVMargin();
        if (y > rect.Y)
            break;
    }

    for (; row < m_items.size(); ++row) {
        for (const LayoutItem &item : m_items[row]) {
            const RectF &item_rect = item.rect();
            if (item_rect.Y > rect.GetBottom())
                break;
            if (item_rect.IntersectsWith(rect))
                ret.push_back(&item);
        }
    }

    return ret;
}

const LayoutItem *GridLayoutManager::itemFromPoint(const PointF &point) const
{
    // get item from 1x1 rect based on point
    const auto items = intersectItems(RectF(point, SizeF(1, 1)));
    return items.empty() ? nullptr : items.front();
}

const LayoutItem *GridLayoutManager::getNextItem(const LayoutItem *item) const
{
    if (!item)
        return nullptr;

    for (size_t row = 0; row < m_items.size(); ++row) {
        for (size_t col = 0; col < m_items[row].size(); ++col) {
            if (&m_items[row][col] == item) {
                if (col + 1 < m_items[row].size()) {
                    return &m_items[row][col + 1];
                } else if (row + 1 < m_items.size()) {
                    return &m_items[row + 1][0];
                } else {
                    return &m_items[0][0];
                }
            }
        }
    }

    // should never happen
    return nullptr;
}

const LayoutItem *GridLayoutManager::getPrevItem(const LayoutItem *item) const
{
    if (!item)
        return nullptr;

    for (size_t row = 0; row < m_items.size(); ++row) {
        for (size_t col = 0; col < m_items[row].size(); ++col) {
            if (&m_items[row][col] == item) {
                if (col > 0) {
                    return &m_items[row][col - 1];
                } else if (row > 0) {
                    return &m_items[row - 1].back();
                } else {
                    return &m_items.back().back();
                }
            }
        }
    }

    // should never happen
    return nullptr;
}

void GridLayoutManager::addItem(WindowHandle *window)
{
    const UIParam *ui = globalData()->UI();

    window->updateAttributes();
    const RectF &window_rect = window->rect();
    const SizeF &scaled_size = LayoutItem::scaledSize(
            window_rect.Width, window_rect.Height,
            ui->gridItemMaxWidth(), ui->gridItemMaxHeight(), ui->gridBarHeight());
    RectF rect(PointF(), scaled_size);
    rect.Width = max(rect.Width, ui->listItemMinWidth());

    if (m_items.empty()) {
        // first item
        m_items.push_back({});
        rect.X = ui->gridEdgeHMargin();
        rect.Y = ui->gridEdgeVMargin();
        m_widest = scaled_size.Width;
        m_highest.push_back(scaled_size.Height);
    } else {
        const RectF &last_rect = m_items.back().back().rect();
        if (last_rect.GetRight() + ui->itemHMargin() + scaled_size.Width
                <= m_width_limit - ui->gridEdgeHMargin()) {
            // append to last row
            rect.X = last_rect.GetRight() + ui->itemHMargin();
            rect.Y = last_rect.Y;
            m_highest.back() = max(m_highest.back(), scaled_size.Height);
        } else {
            // start a new row
            m_items.push_back({});
            rect.X = ui->gridEdgeHMargin();
            rect.Y = last_rect.Y + m_highest.back() + ui->itemVMargin();
            m_highest.push_back(scaled_size.Height);
        }
    }

    m_items.back().emplace_back(window, rect, ui->gridBarHeight());
    m_widest = max(m_widest, rect.GetRight() - m_items.back().front().rect().X);
}

void GridLayoutManager::alignItems()
{
    const UIParam *ui = globalData()->UI();

    if (m_items.empty() || m_items[0].empty()) {
        m_rect = { 0, 0, 0, 0 };
        return;
    }

    // align horizontally center of each row
    for (std::vector<LayoutItem> &row : m_items) {
        const RectF &back = row.back().rect(), front = row.front().rect();
        const REAL width = back.GetRight() - front.X;
        const REAL offset = (m_widest - width) / 2;
        for (LayoutItem &item : row) {
            PointF pos = { item.rect().X + offset, item.rect().Y };
            item.setPosition(pos);
        }
    }
    m_rect = {
        0, 0,
        m_widest + ui->gridEdgeHMargin() * 2,
        m_items.back().back().rect().Y + m_highest.back() + ui->gridEdgeVMargin()
    };
}

// --------------------ListLayoutManager---------------------

void ListLayoutManager::reinitialize(HMONITOR monitor, REAL width_limit)
{
    LayoutManager::reinitialize(monitor, width_limit);
    m_items.clear();
}

const LayoutItem *ListLayoutManager::itemAt(size_t index) const
{
    if (index >= m_items.size())
        return nullptr;
    return &m_items[index];
}

std::vector<const LayoutItem *> ListLayoutManager::intersectItems(const RectF &rect) const
{
    std::vector<const LayoutItem *> ret;
    if (rect.IsEmptyArea() || m_items.empty())
        return ret;

    for (const LayoutItem &item : m_items) {
        const RectF &item_rect = item.rect();
        if (item_rect.Y > rect.GetBottom())
            break;
        if (item_rect.IntersectsWith(rect))
            ret.push_back(&item);
    }
    return ret;
}

const LayoutItem *ListLayoutManager::itemFromPoint(const PointF &point) const
{
    // get item from 1x1 rect based on point
    const auto items = intersectItems(RectF(point, SizeF(1, 1)));
    return items.empty() ? nullptr : items.front();
}

void ListLayoutManager::addItem(WindowHandle *window)
{
    const UIParam *ui = globalData()->UI();

    window->updateAttributes();
    const RectF &window_rect = window->rect();
    const SizeF &scaled_size = LayoutItem::scaledSize(
            window_rect.Width, window_rect.Height,
            ui->listItemMaxWidth(), ui->listItemMaxHeight(), ui->listBarHeight());
    RectF rect(PointF(), scaled_size);
    rect.Width = max(rect.Width, ui->listItemMinWidth());

    if (m_items.empty()) {
        rect.X = ui->listEdgeHMargin();
        rect.Y = ui->listEdgeVMargin();
    } else {
        const RectF &last_rect = m_items.back().rect();
        rect.X = last_rect.X;
        rect.Y = last_rect.GetBottom() + ui->itemVMargin();
    }

    m_items.emplace_back(window, rect, ui->listBarHeight());
}

void ListLayoutManager::alignItems()
{
    const UIParam *ui = globalData()->UI();

    if (m_items.empty()) {
        m_rect = { 0, 0, 0, 0 };
        return;
    }

    m_widest = ui->listItemMaxWidth();
    // align horizontally center of each row
    for (LayoutItem &item : m_items) {
        RectF item_rect = item.rect();
        const LONG offset = (m_widest - item_rect.Width) / 2;
        item.setPosition({ item_rect.X + offset, item_rect.Y });
    }
    m_rect = {
        0, 0,
        m_widest + ui->listEdgeHMargin() * 2,
        m_items.back().rect().GetBottom() + ui->listEdgeVMargin()
    };
}

const LayoutItem *ListLayoutManager::getNextItem(const LayoutItem *item) const
{
    if (!item || m_items.size() == 1)
        return item;

    for (size_t i = 0; i < m_items.size(); ++i) {
        if (&m_items[i] == item)
            return &m_items[(i + 1) % m_items.size()];
    }

    // should never happen
    return nullptr;
}

const LayoutItem *ListLayoutManager::getPrevItem(const LayoutItem *item) const
{
    if (!item || m_items.size() == 1)
        return item;

    if (item == &m_items.front())
        return &m_items.back();

    for (size_t i = 1; i < m_items.size(); ++i) {
        if (&m_items[i] == item)
            return &m_items[(i - 1) % m_items.size()];
    }

    // should never happen
    return nullptr;
}
