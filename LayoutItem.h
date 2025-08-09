#pragma once

#include <Windows.h>
#include <gdiplus.h>

#include <memory>

using Gdiplus::Bitmap;
using Gdiplus::Graphics;
using Gdiplus::PointF;
using Gdiplus::REAL;
using Gdiplus::RectF;
using Gdiplus::SizeF;

class WindowHandle;

class LayoutItem
{
public:
    LayoutItem(WindowHandle *window, const RectF &rect, const REAL bar_height);

    WindowHandle *windowHandle() { return m_window; }
    const WindowHandle *windowHandle() const { return m_window; }
    const RectF &rect() const { return m_rect; }
    RectF thumbnailRect() const { return m_thumbnail_rect; }

    void setPosition(const PointF &pos);

    void drawInfo(Graphics *graphics) const;

    static SizeF scaledSize(REAL cx, REAL cy, REAL width_limit, REAL height_limit,
            REAL bar_height);

private:
    WindowHandle *m_window = nullptr;
    RectF m_rect;
    RectF m_thumbnail_rect;
    REAL m_bar_height = 0;
    std::unique_ptr<Bitmap> m_icon_bitmap = nullptr;
    RectF m_icon_rect;
};
