#pragma once

#include "MainWindow.h"
#include "ThumbnailWindow.h"
#include "WindowHandle.h"

#include <string>
#include <unordered_map>
#include <vector>

using Gdiplus::REAL;

class GlobalData
{
    friend BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);

public:
    enum class MonitorBasis
    {
        MonitorBasisWindow,
        MonitorBasisCursor
    };

    static GlobalData *instance();

    HINSTANCE hInstance() const { return m_hinstance; }
    bool internalDestroying() const { return m_internal_destroying; }
    HWND activeWindow() const { return m_active_window; }
    const HMONITOR &currentMonitor() const { return m_current_monitor; }
    const MONITORINFOEX &monitorInfo() const { return m_monitor_info; }
    const std::vector<WindowHandle> &windows() const { return m_windows; }
    const std::vector<std::vector<WindowHandle *>> &windowGroups() const { return m_window_groups; }
    const std::vector<WindowHandle *> &windowsFromGroup(const std::wstring &group_name) const;
    RectF groupWindowLimitRect() const;
    RectF listWindowLimitRect() const;
    MainWindow *mainWindow() const { return m_main_window.get(); }
    GroupThumbnailWindow *groupWindow() const { return m_group_window.get(); }
    ListThumbnailWindow *listWindow() const { return m_list_window.get(); }

    bool initialize(HINSTANCE instance);
    void update(MonitorBasis basis);
    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void activateWindow(const WindowHandle *window);

private:
    GlobalData() = default;
    ~GlobalData();

    HINSTANCE m_hinstance = nullptr;
    bool m_internal_destroying = false;

    HWND m_active_window = nullptr;
    HMONITOR m_current_monitor = nullptr;
    MONITORINFOEX m_monitor_info;

    std::vector<WindowHandle> m_windows;
    std::unordered_map<std::wstring, size_t> m_group_index;
    std::vector<std::vector<WindowHandle *>> m_window_groups;

    std::unique_ptr<MainWindow> m_main_window = nullptr;
    std::unique_ptr<GroupThumbnailWindow> m_group_window = nullptr;
    std::unique_ptr<ListThumbnailWindow> m_list_window = nullptr;
};

GlobalData *globalData();
