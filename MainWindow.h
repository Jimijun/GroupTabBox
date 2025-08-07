#pragma once

#include <Windows.h>

#include <array>
#include <memory>

class MainWindow
{
public:
    ~MainWindow();

    HWND hwnd() const { return m_hwnd.get(); }

    bool create(HINSTANCE instance);

    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    enum HotkeyID
    {
        HotkeyIDSwitchGroup = 0,
        HotkeyIDSwitchPrevGroup,
        HotkeyIDSwitchWindow,
        HotkeyIDSwitchPrevWindow,
        HotkeyIDSwitchMonitor,
        HotkeyIDSwitchPrevMonitor,
        HotkeyIDKeepShowingWindow,
        HotkeyIDNumber
    };

    void handleSwitchGroup(HotkeyID kid);
    void handleSwitchWindow(HotkeyID kid);
    void handleSwitchMonitor(HotkeyID kid);
    void handleShowWindow(HotkeyID kid);

    std::unique_ptr<HWND__, decltype(&DestroyWindow)> m_hwnd =  { nullptr, DestroyWindow };
    std::unique_ptr<HMENU__, decltype(&DestroyMenu)> m_tray_menu = { nullptr, DestroyMenu };
    std::array<bool, HotkeyID::HotkeyIDNumber> m_registered_hotkey = { false };
};

