# Rendering Model

The renderer owns all graphics backend details.

Rules:
- UI and graph code submit draw commands.
- UI and graph code do not call OpenGL directly.
- Coordinate origin is top-left.
- Units are pixels.
- Renderer screen space is window pixel space.
- Graph canvas has its own world/canvas coordinate space.
