"""
Floorplan Editor - Level Editor Package

This package provides a tkinter-based level editor for creating and editing
floorplan layouts in JSON format.

Modules:
- object_types: Object type definitions and properties
- geometry: Geometry and transformation utilities  
- canvas_helper: Canvas drawing helpers
- file_io: File loading/saving operations
- editor_state: Editor state management
- object_renderer: Object rendering to canvas
- properties_panel: Properties panel UI component
- object_factory: Element creation factory functions
"""

from .object_types import (
    get_object_type,
    get_type_names,
    MODE_TO_TYPE,
    LINE_MODES,
    RECT_MODES,
    POINT_MODES,
    get_mode_color,
)

from .editor_state import EditorState
from .properties_panel import PropertiesPanel
from .object_factory import (
    create_wall,
    create_door,
    create_window,
    create_elevator,
    create_floor_ceiling_pair,
    create_ceiling_light,
    create_cubicle,
    create_spawn_point,
    create_room_tone,
)

__all__ = [
    'get_object_type',
    'get_type_names',
    'MODE_TO_TYPE',
    'LINE_MODES',
    'RECT_MODES',
    'POINT_MODES',
    'get_mode_color',
    'EditorState',
    'PropertiesPanel',
]
