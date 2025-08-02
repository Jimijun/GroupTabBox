#include "MainWindow.h"
#include "GlobalData.h"
#include "ThumbnailWindow.h"

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
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDGroupSelectNext);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDGroupSelectPrev);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDWindowSelectNext);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDWindowSelectPrev);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDMonitorSelectNext);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDMonitorSelectPrev);
        UnregisterHotKey(m_hwnd, HotkeyID::HotkeyIDKeepShowWindow);

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

    bool success = true;
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDGroupSelectNext, MOD_ALT, VK_F1);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDGroupSelectPrev, MOD_ALT | MOD_SHIFT, VK_F1);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDWindowSelectNext, MOD_ALT, VK_F2);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDWindowSelectPrev, MOD_ALT | MOD_SHIFT, VK_F2);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDMonitorSelectNext, MOD_ALT, VK_F3);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDMonitorSelectPrev, MOD_ALT | MOD_SHIFT, VK_F3);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDKeepShowWindow, MOD_CONTROL | MOD_ALT, VK_F1);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDKeepShowWindow, MOD_CONTROL | MOD_ALT, VK_F2);
    success &= RegisterHotKey(m_hwnd, HotkeyID::HotkeyIDKeepShowWindow, MOD_CONTROL | MOD_ALT, VK_F3);

    return success;
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
        case HotkeyID::HotkeyIDGroupSelectNext:
        case HotkeyID::HotkeyIDGroupSelectPrev:
            handleSwitchGroup(static_cast<HotkeyID>(wParam));
            break;

        case HotkeyID::HotkeyIDWindowSelectNext:
        case HotkeyID::HotkeyIDWindowSelectPrev:
            handleSwitchWindow(static_cast<HotkeyID>(wParam));
            break;

        case HotkeyID::HotkeyIDMonitorSelectNext:
        case HotkeyID::HotkeyIDMonitorSelectPrev:
            handleSwitchMonitor(static_cast<HotkeyID>(wParam));
            break;

        case HotkeyID::HotkeyIDKeepShowWindow:
            handleShowWindow(static_cast<HotkeyID>(wParam));
            break;
        }
        return 0;

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
        if (kid == HotkeyID::HotkeyIDGroupSelectNext) {
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
        if (kid == HotkeyID::HotkeyIDWindowSelectNext) {
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
    group->hide();
    list->hide();

    const std::vector<HMONITOR> &monitors = globalData()->monitors();
    HMONITOR current_monitor = globalData()->currentMonitor();
    if (kid == HotkeyID::HotkeyIDMonitorSelectNext) {
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
