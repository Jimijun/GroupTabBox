#include "LayoutItem.h"
#include "GlobalData.h"
#include "UIParam.h"
#include "WindowHandle.h"

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
    : m_window(window), m_rect(rect), m_bar_height(bar_height)
{
    // align thumbnail rect
    const RectF &window_rect = window->rect();
    SizeF thumbnail_size = { 0, rect.Height - bar_height };
    thumbnail_size.Width = thumbnail_size.Height * (window_rect.Width / window_rect.Height);
    m_thumbnail_rect = {
        m_rect.X + (m_rect.Width - thumbnail_size.Width) / 2, m_rect.Y + bar_height,
        thumbnail_size.Width, thumbnail_size.Height
    };

    const UIParam *ui = globalData()->UI();
    m_icon_rect = { m_rect.X, m_rect.Y, bar_height, bar_height };
    m_icon_rect.Inflate(-ui->itemIconMargin(), -ui->itemIconMargin());

    // initialize icon bitmap data
    HICON icon = m_window->icon();
    if (icon)
        m_icon_bitmap = decltype(m_icon_bitmap)(Bitmap::FromHICON(icon));
}

void LayoutItem::setPosition(const PointF &pos)
{
    const PointF offset = { pos.X - m_rect.X, pos.Y - m_rect.Y };
    m_rect.Offset(offset);
    m_thumbnail_rect.Offset(offset);
    m_icon_rect.Offset(offset);
}

void LayoutItem::drawInfo(Graphics *graphics) const
{
    if (!graphics)
        return;

    const UIParam *ui = globalData()->UI();

    // draw background
    Gdiplus::SolidBrush back_brush{Gdiplus::Color(ui->itemBackgroundColor())};
    graphics->FillRectangle(&back_brush, m_rect);

    // draw icon
    if (m_icon_bitmap)
        graphics->DrawImage(m_icon_bitmap.get(), m_icon_rect);

    // draw title
    Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
    format.SetTrimming(Gdiplus::StringTrimming::StringTrimmingEllipsisCharacter);
    format.SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);
    format.SetAlignment(Gdiplus::StringAlignment::StringAlignmentNear);
    Gdiplus::Font font(ui->itemFontName().c_str(), ui->itemFontSize());
    Gdiplus::SolidBrush text_brush(Gdiplus::Color::White);
    RectF title_rect = {
        m_icon_rect.GetRight() + ui->itemIconMargin() * 2, m_icon_rect.Y,
        m_rect.Width - m_icon_rect.Width - ui->itemIconMargin() * 4, m_icon_rect.Height
    };
    graphics->DrawString(m_window->title().c_str(), -1, &font, title_rect, &format, &text_brush);
}
