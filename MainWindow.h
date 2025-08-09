#pragma once

#include <Windows.h>

#include <memory>

class MainWindow
{
public:
    HWND hwnd() const { return m_hwnd.get(); }

    bool create(HINSTANCE instance);

    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    // events id
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
    enum MODID
    {
        MODIDALT = 0
    };

    void handleSwitchGroup(HotkeyID kid);
    void handleSwitchWindow(HotkeyID kid);
    void handleSwitchMonitor(HotkeyID kid);
    void handleShowWindow(HotkeyID kid);

    std::unique_ptr<HWND__, decltype(&DestroyWindow)> m_hwnd =  { nullptr, DestroyWindow };
    std::unique_ptr<HMENU__, decltype(&DestroyMenu)> m_tray_menu = { nullptr, DestroyMenu };
    std::unique_ptr<HINSTANCE__, decltype(&FreeLibrary)> m_keyboard_dll = { nullptr, FreeLibrary };
    std::unique_ptr<HHOOK__, decltype(&UnhookWindowsHookEx)> m_keyboard_hook = { nullptr, UnhookWindowsHookEx };
};

