#include "app/DashboardLayout.h"

#include <algorithm>

DashboardRegions ComputeDashboardLayout(
    int width,
    int height,
    const DashboardLayoutMetrics& metrics
)
{
    const float w = static_cast<float>(std::max(width, 1));
    const float h = static_cast<float>(std::max(height, 1));

    const float gap = metrics.gap;
    const float topH = metrics.topBarHeight;
    const float inspectorH = metrics.inspectorHeight;
    const float eventLogH = metrics.eventLogHeight;
    const float leftW = metrics.leftPanelWidth;
    const float rightW = metrics.rightPanelWidth;

    const float middleY = topH + gap;
    const float middleH = std::max(
        metrics.minMiddleHeight,
        h - topH - inspectorH - eventLogH - gap * 4.0f
    );

    DashboardRegions regions;
    regions.topBar = {0.0f, 0.0f, w, topH};
    regions.leftPanel = {gap, middleY, leftW, middleH};
    regions.rightPanel = {w - rightW - gap, middleY, rightW, middleH};
    regions.graphCanvas = {
        leftW + gap * 2.0f,
        middleY,
        std::max(1.0f, w - leftW - rightW - gap * 4.0f),
        middleH
    };
    regions.inspector = {
        gap,
        middleY + middleH + gap,
        std::max(1.0f, w - gap * 2.0f),
        inspectorH
    };
    regions.eventLog = {
        gap,
        h - eventLogH - gap,
        std::max(1.0f, w - gap * 2.0f),
        eventLogH
    };

    return regions;
}

Rect InsetRect(Rect rect, float amount)
{
    return {
        rect.x + amount,
        rect.y + amount,
        std::max(0.0f, rect.w - amount * 2.0f),
        std::max(0.0f, rect.h - amount * 2.0f)
    };
}

Rect TopSlice(Rect rect, float height)
{
    return {
        rect.x,
        rect.y,
        rect.w,
        std::min(rect.h, height)
    };
}
