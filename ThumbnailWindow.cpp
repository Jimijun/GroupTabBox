#include "ThumbnailWindow.h"
#include "GlobalData.h"

#include <gdiplus.h>
#include <windowsx.h>

ThumbnailWindowBase::~ThumbnailWindowBase()
{
    hide();
    m_bitmap.reset();
    m_dc.reset();
    if (m_hwnd)
        DestroyWindow(m_hwnd);
}

LRESULT ThumbnailWindowBase::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        m_hwnd = nullptr;
        return 0;

    case WM_ERASEBKGND:
        // do not erase background
        return 1;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            handlePaint(hdc);
            EndPaint(m_hwnd, &ps);
        }
        return 0;

    case WM_LBUTTONUP:
        handleLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEWHEEL:
        handleMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam),
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (!m_keep_showing)
            handleKeyUp(wParam);
        return 0;

    default:
        break;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

bool ThumbnailWindowBase::create(HINSTANCE instance)
{
    if (m_hwnd)
        return true;

    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"GroupTabBox", L"ThumbnailWindow",
        WS_POPUPWINDOW,
        0, 0, 1, 1,
        nullptr, nullptr, instance, nullptr
    );

    return m_hwnd != nullptr;
}

void ThumbnailWindowBase::show(bool keep)
{
    if (!m_hwnd || visible())
        return;

    m_keep_showing = keep;
    m_monitor = globalData()->currentMonitor();
    initializeLayout();

    updateBitmap(true);
    updateView({ 0, 0, m_rect.Width, m_rect.Height });

    // consider border
    SetWindowPos(m_hwnd, nullptr, m_rect.X - 1, m_rect.Y - 1,
            m_rect.Width + 2, m_rect.Height + 2, SWP_SHOWWINDOW);

    // show and focus
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    m_visible = true;
}

void ThumbnailWindowBase::hide()
{
    if (!m_hwnd || !visible())
        return;

    updateView({});
    ShowWindow(m_hwnd, SW_HIDE);
    m_bitmap.reset();
    m_visible = false;
}

void ThumbnailWindowBase::activateSelected()
{
    if (m_selected)
        globalData()->activateWindow(m_selected->windowHandle());
}

void ThumbnailWindowBase::selectNext()
{
    setSelected(m_layout_manager->getNextItem(m_selected));
}

void ThumbnailWindowBase::selectPrev()
{
    setSelected(m_layout_manager->getPrevItem(m_selected));
}

void ThumbnailWindowBase::requestRepaint()
{
    if (!visible())
        return;

    InvalidateRect(m_hwnd, nullptr, false);
    UpdateWindow(m_hwnd);
}

void ThumbnailWindowBase::initializeBitmap()
{
    auto release_dc = [this](HDC hdc) { ReleaseDC(m_hwnd, hdc); };
    std::unique_ptr<HDC__, decltype(release_dc)> hdc = { GetDC(m_hwnd), release_dc };

    if (!m_dc)
        m_dc = { CreateCompatibleDC(hdc.get()), DeleteDC };

    const RectF &layout_rect = m_layout_manager->rect();
    const SIZE bitmap_size = {
        max(static_cast<LONG>(layout_rect.Width), static_cast<LONG>(m_rect.Width)),
        max(static_cast<LONG>(layout_rect.Height), static_cast<LONG>(m_rect.Height))
    };

    // redraw all next time
    m_dirty_region.MakeInfinite();

    if (m_bitmap) {
        SIZE current_size;
        if (GetBitmapDimensionEx(m_bitmap.get(), &current_size)
                && current_size.cx == bitmap_size.cx && current_size.cy == bitmap_size.cy)
            return;
    }
    m_bitmap = {
        CreateCompatibleBitmap(hdc.get(), bitmap_size.cx, bitmap_size.cy),
        DeleteObject
    };
}

