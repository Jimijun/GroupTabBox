#pragma once

#include <Windows.h>

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
        HotkeyIDGroupSelectNext = 0,
        HotkeyIDGroupSelectPrev,
        HotkeyIDWindowSelectNext,
        HotkeyIDWindowSelectPrev,
        HotkeyIDMonitorSelectNext,
        HotkeyIDMonitorSelectPrev,
        HotkeyIDKeepShowWindow
    };

    void handleSwitchGroup(HotkeyID kid);
    void handleSwitchWindow(HotkeyID kid);
    void handleSwitchMonitor(HotkeyID kid);
    void handleShowWindow(HotkeyID kid);

    HWND m_hwnd = nullptr;
};

