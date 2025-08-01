#pragma once

#include "LayoutItem.h"
#include "UIParam.h"
#include "WindowHandle.h"

#include <vector>

using Gdiplus::REAL;
using Gdiplus::RectF;
using Gdiplus::PointF;

class LayoutManager
{
public:
    LayoutManager(HMONITOR monitor, REAL width_limit)
        : m_monitor(monitor), m_width_limit(width_limit) {}

    const RectF &rect() const { return m_rect; }

    virtual void reinitialize(HMONITOR monitor, REAL width_limit);

    virtual const LayoutItem *itemAt(size_t index) const = 0;
    virtual std::vector<const LayoutItem *> intersectItems(const RectF &rect) const = 0;
    virtual const LayoutItem *itemFromPoint(const PointF &point) const = 0;
    virtual const LayoutItem *getNextItem(const LayoutItem *item) const = 0;
    virtual const LayoutItem *getPrevItem(const LayoutItem *item) const = 0;

    virtual void addItem(WindowHandle *window) = 0;
    virtual void alignItems() = 0;

protected:
    HMONITOR m_monitor;
    REAL m_width_limit;
    RectF m_rect;

    REAL m_widest = 0;  // widest row
};

class GridLayoutManager : public LayoutManager
{
public:
    GridLayoutManager(HMONITOR monitor, REAL width_limit)
        : LayoutManager(monitor, width_limit) {}

    void reinitialize(HMONITOR monitor, REAL width_limit) override;

    const LayoutItem *itemAt(size_t index) const override;
    std::vector<const LayoutItem *> intersectItems(const RectF &rect) const override;
    const LayoutItem *itemFromPoint(const PointF &point) const override;
    const LayoutItem *getNextItem(const LayoutItem *item) const override;
    const LayoutItem *getPrevItem(const LayoutItem *item) const override;

    void addItem(WindowHandle *window) override;
    void alignItems() override;

private:
    std::vector<std::vector<LayoutItem>> m_items;
    std::vector<REAL> m_highest;  // highest item of each row
};

class ListLayoutManager : public LayoutManager
{
public:
    ListLayoutManager(HMONITOR monitor, REAL width_limit)
        : LayoutManager(monitor, width_limit) {}

    void reinitialize(HMONITOR monitor, REAL width_limit) override;

    const LayoutItem *itemAt(size_t index) const override;
    std::vector<const LayoutItem *> intersectItems(const RectF &rect) const override;
    const LayoutItem *itemFromPoint(const PointF &point) const override;
    const LayoutItem *getNextItem(const LayoutItem *item) const override;
    const LayoutItem *getPrevItem(const LayoutItem *item) const override;

    void addItem(WindowHandle *window);
    void alignItems();

private:
    std::vector<LayoutItem> m_items;
};
