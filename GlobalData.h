#pragma once

#include "WindowHandle.h"

using Gdiplus::REAL;

class Configure;
class GroupThumbnailWindow;
class KeyboardHook;
class ListThumbnailWindow;
class MainWindow;
class UIParam;

class GlobalData
{
    friend BOOL enumWindowsProc(HWND hwnd, LPARAM lParam);
    friend BOOL enumMonitorsProc(HMONITOR monitor, HDC hdc, LPRECT lprc, LPARAM lParam);

public:
    static GlobalData *instance();

    HINSTANCE hInstance() const { return m_hinstance; }
    HWND activeWindow() const { return m_active_window; }
    const HMONITOR &currentMonitor() const { return m_current_monitor; }
    const MONITORINFOEX &monitorInfo() const { return m_monitor_info; }
    const std::vector<HMONITOR> &monitors() const { return m_monitors; }
    REAL monitorScale() const { return m_monitor_scale; }
    const Configure *config() const { return m_config.get(); }
    const UIParam *UI() const { return m_ui.get(); }
    const std::vector<WindowHandle> &windows() const { return m_windows; }
    const std::vector<std::vector<WindowHandle *>> &windowGroups() const { return m_window_groups; }
    const std::vector<WindowHandle *> &windowsFromGroup(const std::wstring &group_name) const;
    RectF groupWindowLimitRect() const;
    RectF listWindowLimitRect() const;
    MainWindow *mainWindow() const { return m_main_window.get(); }
    GroupThumbnailWindow *groupWindow() const { return m_group_window.get(); }
    ListThumbnailWindow *listWindow() const { return m_list_window.get(); }
    KeyboardHook *keyboardHook() const { return m_keyboard_hook.get(); }

    void setCurrentMonitor(HMONITOR monitor);

    bool initialize(HINSTANCE instance);
    void destroy();
    bool update(HMONITOR monitor);
    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void activateWindow(const WindowHandle *window);

private:
    GlobalData() = default;
    ~GlobalData() = default;

    HINSTANCE m_hinstance = nullptr;

    HWND m_active_window = nullptr;
    HMONITOR m_current_monitor = nullptr;
    MONITORINFOEX m_monitor_info;
    std::vector<HMONITOR> m_monitors;
    REAL m_monitor_scale = 1.f;

    std::unique_ptr<Configure> m_config = nullptr;
    std::unique_ptr<UIParam> m_ui = nullptr;

    std::vector<WindowHandle> m_windows;
    std::unordered_map<std::wstring, size_t> m_group_index;
    std::vector<std::vector<WindowHandle *>> m_window_groups;

    std::unique_ptr<MainWindow> m_main_window = nullptr;
    std::unique_ptr<GroupThumbnailWindow> m_group_window = nullptr;
    std::unique_ptr<ListThumbnailWindow> m_list_window = nullptr;

    std::unique_ptr<KeyboardHook> m_keyboard_hook = nullptr;
};

GlobalData *globalData();
