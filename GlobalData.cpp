#include "GlobalData.h"

BOOL enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (WindowHandle::validWindow(hwnd))
        globalData()->m_windows.emplace_back(hwnd);
    return TRUE;
}

GlobalData *globalData()
{
    return GlobalData::instance();
}

GlobalData *GlobalData::instance()
{
    static GlobalData instance;
    return &instance;
}

GlobalData::~GlobalData()
{
    // set true to ignore messages
    m_internal_destroying = true;
}

const std::vector<WindowHandle *> &GlobalData::windowsFromGroup(const std::wstring &group_name) const
{
    auto it = m_group_index.find(group_name);
    if (it == m_group_index.end())
        return {};
    return m_window_groups[it->second];
}

RectF GlobalData::groupWindowLimitRect() const
{
    RectF rect;
    if (!m_current_monitor)
        return rect;

    RECT rc_work = monitorInfo().rcWork;
    RectF work_rect(rc_work.left, rc_work.top,
            rc_work.right - rc_work.left, rc_work.bottom - rc_work.top);
    work_rect.Width -= kListWindowWidthLimit;
    rect.Width = max(work_rect.Width * kGroupWindowWidthLimitRadio, kGridItemMaxWidth + kItemHMargin * 2);
    rect.Height = max(work_rect.Height * kGroupWindowHeightLimitRadio, kGridItemMaxHeight + kItemVMargin * 2);
    rect.X = work_rect.X + (work_rect.Width - rect.Width) / 2;
    rect.Y = work_rect.Y + (work_rect.Height - rect.Height) / 2;
    return rect;
}

RectF GlobalData::listWindowLimitRect() const
{
    if (!m_current_monitor)
        return RectF();

    RECT rc_work = monitorInfo().rcWork;
    return RectF(rc_work.right - kListWindowWidthLimit, rc_work.top,
            kListWindowWidthLimit, rc_work.bottom - rc_work.top);
}

bool GlobalData::initialize(HINSTANCE instance)
{
    m_hinstance = instance;

    if (!m_main_window) {
        m_main_window = std::make_unique<MainWindow>();
        if (!m_main_window || !m_main_window->create(instance))
            return false;
    }
    if (!m_group_window) {
        m_group_window = std::make_unique<GroupThumbnailWindow>();
        if (!m_group_window || !m_group_window->create(instance))
            return false;
    }
    if (!m_list_window) {
        m_list_window = std::make_unique<ListThumbnailWindow>();
        if (!m_list_window || !m_list_window->create(instance))
            return false;
    }

    return true;
}

void GlobalData::update(MonitorBasis basis)
{
    m_active_window = GetForegroundWindow();
    switch (basis) {
    case MonitorBasis::MonitorBasisWindow:
        if (m_active_window)
            m_current_monitor = MonitorFromWindow(m_active_window, MONITOR_DEFAULTTONEAREST);
        // else fallthrough

    case MonitorBasis::MonitorBasisCursor:
    default:
        {
            POINT pos;
            GetCursorPos(&pos);
            m_current_monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTONEAREST);
        }
        break;
    }

    m_group_window->hide();
    m_list_window->hide();

    m_monitor_info.cbSize = sizeof(m_monitor_info);
    GetMonitorInfo(m_current_monitor, &m_monitor_info);

    m_windows.clear();
    EnumWindows(enumWindowsProc, 0);

    m_group_index.clear();
    m_window_groups.clear();
    for (auto &window : m_windows) {
        // group by exe path
        const std::wstring &path = window.exePath();
        auto it = m_group_index.find(path);
        if (it != m_group_index.end()) {
            m_window_groups[it->second].push_back(&window);
        } else {
            m_group_index.insert({ path, m_group_index.size() });
            m_window_groups.push_back({ &window });
        }
    }
}

LRESULT GlobalData::handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_main_window && hwnd == m_main_window->hwnd()) {
        return m_main_window->handleMessage(uMsg, wParam, lParam);
    } else if (m_group_window && hwnd == m_group_window->hwnd()) {
        return m_group_window->handleMessage(uMsg, wParam, lParam);
    } else if (m_list_window && hwnd == m_list_window->hwnd()) {
        return m_list_window->handleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GlobalData::activateWindow(const WindowHandle *window)
{
    if (!window)
        return;

    window->activate();

    if (m_group_window)
        m_group_window->hide();
    if (m_list_window)
        m_list_window->hide();
}
