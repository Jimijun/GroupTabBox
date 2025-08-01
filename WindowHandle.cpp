#include "WindowHandle.h"
#include "GlobalData.h"

#include <psapi.h>

#include <array>

static DWORD getWidnowPid(HWND hwnd)
{
    DWORD pid;
    if (GetWindowThreadProcessId(hwnd, &pid) != 0)
        return pid;
    return 0;
}

static BOOL enumChildWindowsProc(HWND hwnd, LPARAM lParam)
{
    std::array<DWORD, 2> *param = reinterpret_cast<decltype(param)>(lParam);
    DWORD pid = getWidnowPid(hwnd);
    if (pid != 0 && pid != param->at(0)) {
        param->at(1) = pid;
        return FALSE;
    }
    return TRUE;
}

static std::wstring getProcessPath(DWORD pid)
{
    std::wstring path;
    if (pid != 0) {
        std::array<wchar_t, MAX_PATH> buffer;
        std::unique_ptr<void, decltype(&CloseHandle)> process(
                OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid), CloseHandle);
        if (process && GetModuleFileNameEx(process.get(), nullptr, buffer.data(), buffer.size()) != 0)
            path = buffer.data();
    }
    return path;
}

WindowHandle::WindowHandle(HWND hwnd)
    : m_hwnd(hwnd)
{
    updateAttributes();
}

WindowHandle::WindowHandle(WindowHandle &&other)
    : m_hwnd(other.m_hwnd)
    , m_icon(std::move(other.m_icon))
    , m_minimized(other.m_minimized)
    , m_rect(other.m_rect)
    , m_title(std::move(other.m_title))
    , m_exe_path(std::move(other.m_exe_path))
    , m_thumbnails(std::move(other.m_thumbnails))
{
    other.m_hwnd = nullptr;
    other.m_icon = nullptr;
}

WindowHandle::~WindowHandle()
{
    for (auto &pair : m_thumbnails)
        DwmUnregisterThumbnail(pair.second);
}

void WindowHandle::activate() const
{
    if (m_minimized)
        ShowWindow(m_hwnd, SW_RESTORE);

    SetForegroundWindow(m_hwnd);
}

void WindowHandle::updateAttributes()
{
    m_minimized = IsIconic(m_hwnd);

    RECT rect;
    if (!m_minimized) {
        DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    } else {
        // if window is minimized, get its rect from placement
        WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
        GetWindowPlacement(m_hwnd, &placement);
        if (placement.flags & WPF_RESTORETOMAXIMIZED) {
            // window is maximized before minimized, use monitor work rect
            rect = globalData()->monitorInfo().rcWork;
        } else {
            rect = placement.rcNormalPosition;
        }
        // scale rect
        const float scale = GetDpiForWindow(m_hwnd) / 96.f;
        const LONG width = (rect.right - rect.left) * scale;
        const LONG height = (rect.bottom - rect.top) * scale;
        rect.right = rect.left + width;
        rect.bottom = rect.top + height;
    }
    m_rect = RectF(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

    std::array<wchar_t, MAX_PATH> buffer;
    GetWindowText(m_hwnd, buffer.data(), buffer.size());
    m_title = buffer.data();

    m_exe_path = getProcessPath(getWidnowPid(m_hwnd));
    // if app runs under ApplicationFrameHost.exe, search process from its child windows
    if (m_exe_path == L"C:\\Windows\\System32\\ApplicationFrameHost.exe") {
        std::array<DWORD, 2> info = { getWidnowPid(m_hwnd), 0 };
        EnumChildWindows(m_hwnd, enumChildWindowsProc, reinterpret_cast<LPARAM>(&info));
        m_exe_path = getProcessPath(info[1]);
    }

    HICON icon = reinterpret_cast<HICON>(GetClassLongPtr(m_hwnd, GCLP_HICON));
    if (!icon && !m_exe_path.empty())
        icon = ExtractIcon(globalData()->hInstance(), m_exe_path.c_str(), 0);
    m_icon = { icon, DestroyIcon };
}

void WindowHandle::showThumbnail(HWND dst_hwnd, const RectF &dst_rect) const
{
    DWM_THUMBNAIL_PROPERTIES props = {};
    auto it = m_thumbnails.find(dst_hwnd);
    if (it == m_thumbnails.end()) {
        HTHUMBNAIL thumbnail = nullptr;
        if (DwmRegisterThumbnail(dst_hwnd, m_hwnd, &thumbnail) != S_OK)
            return;

        it = m_thumbnails.emplace(dst_hwnd, thumbnail).first;
        props.dwFlags |= DWM_TNP_SOURCECLIENTAREAONLY;
        props.fSourceClientAreaOnly = FALSE;
    }

    props.dwFlags |= DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
    props.rcDestination = {
        static_cast<LONG>(std::ceil(dst_rect.X)),
        static_cast<LONG>(std::ceil(dst_rect.Y)),
        static_cast<LONG>(std::ceil(dst_rect.GetRight())),
        static_cast<LONG>(std::ceil(dst_rect.GetBottom()))
    };
    props.fVisible = TRUE;
    DwmUpdateThumbnailProperties(it->second, &props);
}

void WindowHandle::hideThumbnail(HWND dst_hwnd) const
{
    auto it = m_thumbnails.find(dst_hwnd);
    if (it == m_thumbnails.end())
        return;

    DWM_THUMBNAIL_PROPERTIES props = {};
    props.dwFlags = DWM_TNP_VISIBLE;
    props.fVisible = FALSE;
    DwmUpdateThumbnailProperties(it->second, &props);
}

bool WindowHandle::validWindow(HWND hwnd)
{
    if (GetWindow(hwnd, GW_OWNER))
        return false;

    WINDOWINFO info = {};
    if (!GetWindowInfo(hwnd, &info)
            || !(info.dwStyle & WS_VISIBLE)
            || info.dwExStyle & (WS_EX_TOOLWINDOW | WS_EX_TOPMOST))
        return false;

    DWORD cloaked = 0;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked)
        return false;

    if (MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST) != globalData()->currentMonitor())
        return false;

    return true;
}
