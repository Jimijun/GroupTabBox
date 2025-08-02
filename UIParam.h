#pragma once

#include <cstdint>

class UIParam
{
public:
    UIParam() { update(1.0f); }

    float itemHMargin() const { return m_item_h_margin; }
    float itemVMargin() const { return m_item_v_margin; }
    float itemIconMargin() const { return m_item_icon_margin; }

    uint32_t backgroundColor() const { return m_kBackgroundColor; }
    uint32_t itemBackgroundColor() const { return m_kItembackgroundColor; }

    float gridItemMaxWidth() const { return m_grid_layout_param.item_max_width; }
    float gridItemMinWidth() const { return m_grid_layout_param.item_min_width; }
    float gridItemMaxHeight() const { return m_grid_layout_param.item_max_height; }
    float gridBarHeight() const { return m_grid_layout_param.bar_height; }
    float gridEdgeHMargin() const { return m_grid_layout_param.edge_h_margin; }
    float gridEdgeVMargin() const { return m_grid_layout_param.edge_v_margin; }

    float listItemMaxWidth() const { return m_list_layout_param.item_max_width; }
    float listItemMinWidth() const { return m_list_layout_param.item_min_width; }
    float listItemMaxHeight() const { return m_list_layout_param.item_max_height; }
    float listBarHeight() const { return m_list_layout_param.bar_height; }
    float listEdgeHMargin() const { return m_list_layout_param.edge_h_margin; }
    float listEdgeVMargin() const { return m_list_layout_param.edge_v_margin; }

    uint32_t gridItemShadowColor() const { return m_kGridItemShadowColor; }

    float itemFontSize() const { return m_item_font_size; }
    const wchar_t *itemFontName() const { return m_kItemFontName; }

    float groupWindowWidthLimitRadio() const { return m_kGroupWindowWidthLimitRadio; }
    float groupWindowHeightLimitRadio() const { return m_kGroupWindowHeightLimitRadio; }
    float listWindowWidthLimit() const { return m_list_window_width_limit; }

    float wheelDeltaPixel() const { return m_kWheelDeltaPixel; }

    float selectFrameMargin() const { return m_select_frame_margin; }
    float selectFrameWidth() const { return m_select_frame_width; }
    uint32_t selectFrameColor() const { return m_kSelectFrameColor; }

    void update(float scale)
    {
        m_item_h_margin = 30 * scale;
        m_item_v_margin = 30 * scale;
        m_item_icon_margin = 5 * scale;

        m_grid_layout_param = { 240, 60, 120, 30, 30, 30 };
        m_grid_layout_param *= scale;
        m_list_layout_param = { 360, 90, 180, 45, 30, 30 };
        m_list_layout_param *= scale;

        m_list_window_width_limit = m_item_h_margin * 2 + m_list_layout_param.item_max_width;

        m_item_font_size = 6 * scale;

        m_select_frame_margin = 10 * scale;
        m_select_frame_width = 5 * scale;
    }

private:
    // item and layout parameter
    float m_item_h_margin;
    float m_item_v_margin;
    float m_item_icon_margin;

    const uint32_t m_kBackgroundColor = 0xFF808080;
    const uint32_t m_kItembackgroundColor = 0xFF272727;

    struct {
        float item_max_width, item_min_width, item_max_height, bar_height, edge_h_margin, edge_v_margin;
        void operator*=(float scale)
        {
            item_max_width *= scale;
            item_min_width *= scale;
            item_max_height *= scale;
            bar_height *= scale;
            edge_h_margin *= scale;
            edge_v_margin *= scale;
        }
    } m_grid_layout_param, m_list_layout_param;

    const uint32_t m_kGridItemShadowColor = 0xFF404040;

    float m_item_font_size;
    const wchar_t *m_kItemFontName = L"Segoe UI";

    // window placement parameter
    const float m_kGroupWindowWidthLimitRadio = 0.8f;
    const float m_kGroupWindowHeightLimitRadio = 0.8f;
    float m_list_window_width_limit;

    // wheel scroll speed
    const float m_kWheelDeltaPixel = 30.f / 120.f;

    // select frame
    float m_select_frame_margin;
    float m_select_frame_width;
    const uint32_t m_kSelectFrameColor = 0xFF0063B1;
};
