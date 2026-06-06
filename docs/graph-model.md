# Graph Model

The graph document owns semantic data.

Rules:
- GraphDocument owns nodes, ports, and edges.
- GraphViewState owns positions, camera, zoom, and selection.
- Renderer does not know simulation rules.
- Evaluator reads GraphDocument and produces SimulationResult.
