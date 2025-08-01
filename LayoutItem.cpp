#include "LayoutItem.h"
#include "GlobalData.h"
#include "UIParam.h"

SizeF LayoutItem::scaledSize(REAL width, REAL height, REAL width_limit, REAL height_limit,
        REAL bar_height)
{
    const REAL ratio = width / height;
    // scale base on thumbnail rectangle, exclude bar
    height_limit -= bar_height;
    if (ratio > width_limit / height_limit) {
        width = width_limit;
        height = width / ratio;
    } else {
        height = height_limit;
        width = height * ratio;
    }
    // add bar when return
    return { width, height + bar_height };
}

LayoutItem::LayoutItem(WindowHandle *window, const RectF &rect, const REAL bar_height)
    : m_window(window), m_bar_height(bar_height)
{
    setRect(rect);

    // initialize icon bitmap data
    HICON icon = m_window->icon();
    if (icon)
        m_icon_bitmap = decltype(m_icon_bitmap)(Bitmap::FromHICON(icon));
}

RectF LayoutItem::thumbnailRect() const
{
    return {
        m_rect.X, m_rect.Y + m_bar_height,
        m_rect.Width, m_rect.Height - m_bar_height
    };
}

void LayoutItem::setRect(const RectF &rect)
{
    m_rect = rect;

    const UIParam &ui = globalData()->UI();
    // update icon rect
    const float icon_size = m_bar_height - ui.itemIconMargin() * 2;
    m_icon_rect = { ui.itemIconMargin(), ui.itemIconMargin(), icon_size, icon_size };
    m_icon_rect.Offset(m_rect.X, m_rect.Y);
}

void LayoutItem::drawInfo(Graphics *graphics) const
{
    if (!graphics)
        return;

    const UIParam &ui = globalData()->UI();

    // draw background
    Gdiplus::SolidBrush back_brush{Gdiplus::Color(ui.itemBackgroundColor())};
    graphics->FillRectangle(&back_brush, m_rect);

    // draw icon
    if (m_icon_bitmap)
        graphics->DrawImage(m_icon_bitmap.get(), m_icon_rect);

    // draw title
    Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
    format.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);
    format.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
    format.SetAlignment(Gdiplus::StringAlignment::StringAlignmentNear);
    Gdiplus::Font font(ui.itemFontName(), ui.itemFontSize());
    Gdiplus::SolidBrush text_brush(Gdiplus::Color::White);
    RectF title_rect = {
        m_icon_rect.GetRight() + ui.itemIconMargin() * 2, m_icon_rect.Y,
        m_rect.Width - m_icon_rect.Width - ui.itemIconMargin() * 3, m_icon_rect.Height
    };
    graphics->DrawString(m_window->title().c_str(), -1, &font, title_rect, &format, &text_brush);
}
