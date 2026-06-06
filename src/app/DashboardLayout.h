#pragma once

#include "core/Rect.h"

struct DashboardLayoutMetrics
{
    float topBarHeight = 56.0f;
    float leftPanelWidth = 220.0f;
    float rightPanelWidth = 220.0f;
    float inspectorHeight = 148.0f;
    float eventLogHeight = 44.0f;
    float gap = 8.0f;
    float minMiddleHeight = 64.0f;
};

struct DashboardRegions
{
    Rect topBar{};
    Rect leftPanel{};
    Rect graphCanvas{};
    Rect rightPanel{};
    Rect inspector{};
    Rect eventLog{};
};

DashboardRegions ComputeDashboardLayout(
    int width,
    int height,
    const DashboardLayoutMetrics& metrics = {}
);

Rect InsetRect(Rect rect, float amount);
Rect TopSlice(Rect rect, float height);
