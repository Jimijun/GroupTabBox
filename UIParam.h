#pragma once

#include <cstdint>

// item and layout parameter
const float kItemHMargin = 30;
const float kItemVMargin = 30;
const float kItemIconMargin = 5;

const uint32_t kBackgroundColor = 0xFF808080;
const uint32_t kItemBackgroundColor = 0xFF272727;

const float kGridItemMaxWidth = 240;
const float kGridItemMaxHeight = 120;
const float kGridItemBarHeight = 30;
const float kGridEdgeHMargin = 30;
const float kGridEdgeVMargin = 30;
const uint32_t kGridItemShadowColor = 0xFF404040;

const float kListItemMaxWidth = 360;
const float kListItemMaxHeight = 180;
const float kListItemBarHeight = 45;
const float kListEdgeHMargin = 30;
const float kListEdgeVMargin = 30;

// window placement parameter
const float kGroupWindowWidthLimitRadio = 0.8f;
const float kGroupWindowHeightLimitRadio = 0.8f;
const float kListWindowWidthLimit = kListItemMaxWidth + kListEdgeHMargin * 2;

// wheel scroll speed
const float kWheelDeltaPixel = 30.f / 120.f;

// select frame
const float kSelectFrameMargin = 10;
const float kSelectFrameWidth = 5;
const uint32_t kSelectFrameColor = 0xFF0063B1;
