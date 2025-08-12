#pragma once

#include <dwmapi.h>
#include <Windows.h>
#include <gdiplus.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#pragma comment(lib, "dwmapi.lib")

using Gdiplus::RectF;
// group by exe path and monitor
using WindowGroup = std::pair<std::wstring, HMONITOR>;

class WindowHandle
{
public:
    WindowHandle(HWND hwnd);
    WindowHandle(WindowHandle &&other);
    ~WindowHandle();

    HWND hwnd() const { return m_hwnd; }
    HICON icon() const { return m_icon.get(); }
    bool minimized() const { return m_minimized; }
    const RectF &rect() const { return m_rect; }
    const std::wstring &title() const { return m_title; }
    const std::wstring &exePath() const { return m_exe_path; }
    HMONITOR monitor() const { return m_monitor; }
    WindowGroup group() const { return { m_exe_path, m_monitor }; }

    void activate() const;
    void updateAttributes();
    void showThumbnail(HWND dst_hwnd, const RectF &dst_rect) const;
    void hideThumbnail(HWND dst_hwnd) const;

    static bool validWindow(HWND hwnd);
    static void updateUWPIconCache();

private:
    static HICON extractUWPIcon(const std::wstring &exe_path);

    HWND m_hwnd = nullptr;
    std::unique_ptr<HICON__, decltype(&DestroyIcon)> m_icon = { nullptr, DestroyIcon };
    bool m_minimized = false;
    RectF m_rect;
    std::wstring m_title;
    std::wstring m_exe_path;
    HMONITOR m_monitor = nullptr;

    mutable std::unordered_map<HWND, HTHUMBNAIL> m_thumbnails;

    static std::unordered_map<std::wstring, std::vector<BYTE>> s_UWP_icon_cache;
};
