#pragma once

#include <raylib.h>

namespace game::ui::style {

inline constexpr Color Background{20, 24, 35, 255};
inline constexpr Color Panel{35, 42, 58, 255};
inline constexpr Color Text{235, 239, 244, 255};
inline constexpr Color MutedText{151, 163, 184, 255};
inline constexpr Color Accent{244, 180, 0, 255};

inline constexpr int TitleFontSize = 32;
inline constexpr int ButtonFontSize = 20;
inline constexpr int HintFontSize = 12;
inline constexpr int ButtonWidth = 160;
inline constexpr int ButtonHeight = 34;
inline constexpr int ButtonGap = 10;

} // namespace game::ui::style
