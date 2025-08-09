#include "pch.h"

#include <array>
#include <unordered_set>
#include <unordered_map>

#define DLLEXPORT extern "C" __declspec(dllexport)

using HotkeyPair = std::pair<char, UINT>;
using NotifyPair = std::pair<HWND, int>;

template <typename T>
inline void hashCombine(std::size_t &seed, const T &v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename U, typename V>
struct std::hash<std::pair<U, V>>
{
    size_t operator()(const std::pair<U, V> &tt) const
    {
        size_t seed = 0;
        hashCombine(seed, tt.first);
        hashCombine(seed, tt.second);
        return seed;
    }
};

const UINT WMAPP_HOTKEY = WM_APP + 2;
const UINT WMAPP_MODUP = WM_APP + 3;

// shared memory
#pragma data_seg(".shared")
std::unordered_map<HotkeyPair, NotifyPair> gHotkeys = {};
std::array<std::unordered_set<NotifyPair>, 5> gModUpNotify = {};
#pragma data_seg()
#pragma comment(linker, "/section:.shared,RWS")

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

static inline char toModifierBit(UINT key)
{
    if (key < VK_LSHIFT || key > VK_RMENU)
        return 0;
    return 1 << ((key - VK_LSHIFT) >> 1);
}

DLLEXPORT bool addHotkey(HWND hwnd, int id, UINT modifiers, UINT key)
{
    // ALT | CTRL | SHIFT (lower 3 bits)
    char mod = static_cast<char>((modifiers & MOD_ALT) > 0) << 2
            | static_cast<char>((modifiers & MOD_CONTROL) > 0) << 1
            | static_cast<char>((modifiers & MOD_SHIFT) > 0);
    HotkeyPair hotkey(mod, key);
    auto it = gHotkeys.find(hotkey);
    if (it != gHotkeys.end())
        return false;
    gHotkeys.emplace(hotkey, NotifyPair(hwnd, id));
    return true;
}

DLLEXPORT bool addModUpNotify(HWND hwnd, int id, UINT modifiers)
{
    char index = 0;
    // only support single modifier
    // convert modifiers into index (ALT | CTRL | SHIFT)
    switch (modifiers) {
    case MOD_ALT:
        index = 4;
        break;

    case MOD_CONTROL:
        index = 2;
        break;

    case MOD_SHIFT:
        index = 1;
        break;

    default:
        return false;
    }
    gModUpNotify[index].emplace(NotifyPair(hwnd, id));
    return true;
}

DLLEXPORT LRESULT CALLBACK keyboardHookProc(int code, WPARAM wParam, LPARAM lParam)
{
    static char mod_state = 0;

    if (code < 0)
        return CallNextHookEx(nullptr, code, wParam, lParam);

    KBDLLHOOKSTRUCT *key_info = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
    char mod = toModifierBit(key_info->vkCode);

    if (key_info->flags & LLKHF_UP) {
        // key up
        mod_state &= ~mod;
        if (mod != 0) {
            // modifier mod up
            for (const auto &pair : gModUpNotify[mod])
                SendMessage(pair.first, WMAPP_MODUP, pair.second, 0);
        }
    } else {
        if (mod != 0) {
            // modifier key down
            mod_state |= mod;
        } else {
            // normal key down
            HotkeyPair hotkey(mod_state, key_info->vkCode);
            auto it = gHotkeys.find(hotkey);
            if (it != gHotkeys.end()) {
                SendMessage(it->second.first, WMAPP_HOTKEY, it->second.second, 0);
                return 1;
            }
        }
    }

    return CallNextHookEx(nullptr, code, wParam, lParam);
}
