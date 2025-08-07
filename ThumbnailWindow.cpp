#include "ThumbnailWindow.h"
#include "GlobalData.h"

#include <gdiplus.h>
#include <windowsx.h>

static void enableBlur(HWND hwnd)
{
    HMODULE handle = GetModuleHandle(L"user32.dll");
    if(handle) {
        enum WINDOWCOMPOSITIONATTRIB { WCA_ACCENT_POLICY = 19 };
        struct WINDOWCOMPOSITIONATTRIBDATA
        {
            WINDOWCOMPOSITIONATTRIB Attrib;
            PVOID pvData;
            SIZE_T cbData;
        };
        enum ACCENT_STATE { ACCENT_ENABLE_BLURBEHIND = 3 };
        struct ACCENT_POLICY
        {
            ACCENT_STATE AccentState;
            DWORD AccentFlags, GradientColor, AnimationId;
        };

        BOOL (*func)(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA *data) =
                reinterpret_cast<decltype(func)>(GetProcAddress(handle, "SetWindowCompositionAttribute"));
        if(func){
            ACCENT_POLICY accent = { ACCENT_ENABLE_BLURBEHIND, 0, 0, 0 };
            WINDOWCOMPOSITIONATTRIBDATA data = { WCA_ACCENT_POLICY, &accent, sizeof(accent) };
            func(hwnd, &data);
        }
    }
}

ThumbnailWindowBase::~ThumbnailWindowBase()
{
    hide();
}

LRESULT ThumbnailWindowBase::handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (hwnd != m_hwnd.get() && hwnd != m_fore_hwnd.get())
        return -1;

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
            HDC hdc = BeginPaint(hwnd, &ps);
            handlePaint(hwnd, hdc);
            EndPaint(hwnd, &ps);
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
    return -1;
}

bool ThumbnailWindowBase::create(HINSTANCE instance)
{
    if (m_hwnd && m_fore_hwnd)
        return true;

    m_hwnd =  {
        CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            L"GroupTabBox", L"ThumbnailWindow",
            WS_POPUPWINDOW,
            0, 0, 1, 1,
            nullptr, nullptr, instance, nullptr
        ),
        DestroyWindow
    };
    m_fore_hwnd = {
        CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
            L"GroupTabBox", L"ForegroundWindow",
            WS_POPUPWINDOW,
            0, 0, 1, 1,
            m_hwnd.get(), nullptr, instance, nullptr
        ),
        DestroyWindow
    };

    if (m_hwnd && m_fore_hwnd) {
        const Configure *config = globalData()->config();

        // foreground window's background should be fully transparent
        SetLayeredWindowAttributes(m_fore_hwnd.get(), RGB(0, 255, 0), 0, LWA_COLORKEY);

        if (config->enableBackgroundBlur()) {
            // remove WS_EX_LAYERED before blur
            SetWindowLong(m_hwnd.get(), GWL_EXSTYLE,
                    GetWindowLong(m_hwnd.get(), GWL_EXSTYLE) & ~WS_EX_LAYERED);
            enableBlur(m_hwnd.get());
        } else {
            // set background alpha
            float alpha = config->backgroundAlpha();
            alpha = min(alpha, 1.0f);
            alpha = max(alpha, 0.0f);
            SetLayeredWindowAttributes(m_hwnd.get(), 0, alpha * 255, LWA_ALPHA);
        }
    }

    return m_hwnd && m_fore_hwnd;
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
    SetWindowPos(m_hwnd.get(), nullptr, m_rect.X - 1, m_rect.Y - 1,
            m_rect.Width + 2, m_rect.Height + 2, SWP_SHOWWINDOW);
    SetWindowPos(m_fore_hwnd.get(), nullptr, m_rect.X - 1, m_rect.Y - 1,
            m_rect.Width + 2, m_rect.Height + 2, SWP_SHOWWINDOW);

    // show and focus
    ShowWindow(m_hwnd.get(), SW_SHOW);
    ShowWindow(m_fore_hwnd.get(), SW_SHOW);
    SetForegroundWindow(m_hwnd.get());
    m_visible = true;
}

void ThumbnailWindowBase::hide()
{
    if (!m_hwnd || !visible())
        return;

    updateView({});
    ShowWindow(m_hwnd.get(), SW_HIDE);
    ShowWindow(m_fore_hwnd.get(), SW_HIDE);
    m_bitmap.reset();
    m_visible = false;
}

void ThumbnailWindowBase::activateSelected()
{
    if (m_selected) {
        globalData()->activateWindow(m_selected->windowHandle());
    } else {
        globalData()->activateWindow(nullptr);
    }
}

void ThumbnailWindowBase::selectNext()
{
    setSelected(m_layout_manager->getNextItem(m_selected));
}

void ThumbnailWindowBase::selectPrev()
{
    setSelected(m_layout_manager->getPrevItem(m_selected));
}

void ThumbnailWindowBase::requestRepaint(bool repaint_background)
{
    if (!visible())
        return;

    if (repaint_background) {
        InvalidateRect(m_hwnd.get(), nullptr, false);
        UpdateWindow(m_hwnd.get());
    }
    InvalidateRect(m_fore_hwnd.get(), nullptr, false);
    UpdateWindow(m_fore_hwnd.get());
}