void ThumbnailWindowBase::updateView(const RectF &next_view_rect)
{
    if (!m_layout_manager || m_view_rect.Equals(next_view_rect))
        return;

    m_thumbnail_updated = false;

    std::vector<const LayoutItem *> current_items = m_layout_manager->intersectItems(m_view_rect);
    // hide all item if next is empty
    if (next_view_rect.IsEmptyArea()) {
        for (const auto &item : current_items)
            item->windowHandle()->hideThumbnail(m_hwnd);
        m_view_rect = { 0, 0, 0, 0 };
        return;
    }

    std::vector<const LayoutItem *> next_items = m_layout_manager->intersectItems(next_view_rect);
    // hide invisible items
    for (const auto &item : current_items) {
        if (std::find(next_items.begin(), next_items.end(), item) == next_items.end())
            item->windowHandle()->hideThumbnail(m_hwnd);
    }

    m_view_rect = next_view_rect;
}

void ThumbnailWindowBase::updateBitmap(bool redraw_all)
{
    if (!m_dc || !m_bitmap) {
        initializeBitmap();
        redraw_all = true;
    }

    if (redraw_all)
        m_dirty_region.MakeInfinite();

    SelectObject(m_dc.get(), m_bitmap.get());
    Graphics graphics(m_dc.get());
    if (m_dirty_region.IsEmpty(&graphics))
        return;

    beforeDrawContent(&graphics);
    drawContent(&graphics);
    afterDrawContent(&graphics);
}

void ThumbnailWindowBase::setSelected(const LayoutItem *item)
{
    if (!item || m_selected == item)
        return;

    const UIParam &ui = globalData()->UI();

    // calculate dirty region
    const int margin = ui.selectFrameMargin() + (ui.selectFrameWidth() / 2) + 1;
    if (m_selected) {
        RectF rect = m_selected->rect();
        rect.Inflate(margin, margin);
        m_dirty_region.Xor(rect);
    }
    RectF rect = item->rect();
    rect.Inflate(margin, margin);
    m_dirty_region.Xor(rect);

    m_selected = item;

    // scroll view if selected item is out of view
    const RectF &selected_rect = m_selected->rect();
    if (!m_view_rect.Contains(selected_rect)) {
        RectF next_view_rect = m_view_rect;
        if (selected_rect.Y < next_view_rect.Y) {
            next_view_rect.Y = max(selected_rect.Y - ui.itemVMargin(), 0);
        } else if (selected_rect.GetBottom() > next_view_rect.GetBottom()) {
            next_view_rect.Y = min(
                    selected_rect.GetBottom() + ui.itemVMargin() - m_view_rect.Height,
                    m_layout_manager->rect().Height - m_view_rect.Height);
        }
        updateView(next_view_rect);
    }

    updateBitmap();
    requestRepaint();
}

void ThumbnailWindowBase::beforeDrawContent(Graphics *graphics)
{
    // clear dirty region
    const Gdiplus::Color back_color(globalData()->UI().backgroundColor());
    Gdiplus::SolidBrush back_brush(back_color);
    graphics->FillRegion(&back_brush, &m_dirty_region);
}

void ThumbnailWindowBase::drawContent(Graphics *graphics)
{
    const UIParam &ui = globalData()->UI();

    // draw item info
    RectF bound_rect;
    m_dirty_region.GetBounds(&bound_rect, graphics);
    std::vector<const LayoutItem *> items = m_layout_manager->intersectItems(bound_rect);
    for (const auto &item : items) {
        if (m_dirty_region.IsVisible(item->rect()))
            item->drawInfo(graphics);
    }

    // draw select frame
    if (m_selected && m_dirty_region.IsVisible(m_selected->rect())) {
        RectF select_rect = m_selected->rect();
        select_rect.Inflate(ui.selectFrameMargin(), ui.selectFrameMargin());
        Gdiplus::Pen select_pen(Gdiplus::Color(ui.selectFrameColor()), ui.selectFrameWidth());
        graphics->DrawRectangle(&select_pen, select_rect);
    }
}

void ThumbnailWindowBase::afterDrawContent(Graphics *graphics)
{
    m_dirty_region.MakeEmpty();
}

