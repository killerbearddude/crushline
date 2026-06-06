#include "ui/Theme.h"

UiTheme MakeIndustrialDarkTheme()
{
    UiTheme theme;

    theme.background = {0.035f, 0.038f, 0.042f, 1.0f};
    theme.canvas = {0.025f, 0.028f, 0.031f, 1.0f};
    theme.panel = {0.070f, 0.075f, 0.080f, 1.0f};
    theme.panelAlt = {0.095f, 0.100f, 0.105f, 1.0f};
    theme.panelHeader = {0.115f, 0.120f, 0.126f, 1.0f};
    theme.panelBorder = {0.190f, 0.205f, 0.215f, 1.0f};
    theme.panelHighlight = {0.135f, 0.145f, 0.150f, 1.0f};

    theme.textPrimary = {0.820f, 0.840f, 0.830f, 1.0f};
    theme.textSecondary = {0.620f, 0.660f, 0.660f, 1.0f};
    theme.textMuted = {0.400f, 0.440f, 0.440f, 1.0f};

    theme.accentCyan = {0.100f, 0.720f, 0.740f, 1.0f};
    theme.accentAmber = {0.950f, 0.560f, 0.180f, 1.0f};
    theme.accentRed = {0.850f, 0.220f, 0.170f, 1.0f};
    theme.accentGreen = {0.330f, 0.720f, 0.360f, 1.0f};

    theme.nodeBody = {0.105f, 0.110f, 0.115f, 1.0f};
    theme.nodeHeader = {0.135f, 0.140f, 0.145f, 1.0f};
    theme.nodeBorder = {0.260f, 0.280f, 0.285f, 1.0f};
    theme.nodeSelected = {0.100f, 0.650f, 0.680f, 1.0f};

    theme.wireNormal = {0.080f, 0.650f, 0.660f, 1.0f};
    theme.wireActive = {0.150f, 0.850f, 0.850f, 1.0f};
    theme.wireWarning = {0.950f, 0.560f, 0.180f, 1.0f};

    theme.gridMinor = {0.065f, 0.075f, 0.080f, 1.0f};
    theme.gridMajor = {0.090f, 0.105f, 0.110f, 1.0f};

    theme.panelPadding = 12.0f;
    theme.panelGap = 8.0f;
    theme.borderThickness = 1.0f;
    theme.gridSpacing = 24.0f;

    return theme;
}