void ThumbnailWindowBase::initializeBitmap()
{
    auto release_dc = [this](HDC hdc) { ReleaseDC(m_fore_hwnd.get(), hdc); };
    std::unique_ptr<HDC__, decltype(release_dc)> hdc = { GetDC(m_fore_hwnd.get()), release_dc };

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
            item->windowHandle()->hideThumbnail(m_fore_hwnd.get());
        m_view_rect = { 0, 0, 0, 0 };
        return;
    }

    std::vector<const LayoutItem *> next_items = m_layout_manager->intersectItems(next_view_rect);
    // hide invisible items
    for (const auto &item : current_items) {
        if (std::find(next_items.begin(), next_items.end(), item) == next_items.end())
            item->windowHandle()->hideThumbnail(m_fore_hwnd.get());
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

    const UIParam *ui = globalData()->UI();

    // calculate dirty region
    const int margin = ui->selectFrameMargin() + (ui->selectFrameWidth() / 2) + 1;
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
            next_view_rect.Y = max(selected_rect.Y - ui->itemVMargin(), 0);
        } else if (selected_rect.GetBottom() > next_view_rect.GetBottom()) {
            next_view_rect.Y = min(
                    selected_rect.GetBottom() + ui->itemVMargin() - m_view_rect.Height,
                    m_layout_manager->rect().Height - m_view_rect.Height);
        }
        updateView(next_view_rect);
    }

    updateBitmap();
    requestRepaint();
}

void ThumbnailWindowBase::beforeDrawContent(Graphics *graphics)
{
    // fill dirty region with green
    const Gdiplus::Color bitmap_back_color(0xFF00FF00);
    const Gdiplus::Color back_color(bitmap_back_color);
    Gdiplus::SolidBrush back_brush(back_color);
    graphics->FillRegion(&back_brush, &m_dirty_region);
}

void ThumbnailWindowBase::drawContent(Graphics *graphics)
{
    const UIParam *ui = globalData()->UI();

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
        select_rect.Inflate(ui->selectFrameMargin(), ui->selectFrameMargin());
        Gdiplus::Pen select_pen(Gdiplus::Color(ui->selectFrameColor()), ui->selectFrameWidth());
        graphics->DrawRectangle(&select_pen, select_rect);
    }
}

void ThumbnailWindowBase::afterDrawContent(Graphics *graphics)
{
    m_dirty_region.MakeEmpty();
}

void ThumbnailWindowBase::handlePaint(HWND hwnd, HDC hdc)
{
    if (hwnd == m_hwnd.get()) {
        // draw background window
        Graphics graphics(hdc);
        graphics.Clear(globalData()->UI()->backgroundColor());
    } else if (hwnd == m_fore_hwnd.get()) {
        // draw thumbnail
        if (!m_thumbnail_updated && m_layout_manager) {
            const std::vector<const LayoutItem *> items = m_layout_manager->intersectItems(m_view_rect);
            for (const auto &item : items) {
                RectF rect = item->thumbnailRect();
                rect.Offset(-m_view_rect.X + 1, -m_view_rect.Y + 1);
                item->windowHandle()->showThumbnail(m_fore_hwnd.get(), rect);
            }
            m_thumbnail_updated = true;
        }
        BitBlt(hdc, 0, 0, m_rect.Width, m_rect.Height,
                m_dc.get(), m_view_rect.X, m_view_rect.Y, SRCCOPY);
    }
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
    next_view_rect.Offset(0, -delta * (30.f / 120.f));
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

LRESULT GroupThumbnailWindow::handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (hwnd == m_hwnd.get()) {
        switch (uMsg) {
        case WM_RBUTTONUP:
            handleRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        default:
            break;
        }
    }
    return ThumbnailWindowBase::handleMessage(hwnd, uMsg, wParam, lParam);
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

    const UIParam *ui = globalData()->UI();

    RectF bound_rect;
    m_dirty_region.GetBounds(&bound_rect, graphics);
    std::vector<const LayoutItem *> items = m_layout_manager->intersectItems(bound_rect);
    Gdiplus::SolidBrush brush{Gdiplus::Color(ui->gridItemShadowColor())};

    const float scale = globalData()->monitorScale();
    for (const auto &item : items) {
        if (!m_dirty_region.IsVisible(item->rect()))
            continue;

        // if more than one window in the group, draw a fake shadow
        if (multipleWindowsInGroup(item->windowHandle()->exePath())) {
            RectF rect = item->rect();
            rect.Offset(10 * scale, 10 * scale);
            brush.SetColor(Gdiplus::Color(ui->gridItemShadowColor()));
            graphics->FillRectangle(&brush, rect);
            rect.Width -= 7 * scale;
            rect.Height -= 7 * scale;
            brush.SetColor(Gdiplus::Color(0xFF00FF00));
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
    y += m_view_rect.Y;
    if (m_selected && m_selected->rect().Contains(x, y)
            && !multipleWindowsInGroup(m_selected->windowHandle()->exePath()))
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