void ThumbnailWindowBase::handlePaint(HDC hdc)
{
    if (!m_thumbnail_updated) {
        const std::vector<const LayoutItem *> items = m_layout_manager->intersectItems(m_view_rect);
        for (const auto &item : items) {
            RectF rect = item->thumbnailRect();
            rect.Offset(-m_view_rect.X + 1, -m_view_rect.Y + 1);
            item->windowHandle()->showThumbnail(m_hwnd, rect);
        }
        m_thumbnail_updated = true;
    }

    BitBlt(hdc, 0, 0, m_rect.Width, m_rect.Height,
            m_dc.get(), m_view_rect.X, m_view_rect.Y, SRCCOPY);
}

void ThumbnailWindowBase::handleLButtonUp(int x, int y)
{
    y += m_view_rect.Y;
    const LayoutItem *item = m_layout_manager->itemFromPoint(PointF(x, y));
    if (item && m_selected != item)
        setSelected(item);
}

void ThumbnailWindowBase::handleMouseWheel(short delta, int x, int y)
{
    const RectF &layout_rect = m_layout_manager->rect();
    if (m_view_rect.Height >= layout_rect.Height)
        return;

    RectF next_view_rect = m_view_rect;
    next_view_rect.Offset(0, -delta * globalData()->UI().wheelDeltaPixel());
    if (next_view_rect.Y < 0)
        next_view_rect.Y = 0;
    if (next_view_rect.GetBottom() > layout_rect.GetBottom())
        next_view_rect.Y = layout_rect.GetBottom() - m_view_rect.Height;

    updateView(next_view_rect);
    requestRepaint();
}

void ThumbnailWindowBase::handleKeyUp(WPARAM key)
{
    if (key == VK_MENU && m_selected)
        activateSelected();
}

// --------------------GroupThumbnailWindow---------------------

LRESULT GroupThumbnailWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_RBUTTONUP:
        handleRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    default:
        break;
    }
    return ThumbnailWindowBase::handleMessage(uMsg, wParam, lParam);
}

void GroupThumbnailWindow::activateSelected()
{
    if (!m_selected)
        return;

    // if more than one window in the group, do not activate directly
    if (multipleWindowsInGroup(m_selected->windowHandle()->exePath())) {
        ListThumbnailWindow *list = globalData()->listWindow();
        if (list)
            list->activateSelected();
        return;
    }

    globalData()->activateWindow(m_selected->windowHandle());
}

void GroupThumbnailWindow::initializeLayout()
{
    updateView({});

    const RectF &limit_rect = globalData()->groupWindowLimitRect();

    // initialize layout and align items
    if (!m_layout_manager) {
        m_layout_manager = std::make_unique<GridLayoutManager>(m_monitor, limit_rect.Width);
    } else {
        m_layout_manager->reinitialize(m_monitor, limit_rect.Width);
    }
    // show first window of each group
    for (const auto &group : globalData()->windowGroups()) {
        for (WindowHandle *window : group) {
            if (window->monitor() == m_monitor) {
                m_layout_manager->addItem(window);
                break;
            }
        }
    }
    m_selected = m_layout_manager->itemAt(0);
    updateListWindow();
    m_layout_manager->alignItems();

    // calculate window rect
    const RectF &layout_rect = m_layout_manager->rect();
    if (layout_rect.IsEmptyArea())
        return;
    m_rect.Width = layout_rect.Width;
    m_rect.Height = min(layout_rect.Height, limit_rect.Height);
    m_rect.X = limit_rect.X + (limit_rect.Width - layout_rect.Width) / 2;
    m_rect.Y = limit_rect.Y + (limit_rect.Height - m_rect.Height) / 2;

    updateView({ 0, 0, m_rect.Width, m_rect.Height });
}

void GroupThumbnailWindow::setSelected(const LayoutItem *item)
{
    const LayoutItem *prev_selected = m_selected;
    ThumbnailWindowBase::setSelected(item);
    if (prev_selected != m_selected)
        updateListWindow();
}

