#pragma once

#include "LayoutManager.h"

#include <gdiplus.h>
#include <Windows.h>

#include <memory>
#include <string>

using Gdiplus::Graphics;
using Gdiplus::RectF;
using Gdiplus::Region;

class ThumbnailWindowBase
{
public:
    ThumbnailWindowBase() = default;
    virtual ~ThumbnailWindowBase();

    HWND hwnd() const { return m_hwnd; }
    bool visible() const { return m_visible; }

    void selectNext();
    void selectPrev();
    void keepShowing(bool keep) { m_keep_showing = keep; }

    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual bool create(HINSTANCE instance);
    virtual void show(bool keep);
    virtual void hide();
    virtual void activateSelected();

protected:
    void requestRepaint();
    void initializeBitmap();
    void updateBitmap(bool redraw_all = false);

    virtual void initializeLayout() = 0;
    virtual void setSelected(const LayoutItem *item);
    virtual void updateView(const RectF &next_view_rect);
    virtual void beforeDrawContent(Graphics *graphics);
    virtual void drawContent(Graphics *graphics);
    virtual void afterDrawContent(Graphics *graphics);

    // handle events
    virtual void handlePaint(HDC hdc);
    virtual void handleLButtonUp(int x, int y);
    virtual void handleMouseWheel(short delta, int x, int y);
    virtual void handleKeyUp(WPARAM key);

    HWND m_hwnd = nullptr;
    bool m_visible = false;
    bool m_keep_showing = false;
    HMONITOR m_monitor = nullptr;
    RectF m_rect;
    RectF m_view_rect;

    std::unique_ptr<LayoutManager> m_layout_manager = nullptr;
    const LayoutItem *m_selected = nullptr;

    std::unique_ptr<HDC__, decltype(&DeleteDC)> m_dc = { nullptr, DeleteDC };
    std::unique_ptr<HBITMAP__, decltype(&DeleteObject)> m_bitmap = { nullptr, DeleteObject };
    Region m_dirty_region;
    bool m_thumbnail_updated = false;
};

class GroupThumbnailWindow : public ThumbnailWindowBase
{
public:
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    void activateSelected() override;

private:
    void initializeLayout() override;
    void setSelected(const LayoutItem *item) override;
    void beforeDrawContent(Graphics *graphics) override;
    void handleMouseWheel(short delta, int x, int y) override;
    void handleLButtonUp(int x, int y) override;

    void handleRButtonUp(int x, int y);

    void updateListWindow();
    bool multipleWindowsInGroup(const std::wstring &group) const;
};

class ListThumbnailWindow : public ThumbnailWindowBase
{
public:
    void setGroup(const std::wstring &group_name);

private:
    void initializeLayout() override;
    void handleLButtonUp(int x, int y) override;

    std::wstring m_group_name;
};
