#pragma once

#include <Windows.h>

#include <string>
#include <utility>

class Configure
{
public:
    Configure() = default;

    using HotkeyPair = std::pair<UINT, UINT>;

    bool ignoreMinimized() const { return m_ignore_minimized; }

    const std::string &fontFamily() const { return m_font_family; }
    float fontSize() const { return m_font_size; }

    const HotkeyPair &switchGroupHotkey() const { return m_switch_group_hotkey; }
    const HotkeyPair &switchPrevGroupHotkey() const { return m_switch_prev_group_hotkey; }
    const HotkeyPair &switchWindowHotkey() const { return m_switch_window_hotkey; }
    const HotkeyPair &switchPrevWindowHotkey() const { return m_switch_prev_window_hotkey; }
    const HotkeyPair &switchMonitorHotkey() const { return m_switch_monitor_hotkey; }
    const HotkeyPair &switchPrevMonitorHotkey() const { return m_switch_prev_monitor_hotkey; }
    const HotkeyPair &keepShowingWindowHotkey() const { return m_keep_showing_window; }

    bool load();

private:
    // window filter settings
    bool m_ignore_minimized = false;

    // ui settings
    std::string m_font_family = "Segoe UI";
    float m_font_size = 8;

    // hotkeys settings
    HotkeyPair m_switch_group_hotkey = { MOD_ALT, VK_F1 };
    HotkeyPair m_switch_prev_group_hotkey = { 0, 0 };
    HotkeyPair m_switch_window_hotkey = { MOD_ALT, VK_F2 };
    HotkeyPair m_switch_prev_window_hotkey = { 0, 0 };
    HotkeyPair m_switch_monitor_hotkey = { MOD_ALT, VK_F3 };
    HotkeyPair m_switch_prev_monitor_hotkey = { 0, 0 };
    HotkeyPair m_keep_showing_window = { 0, 0 };
};
