#include "UIParam.h"
#include "Configure.h"

UIParam::UIParam()
{
    update(1.0f);
}

void UIParam::update(REAL scale)
{
    m_item_h_margin = 30 * scale;
    m_item_v_margin = 30 * scale;
    m_item_icon_margin = 5 * scale;

    m_grid_layout_param = { 240, 60, 120, 30, 30, 30 };
    m_grid_layout_param *= scale;
    m_list_layout_param = { 360, 90, 180, 45, 30, 30 };
    m_list_layout_param *= scale;

    m_list_window_width_limit = m_item_h_margin * 2 + m_list_layout_param.item_max_width;

    m_item_font_name = std::wstring(config()->fontFamily().begin(), config()->fontFamily().end());
    m_item_font_size = config()->fontSize() * scale;

    m_select_frame_margin = 10 * scale;
    m_select_frame_width = 5 * scale;
}
