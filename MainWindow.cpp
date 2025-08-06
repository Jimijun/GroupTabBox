#include "MainWindow.h"
#include "GlobalData.h"
#include "resource.h"
#include "ThumbnailWindow.h"

#include <CommCtrl.h>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' \
        version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

const UINT WMAPP_TRAYCALLBACK = WM_APP + 1;

const UINT kTrayIconID = 114;
const UINT kTrayMenuExitID = 514;

static HMONITOR monitorFromActiveWindow()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return nullptr;
    return MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
}

static HMONITOR monitorFromCursor()
{
    POINT pos;
    GetCursorPos(&pos);
    return MonitorFromPoint(pos, MONITOR_DEFAULTTONEAREST);
}

MainWindow::~MainWindow()
{
    if (m_hwnd) {
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDSwitchGroup);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDSwitchPrevGroup);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDSwitchWindow);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDSwitchPrevWindow);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDSwitchMonitor);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDSwitchPrevMonitor);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDKeepShowingWindow);

        DestroyWindow(m_hwnd);
    }
}

bool MainWindow::create(HINSTANCE instance)
{
    if (m_hwnd)
        return true;

    m_hwnd = CreateWindow(
        L"GroupTabBox", L"MainWindow",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, instance, nullptr
    );
    if (!m_hwnd)
        return false;

    const Configure *config = globalData()->config();
    bool success = true;

#define REGISTER_HELPER(id, key, enable_prev, prev_id) \
    if (key != 0) success &= RegisterHotKey(m_hwnd, id, MOD_ALT, key); \
    if (enable_prev) success &= RegisterHotKey(m_hwnd, prev_id, MOD_ALT | MOD_SHIFT, key); \
    if (!success) return false

    REGISTER_HELPER(HotkeyID::HotkeyIDSwitchGroup, config->switchGroupkey(),
            config->enablePrevGroupHotkey(), HotkeyID::HotkeyIDSwitchPrevGroup);
    REGISTER_HELPER(HotkeyID::HotkeyIDSwitchWindow, config->switchWindowkey(),
            config->enablePrevWindowHotkey(), HotkeyID::HotkeyIDSwitchPrevWindow);
    REGISTER_HELPER(HotkeyID::HotkeyIDSwitchMonitor, config->switchMonitorkey(),
            config->enablePrevMonitorHotkey(), HotkeyID::HotkeyIDSwitchPrevMonitor);

#undef REGISTER_HELPER

    const Configure::HotkeyPair &hotkey_pair = config->keepShowingHotkey();
    if (hotkey_pair.first != 0 && hotkey_pair.second != 0) {
        if (!RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDKeepShowingWindow,
                hotkey_pair.first, hotkey_pair.second))
            return false;
    }

    // add tray icon
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(nid);
    nid.hWnd = m_hwnd;
    nid.uID = kTrayIconID;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    LoadIconMetric(instance, MAKEINTRESOURCE(IDI_ICON1), LIM_SMALL, &nid.hIcon);
    const WCHAR tip[] = L"GroupTabBox";
    std::copy(tip, tip + sizeof(tip), nid.szTip);
    nid.uCallbackMessage = WMAPP_TRAYCALLBACK;
    if (!Shell_NotifyIcon(NIM_ADD, &nid))
        return false;

    // create tray menu
    m_tray_menu = { CreatePopupMenu(), DestroyMenu };
    if (!m_tray_menu)
        return false;
    AppendMenu(m_tray_menu.get(), MF_STRING, kTrayMenuExitID, L"Exit");

    return true;
}

