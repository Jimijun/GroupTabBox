#ifndef UNICODE
#define UNICODE
#endif

#include "GlobalData.h"

#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // avoid handling messages while internal destroying
    if (!globalData()->internalDestroying())
        return globalData()->handleMessage(hwnd, uMsg, wParam, lParam);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        PWSTR lpCmdLine, int nCmdShow)
{
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
    RegisterClass(&wc);

    if (!globalData()->initialize(hInstance))
        return 1;

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
