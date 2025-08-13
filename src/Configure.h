#pragma once

#include <Windows.h>

#include <string>
#include <utility>

class Configure
{
public:
    using HotkeyPair = std::pair<UINT, UINT>;

    static Configure *instance();

    bool autoStart() const { return m_auto_start; }
    bool runAsAdmin() const { return m_run_as_admin; }

    bool ignoreMinimized() const { return m_ignore_minimized; }

    const std::string &fontFamily() const { return m_font_family; }
    float fontSize() const { return m_font_size; }
    float backgroundAlpha() const { return m_background_alpha; }

    UINT switchGroupkey() const { return m_switch_group_key; }
    bool enablePrevGroupHotkey() const { return m_enable_prev_group_hotkey; }
    UINT switchWindowkey() const { return m_switch_window_key; }
    bool enablePrevWindowHotkey() const { return m_enable_prev_window_hotkey; }
    UINT switchMonitorkey() const { return m_switch_monitor_key; }
    bool enablePrevMonitorHotkey() const { return m_enable_prev_monitor_hotkey; }
    const HotkeyPair &keepShowingHotkey() const { return m_keep_showing_hotkey; }

    bool load();

private:
    Configure() = default;
    ~Configure() = default;

    // general settings
    bool m_auto_start = false;
    bool m_run_as_admin = false;

    // window filter settings
    bool m_ignore_minimized = false;

    // ui settings
    std::string m_font_family = "Segoe UI";
    float m_font_size = 8;
    float m_background_alpha = 0.8f;

    // hotkeys settings
    UINT m_switch_group_key = VK_F1;
    bool m_enable_prev_group_hotkey = false;
    UINT m_switch_window_key = VK_F2;
    bool m_enable_prev_window_hotkey = false;
    UINT m_switch_monitor_key = VK_F3;
    bool m_enable_prev_monitor_hotkey = false;
    HotkeyPair m_keep_showing_hotkey = { 0, 0 };
};

Configure *config();
