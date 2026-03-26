# Layout

This module now contains the first production layout pass for the backend graph pipeline.

## Current capability

- Builds a layered layout over inferred module dependency edges.
- Detects cyclic module groups and pushes them into a fallback final layer.
- Produces stable `(x, y)` positions for module nodes.
- Formats a readable layout summary for the current desktop shell.

## Why it exists now

The UI layer should not be responsible for inventing graph coordinates. By the time the frontend starts rendering nodes and edges, it should already receive:

- graph nodes
- graph edges
- module dependency weights
- layout coordinates

That keeps visualization work focused on rendering and interaction instead of backend inference.
