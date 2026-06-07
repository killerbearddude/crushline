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
    struct AddNodeMenuEntry
    {
        std::string label;
        std::string searchText;
        graph::MachineId machineId = graph::InvalidMachineId;
        graph::RecipeId recipeId = graph::InvalidRecipeId;
    };

    void AddEvent(std::string message);
    void MarkGraphDirty();
    void EvaluateGraphIfDirty();
    void UpdateEventLogFromProduction();

    // Rebuilds the Add Node menu from the current Tier 0 catalogs. The menu is
    // catalog-backed so node creation uses the same recipe IDs as the evaluator.
    void RebuildAddNodeMenuEntries();

    // Opens the Add Node menu at a canvas-space position where the chosen node
    // should be created.
    void OpenAddNodeMenu(Vec2 canvasPosition);

    // Closes the Add Node menu and disables SDL text input.
    void CloseAddNodeMenu();

    // Returns Add Node entries matching the current search text. The returned
    // indices point into m_addNodeMenuEntries and are valid until that vector is
    // rebuilt.
    [[nodiscard]] std::vector<std::size_t> FilteredAddNodeEntryIndices() const;

    // Handles text filtering, keyboard selection, and recipe-node creation while
    // the Add Node menu owns input for the frame.
    void UpdateAddNodeMenuInput(std::vector<std::string>& dashboardEvents);

    // Draws the Add Node overlay over the graph canvas.
    void DrawAddNodeMenu(Rect graphCanvas);

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

    std::vector<AddNodeMenuEntry> m_addNodeMenuEntries;
    std::string m_addNodeSearchText;
    Vec2 m_addNodeMenuCanvasPosition{};
    int m_addNodeSelectedIndex = 0;
    bool m_addNodeMenuOpen = false;

    // Prevents the key used to open the menu from also being consumed as the
    // first search/navigation input on the same frame.
    bool m_addNodeMenuSuppressInputThisFrame = false;

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
