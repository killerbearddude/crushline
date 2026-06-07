#pragma once

// Declares the top-level application shell. App owns platform/window state,
// renderer state, graph editing state, production catalogs, evaluator outputs,
// and the dashboard/event-log bridge between gameplay data and UI rendering.

#include "app/DashboardLayout.h"
#include "editor/GraphView.h"
#include "graph/GraphEvaluator.h"
#include "graph/GraphTypes.h"
#include "graph/MachineCatalog.h"
#include "graph/ProductionEvaluator.h"
#include "graph/RecipeCatalog.h"
#include "graph/ResourceCatalog.h"
#include "graph/ScenarioCatalog.h"
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
    void UpdateEventLogFromProduction();

    AppConfig m_config;
    DashboardLayoutMetrics m_layoutMetrics;
    UiTheme m_theme = MakeIndustrialDarkTheme();
    graph::GraphDocument m_graph;
    editor::GraphViewState m_graphView;
    graph::SimulationResult m_simulationResult;
    graph::ResourceCatalog m_resourceCatalog;
    graph::MachineCatalog m_machineCatalog;
    graph::RecipeCatalog m_recipeCatalog;
    graph::ScenarioCatalog m_scenarioCatalog;
    graph::ProductionEvaluation m_productionEvaluation;
    std::vector<std::string> m_eventLog;

    // Legacy and production evaluation have separate dirty flags so the new
    // production solver is only recomputed after graph-data mutations. View-only
    // edits such as pan, zoom, selection, and node dragging do not change recipe
    // rates and should not invalidate production results.
    bool m_legacyEvaluationDirty = true;
    bool m_productionEvaluationDirty = true;

    int m_lastSelectedEdgeHintId = -1;

    bool m_eventLogPrimed = false;
    std::size_t m_lastNodeCount = 0;
    std::size_t m_lastEdgeCount = 0;
    int m_lastProductionInvalidConnectionCount = 0;
    int m_lastProductionBottleneckCount = 0;
    bool m_lastScenarioComplete = false;
    float m_lastTargetSatisfactionRatio = -1.0f;

    Window m_window;
    InputState m_input;
    Renderer2D m_renderer;

    bool m_shouldClose = false;

    int m_frameIndex = 0;

    double m_lastTimeSeconds = 0.0;
    float m_deltaTimeSeconds = 0.0f;
};
