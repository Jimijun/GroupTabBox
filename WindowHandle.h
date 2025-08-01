#pragma once

#include <dwmapi.h>
#include <gdiplus.h>
#include <Windows.h>

#include <memory>
#include <unordered_map>
#include <string>

#pragma comment(lib, "dwmapi.lib")

using Gdiplus::RectF;

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

    void activate() const;
    void updateAttributes();
    void showThumbnail(HWND dst_hwnd, const RectF &dst_rect) const;
    void hideThumbnail(HWND dst_hwnd) const;

    static bool validWindow(HWND hwnd);

private:
    HWND m_hwnd = nullptr;
    std::unique_ptr<HICON__, decltype(&DestroyIcon)> m_icon = { nullptr, DestroyIcon };
    bool m_minimized = false;
    RectF m_rect;
    std::wstring m_title;
    std::wstring m_exe_path;

    mutable std::unordered_map<HWND, HTHUMBNAIL> m_thumbnails;
};
