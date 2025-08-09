#include "GlobalData.h"
#include "Configure.h"
#include "MainWindow.h"
#include "ThumbnailWindow.h"
#include "UIParam.h"

#include <ShellScalingApi.h>

#pragma comment(lib, "shcore.lib")

BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (WindowHandle::validWindow(hwnd))
        globalData()->m_windows.emplace_back(hwnd);
    return TRUE;
}

BOOL CALLBACK enumMonitorsProc(HMONITOR monitor, HDC hdc, LPRECT lprc, LPARAM lParam)
{
    globalData()->m_monitors.push_back(monitor);
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

const std::vector<WindowHandle *> &GlobalData::windowsFromGroup(const std::wstring &group_name) const
{
    auto it = m_group_index.find(group_name);
    if (it == m_group_index.end())
        return m_window_groups.back();  // return the empty group
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
    work_rect.Width -= m_ui->listWindowWidthLimit();
    rect.Width = max(work_rect.Width * m_ui->groupWindowWidthLimitRadio(),
            m_ui->gridItemMaxWidth() + m_ui->itemHMargin() * 2);
    rect.Height = max(work_rect.Height * m_ui->groupWindowHeightLimitRadio(),
            m_ui->gridItemMaxHeight() + m_ui->itemVMargin() * 2);
    rect.X = work_rect.X + (work_rect.Width - rect.Width) / 2;
    rect.Y = work_rect.Y + (work_rect.Height - rect.Height) / 2;
    return rect;
}

RectF GlobalData::listWindowLimitRect() const
{
    if (!m_current_monitor)
        return RectF();

    RECT rc_work = monitorInfo().rcWork;
    return RectF(rc_work.right - m_ui->listWindowWidthLimit(), rc_work.top,
            m_ui->listWindowWidthLimit(), rc_work.bottom - rc_work.top);
}

void GlobalData::setCurrentMonitor(HMONITOR monitor)
{
    if (!monitor)
        return;

    m_current_monitor = monitor;
    m_monitor_info.cbSize = sizeof(m_monitor_info);
    GetMonitorInfo(m_current_monitor, &m_monitor_info);

    UINT dpi;
    GetDpiForMonitor(m_current_monitor, MDT_EFFECTIVE_DPI, &dpi, &dpi);
    m_monitor_scale = dpi / 96.0f;
    m_ui->update(m_monitor_scale);
}

bool GlobalData::initialize(HINSTANCE instance)
{
    m_hinstance = instance;

    if (!m_config) {
        m_config = std::make_unique<Configure>();
        if (!m_config)
            return false;
    }
    m_config->load();

    if (!m_ui) {
        m_ui = std::make_unique<UIParam>();
        if (!m_ui)
            return false;
    }

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

void GlobalData::destroy()
{
    m_main_window.reset();
    m_group_window.reset();
    m_list_window.reset();
}

bool GlobalData::update(HMONITOR monitor)
{
    if (!monitor)
        return false;
    setCurrentMonitor(monitor);

    m_group_window->hide();
    m_list_window->hide();

    m_monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, enumMonitorsProc, 0);
    if (m_monitors.empty())
        return false;

    m_active_window = GetForegroundWindow();

    m_windows.clear();
    EnumWindows(enumWindowsProc, 0);
    if (m_windows.empty())
        return false;
    WindowHandle::updateUWPIconCache();

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
    // reserve an empty group
    m_window_groups.push_back({});

    return true;
}

LRESULT GlobalData::handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_KILLFOCUS) {
        HWND focus = reinterpret_cast<HWND>(wParam);
        if (focus != m_group_window->hwnd() && focus != m_group_window->foreHwnd()
                && focus != m_list_window->hwnd() && focus != m_list_window->foreHwnd()) {
            m_group_window->hide();
            m_list_window->hide();
            return 0;
        }
    }

    LRESULT res = -1;

#define HANDLE_MSG(window) \
    if (window) res = window->handleMessage(hwnd, uMsg, wParam, lParam); \
    if (res != -1) return res \

    HANDLE_MSG(m_main_window);
    HANDLE_MSG(m_group_window);
    HANDLE_MSG(m_list_window);
#undef HANDLE_MSG

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GlobalData::activateWindow(const WindowHandle *window)
{
    if (m_group_window)
        m_group_window->hide();
    if (m_list_window)
        m_list_window->hide();

    if (window)
        window->activate();
}
