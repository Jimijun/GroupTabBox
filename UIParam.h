#pragma once

#include <Windows.h>
#include <gdiplus.h>

#include <string>

using Gdiplus::ARGB;
using Gdiplus::REAL;

class UIParam
{
public:
    UIParam();

    REAL itemHMargin() const { return m_item_h_margin; }
    REAL itemVMargin() const { return m_item_v_margin; }
    REAL itemIconMargin() const { return m_item_icon_margin; }

    ARGB backgroundColor() const { return m_kBackgroundColor; }
    ARGB itemBackgroundColor() const { return m_kItembackgroundColor; }

    REAL gridItemMaxWidth() const { return m_grid_layout_param.item_max_width; }
    REAL gridItemMinWidth() const { return m_grid_layout_param.item_min_width; }
    REAL gridItemMaxHeight() const { return m_grid_layout_param.item_max_height; }
    REAL gridBarHeight() const { return m_grid_layout_param.bar_height; }
    REAL gridEdgeHMargin() const { return m_grid_layout_param.edge_h_margin; }
    REAL gridEdgeVMargin() const { return m_grid_layout_param.edge_v_margin; }

    REAL listItemMaxWidth() const { return m_list_layout_param.item_max_width; }
    REAL listItemMinWidth() const { return m_list_layout_param.item_min_width; }
    REAL listItemMaxHeight() const { return m_list_layout_param.item_max_height; }
    REAL listBarHeight() const { return m_list_layout_param.bar_height; }
    REAL listEdgeHMargin() const { return m_list_layout_param.edge_h_margin; }
    REAL listEdgeVMargin() const { return m_list_layout_param.edge_v_margin; }

    ARGB gridItemShadowColor() const { return m_kGridItemShadowColor; }

    REAL itemFontSize() const { return m_item_font_size; }
    const std::wstring &itemFontName() const { return m_item_font_name; }

    REAL groupWindowWidthLimitRadio() const { return m_kGroupWindowWidthLimitRadio; }
    REAL groupWindowHeightLimitRadio() const { return m_kGroupWindowHeightLimitRadio; }
    REAL listWindowWidthLimit() const { return m_list_window_width_limit; }

    REAL selectFrameMargin() const { return m_select_frame_margin; }
    REAL selectFrameWidth() const { return m_select_frame_width; }
    ARGB selectFrameColor() const { return m_kSelectFrameColor; }

    void update(REAL scale);

private:
    // item and layout parameter
    REAL m_item_h_margin;
    REAL m_item_v_margin;
    REAL m_item_icon_margin;

    const ARGB m_kBackgroundColor = 0xFF808080;
    const ARGB m_kItembackgroundColor = 0xFF272727;

    struct {
        REAL item_max_width, item_min_width, item_max_height, bar_height, edge_h_margin, edge_v_margin;
        void operator*=(REAL scale)
        {
            item_max_width *= scale;
            item_min_width *= scale;
            item_max_height *= scale;
            bar_height *= scale;
            edge_h_margin *= scale;
            edge_v_margin *= scale;
        }
    } m_grid_layout_param, m_list_layout_param;

    const ARGB m_kGridItemShadowColor = 0xFF404040;

    REAL m_item_font_size;
    std::wstring m_item_font_name = L"Segoe UI";

    // window placement parameter
    const REAL m_kGroupWindowWidthLimitRadio = 0.8f;
    const REAL m_kGroupWindowHeightLimitRadio = 0.8f;
    REAL m_list_window_width_limit;

    // select frame
    REAL m_select_frame_margin;
    REAL m_select_frame_width;
    const ARGB m_kSelectFrameColor = 0xFF0063B1;
};
