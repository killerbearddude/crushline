#pragma once

#include "core/Color.h"

struct UiTheme
{
    Color background{};
    Color canvas{};
    Color panel{};
    Color panelAlt{};
    Color panelHeader{};
    Color panelBorder{};
    Color panelHighlight{};

    Color textPrimary{};
    Color textSecondary{};
    Color textMuted{};

    Color accentCyan{};
    Color accentAmber{};
    Color accentRed{};
    Color accentGreen{};

    Color nodeBody{};
    Color nodeHeader{};
    Color nodeBorder{};
    Color nodeSelected{};

    Color wireNormal{};
    Color wireActive{};
    Color wireWarning{};

    Color gridMinor{};
    Color gridMajor{};

    float panelPadding = 12.0f;
    float panelGap = 8.0f;
    float borderThickness = 1.0f;
    float gridSpacing = 24.0f;
};

UiTheme MakeIndustrialDarkTheme();
