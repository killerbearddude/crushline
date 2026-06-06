#pragma once

#include "editor/GraphView.h"
#include "graph/GraphTypes.h"

#include <string>

namespace graph
{

bool SaveGraphToFile(
    const GraphDocument& graph,
    const editor::GraphViewState& view,
    const std::string& path,
    std::string* errorMessage = nullptr
);

bool LoadGraphFromFile(
    GraphDocument& graph,
    editor::GraphViewState& view,
    const std::string& path,
    std::string* errorMessage = nullptr
);

} // namespace graph