LRESULT MainWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        m_hwnd = nullptr;
        PostQuitMessage(0);
        return 0;

    case WM_HOTKEY:
        switch (static_cast<HotkeyID>(wParam)) {
        case HotkeyID::HotkeyIDSwitchGroup:
        case HotkeyID::HotkeyIDSwitchPrevGroup:
            handleSwitchGroup(static_cast<HotkeyID>(wParam));
            break;

        case HotkeyID::HotkeyIDSwitchWindow:
        case HotkeyID::HotkeyIDSwitchPrevWindow:
            handleSwitchWindow(static_cast<HotkeyID>(wParam));
            break;

        case HotkeyID::HotkeyIDSwitchMonitor:
        case HotkeyID::HotkeyIDSwitchPrevMonitor:
            handleSwitchMonitor(static_cast<HotkeyID>(wParam));
            break;

        case HotkeyID::HotkeyIDKeepShowingWindow:
            handleShowWindow(static_cast<HotkeyID>(wParam));
            break;
        }
        return 0;

    case WM_COMMAND:
        // exit
        if (LOWORD(wParam) == kTrayMenuExitID) {
            PostQuitMessage(0);
            return 0;
        }
        break;

    case WMAPP_TRAYCALLBACK:
        // show tray menu
        if (LOWORD(wParam) == kTrayIconID && LOWORD(lParam) == WM_RBUTTONDOWN) {
            POINT pos;
            GetCursorPos(&pos);
            SetForegroundWindow(m_hwnd);
            TrackPopupMenuEx(m_tray_menu.get(), TPM_LEFTALIGN | TPM_BOTTOMALIGN,
                    pos.x, pos.y, m_hwnd, nullptr);
            PostMessage(m_hwnd, WM_NULL, 0, 0);
            return 0;
        }
        break;

    default:
        break;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::handleSwitchGroup(HotkeyID kid)
{
    GroupThumbnailWindow *group = globalData()->groupWindow();
    if (!group)
        return;

    if (!group->visible()) {
        if (!globalData()->update(monitorFromActiveWindow()))
            return;
        group->show(false);
    }

    if (group->visible()) {
        if (kid == HotkeyID::HotkeyIDSwitchGroup) {
            group->selectNext();
        } else {
            group->selectPrev();
        }
    }
}

void MainWindow::handleSwitchWindow(HotkeyID kid)
{
    ListThumbnailWindow *list = globalData()->listWindow();
    if (!list)
        return;

    if (!list->visible()) {
        if (!globalData()->update(monitorFromActiveWindow()))
            return;

        // show the group of the active window
        HWND active_window = globalData()->activeWindow();
        if (!active_window)
            return;
        const std::vector<WindowHandle> &windows = globalData()->windows();
        auto it = std::find_if(windows.begin(), windows.end(),
            [active_window] (const WindowHandle &handle) {
                return handle.hwnd() == active_window;
            }
        );
        list->setGroup(it->exePath());

        list->show(false);
    }

    if (list->visible()) {
        if (kid == HotkeyID::HotkeyIDSwitchWindow) {
            list->selectNext();
        } else {
            list->selectPrev();
        }
    }
}

void MainWindow::handleSwitchMonitor(HotkeyID kid)
{
    GroupThumbnailWindow *group = globalData()->groupWindow();
    ListThumbnailWindow *list = globalData()->listWindow();
    if (!group || !list)
        return;

    if (!group->visible() || !list->visible()) {
        if (!globalData()->update(monitorFromActiveWindow()))
            return;
    }

    if (globalData()->monitors().size() <= 1)
        return;

    group->hide();
    list->hide();

    const std::vector<HMONITOR> &monitors = globalData()->monitors();
    HMONITOR current_monitor = globalData()->currentMonitor();
    if (kid == HotkeyID::HotkeyIDSwitchMonitor) {
        auto it = std::find(monitors.begin(), monitors.end(), current_monitor);
        if (it == monitors.end())
            return;
        int index = it - monitors.begin();
        globalData()->setCurrentMonitor(monitors[(index + 1) % monitors.size()]);
    } else {
        auto it = std::find(monitors.begin(), monitors.end(), current_monitor);
        if (it == monitors.end())
            return;
        int index = it - monitors.begin();
        globalData()->setCurrentMonitor(monitors[(index - 1 + monitors.size()) % monitors.size()]);
    }

    group->show(false);
    list->show(false);
}

void MainWindow::handleShowWindow(HotkeyID kid)
{
    GroupThumbnailWindow *group = globalData()->groupWindow();
    ListThumbnailWindow *list = globalData()->listWindow();
    if (!group || !list)
        return;

    if (!group->visible() && !list->visible()) {
        if (!globalData()->update(monitorFromCursor()))
            return;
        group->show(true);
        list->show(true);
    }
    if (group->visible())
        group->keepShowing(true);
    if (list->visible())
        list->keepShowing(true);
}
