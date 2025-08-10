#include "KeyboardHook.h"

const wchar_t *kKeyboardDllFile = L"KeyboardListener.dll";

KeyboardHook::~KeyboardHook()
{
    m_hook.reset();
    m_dll.reset();
}

bool KeyboardHook::initialize(HINSTANCE instance)
{
    m_dll = { LoadLibrary(kKeyboardDllFile), FreeLibrary };
    if (!m_dll)
        return false;

    // find symbols
    m_add_hotkey = reinterpret_cast<decltype(m_add_hotkey)>(
            GetProcAddress(m_dll.get(), "addHotkey"));
    m_mod_up_notify_once = reinterpret_cast<decltype(m_mod_up_notify_once)>(
            GetProcAddress(m_dll.get(), "modUpNotifyOnce"));
    m_hook_proc = reinterpret_cast<HOOKPROC>(GetProcAddress(m_dll.get(), "keyboardHookProc"));
    if (!m_hook_proc || !m_add_hotkey || !m_mod_up_notify_once)
        return false;

    // set hook
    m_hook = {
        SetWindowsHookEx(WH_KEYBOARD_LL, m_hook_proc, m_dll.get(), 0),
        UnhookWindowsHookEx
    };

    return m_hook.get();
}

bool KeyboardHook::addHotkey(HWND hwnd, int id, UINT modifiers, UINT key)
{
    if (!m_add_hotkey || !hwnd)
        return false;
    return m_add_hotkey(hwnd, id, modifiers, key);
}

bool KeyboardHook::modUpNotifyOnce(HWND hwnd, UINT modifiers)
{
    if (!m_mod_up_notify_once || !hwnd)
        return false;
    return m_mod_up_notify_once(hwnd, modifiers);
}
