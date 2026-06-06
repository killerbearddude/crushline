#pragma once

#include "app/DashboardLayout.h"
#include "editor/GraphView.h"
#include "graph/GraphEvaluator.h"
#include "graph/GraphTypes.h"
#include "platform/Input.h"
#include "platform/Window.h"
#include "renderer/Renderer2D.h"
#include "ui/Theme.h"

#include <cstddef>
#include <string>
#include <vector>

struct AppConfig
{
    const char* title = "Crushline";
    int width = 1536;
    int height = 864;

    // A value <= 0 means run until the user closes the window.
    int maxFrames = 0;

    // A value <= 0 disables periodic frame logging.
    int logEveryNFrames = 60;
};

class App
{
public:
    bool Initialize(const AppConfig& config = {});
    void RunFrame();
    void Shutdown();

    bool ShouldClose() const;

private:
    void AddEvent(std::string message);
    void MarkGraphDirty();
    void EvaluateGraphIfDirty();
    void UpdateEventLogFromSimulation();

    AppConfig m_config;
    DashboardLayoutMetrics m_layoutMetrics;
    UiTheme m_theme = MakeIndustrialDarkTheme();
    graph::GraphDocument m_graph;
    editor::GraphViewState m_graphView;
    graph::SimulationResult m_simulationResult;
    std::vector<std::string> m_eventLog;

    bool m_graphDirty = true;

    int m_lastSelectedEdgeHintId = -1;

    bool m_eventLogPrimed = false;
    std::size_t m_lastNodeCount = 0;
    std::size_t m_lastEdgeCount = 0;
    int m_lastWarningCount = 0;
    int m_lastBottleneckCount = 0;

    Window m_window;
    InputState m_input;
    Renderer2D m_renderer;

    bool m_shouldClose = false;

    int m_frameIndex = 0;

    double m_lastTimeSeconds = 0.0;
    float m_deltaTimeSeconds = 0.0f;
};
