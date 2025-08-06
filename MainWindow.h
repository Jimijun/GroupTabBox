#pragma once

#include <Windows.h>

#include <memory>

class MainWindow
{
public:
    ~MainWindow();

    HWND hwnd() const { return m_hwnd; }

    bool create(HINSTANCE instance);

    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    enum HotkeyID
    {
        HotkeyIDSwitchGroup = 0,
        HotkeyIDSwitchPrevGroup,
        HotkeyIDSwitchWindow,
        HotkeyIDSwitchPrevWindow,
        HotkeyIDSwitchMonitor,
        HotkeyIDSwitchPrevMonitor,
        HotkeyIDKeepShowingWindow
    };

    void handleSwitchGroup(HotkeyID kid);
    void handleSwitchWindow(HotkeyID kid);
    void handleSwitchMonitor(HotkeyID kid);
    void handleShowWindow(HotkeyID kid);

    HWND m_hwnd = nullptr;
    std::unique_ptr<HMENU__, decltype(&DestroyMenu)> m_tray_menu = { nullptr, DestroyMenu };
};

