#pragma once

#include <Windows.h>

#include <memory>

class KeyboardHook
{
public:
    bool initialize(HINSTANCE instance);

    bool addHotkey(HWND hwnd, int id, UINT modifiers, UINT key);
    bool modUpNotifyOnce(HWND hwnd, UINT modifiers);

private:
    std::unique_ptr<HINSTANCE__, decltype(&FreeLibrary)> m_dll = { nullptr, FreeLibrary };
    std::unique_ptr<HHOOK__, decltype(&UnhookWindowsHookEx)> m_hook = { nullptr, UnhookWindowsHookEx };

    bool (*m_add_hotkey)(HWND hwnd, int id, UINT modifiers, UINT key) = nullptr;
    bool (*m_mod_up_notify_once)(HWND hwnd, UINT modifiers) = nullptr;
    HOOKPROC m_hook_proc = nullptr;
};

