# UI Model

The UI is retained-mode.

Rules:
- Every widget has bounds.
- Hit testing uses screen-space rectangles.
- UI state tracks hovered, active, and focused widget IDs.
- Layout, input, and drawing remain separate.