void GroupThumbnailWindow::beforeDrawContent(Graphics *graphics)
{
    ThumbnailWindowBase::beforeDrawContent(graphics);

    const UIParam &ui = globalData()->UI();

    RectF bound_rect;
    m_dirty_region.GetBounds(&bound_rect, graphics);
    std::vector<const LayoutItem *> items = m_layout_manager->intersectItems(bound_rect);
    Gdiplus::SolidBrush brush{Gdiplus::Color(ui.gridItemShadowColor())};

    const float scale = globalData()->monitorScale();
    for (const auto &item : items) {
        if (!m_dirty_region.IsVisible(item->rect()))
            continue;

        // if more than one window in the group, draw a fake shadow
        if (multipleWindowsInGroup(item->windowHandle()->exePath())) {
            RectF rect = item->rect();
            rect.Offset(10 * scale, 10 * scale);
            brush.SetColor(Gdiplus::Color(ui.gridItemShadowColor()));
            graphics->FillRectangle(&brush, rect);
            rect.Width -= 7 * scale;
            rect.Height -= 7 * scale;
            brush.SetColor(Gdiplus::Color(ui.backgroundColor()));
            graphics->FillRectangle(&brush, rect);
        }
    }
}

void GroupThumbnailWindow::handleMouseWheel(short delta, int x, int y)
{
    if (m_selected) {
        PointF native_point(x - m_rect.X, y - m_rect.Y);
        RectF select_rect = m_selected->rect();
        select_rect.Offset(-m_view_rect.X, -m_view_rect.Y);
        ListThumbnailWindow *list = globalData()->listWindow();
        if (select_rect.Contains(native_point) && list) {
            if (delta > 0) {
                list->selectPrev();
            } else {
                list->selectNext();
            }
            return;
        }
    }

    ThumbnailWindowBase::handleMouseWheel(delta, x, y);
}

void GroupThumbnailWindow::handleLButtonUp(int x, int y)
{
    ThumbnailWindowBase::handleLButtonUp(x, y);
    if (m_selected && !multipleWindowsInGroup(m_selected->windowHandle()->exePath()))
        activateSelected();
}

void GroupThumbnailWindow::handleRButtonUp(int x, int y)
{
    const LayoutItem *item = m_layout_manager->itemFromPoint(PointF(x, y));
    if (!item)
        return;

    setSelected(item);
    ListThumbnailWindow *list = globalData()->listWindow();
    if (list)
        list->activateSelected();
}

void GroupThumbnailWindow::updateListWindow()
{
    ListThumbnailWindow *list = globalData()->listWindow();
    if (!list || !m_selected)
        return;

    list->setGroup(m_selected->windowHandle()->exePath());
    if (!list->visible())
        list->show(m_keep_showing);
}

bool GroupThumbnailWindow::multipleWindowsInGroup(const std::wstring &group) const
{
    const std::vector<WindowHandle *> &windows = globalData()->windowsFromGroup(group);
    int count = 0;
    for (const auto &window : windows) {
        if (window->monitor() == m_monitor && ++count > 1)
            return true;
    }
    return false;
}

// --------------------ListThumbnailWindow---------------------

void ListThumbnailWindow::setGroup(const std::wstring &group_name)
{
    if (m_group_name == group_name)
        return;

    m_group_name = group_name;
    initializeLayout();
    m_bitmap.reset();
    if (visible()) {
        updateBitmap(true);
        requestRepaint();
    }
}

void ListThumbnailWindow::initializeLayout()
{
    updateView({});

    m_rect = globalData()->listWindowLimitRect();

    if (!m_layout_manager) {
        m_layout_manager = std::make_unique<ListLayoutManager>(m_monitor, m_rect.Width);
    } else {
        m_layout_manager->reinitialize(m_monitor, m_rect.Width);
    }
    for (const auto &window : globalData()->windowsFromGroup(m_group_name)) {
        if (window->monitor() == m_monitor)
            m_layout_manager->addItem(window);
    }
    m_selected = m_layout_manager->itemAt(0);
    m_layout_manager->alignItems();

    updateView({ 0, 0, m_rect.Width, m_rect.Height });
}

void ListThumbnailWindow::handleLButtonUp(int x, int y)
{
    ThumbnailWindowBase::handleLButtonUp(x, y);
    y += m_view_rect.Y;
    if (m_selected && m_selected->rect().Contains(x, y))
        activateSelected();
}
