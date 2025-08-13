#ifndef UNICODE
#define UNICODE
#endif

#include "Configure.h"
#include "GlobalData.h"
#include "resource.h"
#include "utils/ProgramUtils.h"

#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#pragma data_seg("RunMutex")
bool gRunningInstance = false;
#pragma data_seg()
#pragma comment(linker, "/section:RunMutex,RWS")

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return globalData()->handleMessage(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        PWSTR lpCmdLine, int nCmdShow)
{
    config()->load();

    if (!isAlreadyAdmin() && config()->runAsAdmin()) {
        // start an admin process and exit current one
        runAsAdmin();
        return 0;
    }

    // check if an instance is already running
    if (gRunningInstance) {
        MessageBox(nullptr, L"An instance is already running.", nullptr, MB_OK);
        return 0;
    }
    gRunningInstance = true;

    setAutoStart(L"GroupTabBox", config()->autoStart());

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    struct GdiToken
    {
        GdiToken()
        {
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
        }
        ~GdiToken() { Gdiplus::GdiplusShutdown(gdiplusToken); }
        ULONG_PTR gdiplusToken;
    } token;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GroupTabBox";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    RegisterClass(&wc);

    if (!globalData()->initialize(hInstance)) {
        globalData()->destroy();
        return 1;
    }

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    globalData()->destroy();
    return 0;
}
