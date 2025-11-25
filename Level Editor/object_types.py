"""
Object type definitions for the Floorplan Editor.

Each object type defines:
- How to draw itself on canvas
- What properties it has
- How to get its geometry (bounds, center, anchors)
- How to handle rotation and transformations
"""

import math
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple, Any, Callable

# Type aliases
Point = Tuple[float, float]
BBox = Tuple[float, float, float, float]  # x0, y0, x1, y1


@dataclass
class AnchorDef:
    """Definition of an anchor point for an object."""
    kind: str  # "corner", "endpoint", "center"
    color: str
    size: int = 5
    meta: Optional[Dict[str, Any]] = None


@dataclass
class PropertyDef:
    """Definition of an editable property."""
    key: str
    label: str
    prop_type: str  # "float", "int", "string", "bool", "section", "yaw"
    parent_key: Optional[str] = None  # For nested properties like Start.X
    minimum: float = -10000.0
    maximum: float = 10000.0
    step: float = 10.0
    precision: int = 2
    keys: Optional[List[str]] = None  # For section type


class ObjectType:
    """Base class for object types."""
    
    type_name: str = "Unknown"
    
    # Colors for different visual elements
    colors: Dict[str, str] = {}
    
    # Default values for new objects
    defaults: Dict[str, Any] = {}
    
    @classmethod
    def get_start_end(cls, item: Dict) -> Tuple[Optional[Point], Optional[Point]]:
        """Get start and end points from item data."""
        start = None
        end = None
        
        if "Start" in item:
            s = item["Start"]
            start = (float(s.get("X", 0.0)), float(s.get("Y", 0.0)))
        
        if "End" in item:
            e = item["End"]
            end = (float(e.get("X", 0.0)), float(e.get("Y", 0.0)))
        
        return start, end
    
    @classmethod
    def get_bounds(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[BBox]:
        """Get bounding box in world coordinates."""
        start, end = cls.get_start_end(item)
        if start is None:
            return None
        
        if end is not None:
            x0 = min(start[0], end[0])
            y0 = min(start[1], end[1])
            x1 = max(start[0], end[0])
            y1 = max(start[1], end[1])
            return (x0, y0, x1, y1)
        
        # Point-based object
        return (start[0], start[1], start[0], start[1])
    
    @classmethod
    def get_center(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[Point]:
        """Get center point in world coordinates."""
        bounds = cls.get_bounds(item, display_dims)
        if bounds is None:
            return None
        x0, y0, x1, y1 = bounds
        return ((x0 + x1) / 2, (y0 + y1) / 2)
    
    @classmethod
    def get_id_label_position(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[Point]:
        """Get position for ID label. Override for line-based objects."""
        return cls.get_center(item, display_dims)
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        """Get list of editable properties for this type."""
        return []
    
    @classmethod
    def get_anchor_definitions(cls, item: Dict) -> List[Tuple[Point, AnchorDef]]:
        """Get anchor definitions for this item. Override per type."""
        return []
    
    @classmethod
    def rotate(cls, item: Dict, delta_deg: float, center: Optional[Point] = None):
        """Rotate the object by delta degrees around a center point."""
        pass  # Override per type
    
    @classmethod
    def can_rotate(cls) -> bool:
        """Whether this object type supports rotation."""
        return False
    
    @classmethod
    def create_default(cls, position: Point) -> Dict[str, Any]:
        """Create a new object with default values at the given position."""
        item: Dict[str, Any] = {"Type": cls.type_name}
        item.update(cls.defaults)
        if "Start" in cls.defaults or cls.type_name in ("SpawnPoint", "RoomTone", "Cubicle"):
            item["Start"] = {"X": float(position[0]), "Y": float(position[1])}
        return item


class LineBasedType(ObjectType):
    """Base class for line-based objects (Wall, Door, Window, Elevator)."""
    
    @classmethod
    def get_id_label_position(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[Point]:
        """Get position for ID label, offset perpendicular to the line."""
        start, end = cls.get_start_end(item)
        if start is None or end is None:
            return None
        
        x0, y0 = start
        x1, y1 = end
        cx = (x0 + x1) / 2
        cy = (y0 + y1) / 2
        
        dx = x1 - x0
        dy = y1 - y0
        length = math.hypot(dx, dy)
        
        if length > 1e-6:
            offset = 30  # World units
            perp_x = -dy / length * offset
            perp_y = dx / length * offset
            return (cx + perp_x, cy + perp_y)
        
        return (cx, cy)
    
    @classmethod
    def rotate(cls, item: Dict, delta_deg: float, center: Optional[Point] = None):
        """Rotate line endpoints around a center point."""
        start, end = cls.get_start_end(item)
        if start is None or end is None:
            return
        
        if center is None:
            center = ((start[0] + end[0]) / 2, (start[1] + end[1]) / 2)
        
        cos_a = math.cos(math.radians(delta_deg))
        sin_a = math.sin(math.radians(delta_deg))
        
        def rotate_point(p: Point) -> Point:
            rx = p[0] - center[0]
            ry = p[1] - center[1]
            nx = rx * cos_a - ry * sin_a + center[0]
            ny = rx * sin_a + ry * cos_a + center[1]
            return (nx, ny)
        
        new_start = rotate_point(start)
        new_end = rotate_point(end)
        
        item["Start"]["X"] = new_start[0]
        item["Start"]["Y"] = new_start[1]
        item["End"]["X"] = new_end[0]
        item["End"]["Y"] = new_end[1]
    
    @classmethod
    def can_rotate(cls) -> bool:
        return True
    
    @classmethod
    def get_anchor_definitions(cls, item: Dict) -> List[Tuple[Point, AnchorDef]]:
        start, end = cls.get_start_end(item)
        if start is None or end is None:
            return []
        
        center = ((start[0] + end[0]) / 2, (start[1] + end[1]) / 2)
        
        return [
            (start, AnchorDef("endpoint", "#ff8c00", meta={"kind": "endpoint", "endpoint": "start"})),
            (end, AnchorDef("endpoint", "#1f78d1", meta={"kind": "endpoint", "endpoint": "end"})),
            (center, AnchorDef("center", "#777777", size=5, meta={"kind": "center", "center": center})),
        ]


class RectBasedType(ObjectType):
    """Base class for rectangle-based objects (Floor, Ceiling, CeilingLight)."""
    
    @classmethod
    def rotate(cls, item: Dict, delta_deg: float, center: Optional[Point] = None):
        """Rotate rectangle corners around a center point."""
        start, end = cls.get_start_end(item)
        if start is None or end is None:
            return
        
        if center is None:
            center = ((start[0] + end[0]) / 2, (start[1] + end[1]) / 2)
        
        cos_a = math.cos(math.radians(delta_deg))
        sin_a = math.sin(math.radians(delta_deg))
        
        def rotate_point(p: Point) -> Point:
            rx = p[0] - center[0]
            ry = p[1] - center[1]
            nx = rx * cos_a - ry * sin_a + center[0]
            ny = rx * sin_a + ry * cos_a + center[1]
            return (nx, ny)
        
        new_start = rotate_point(start)
        new_end = rotate_point(end)
        
        item["Start"]["X"] = new_start[0]
        item["Start"]["Y"] = new_start[1]
        item["End"]["X"] = new_end[0]
        item["End"]["Y"] = new_end[1]
    
    @classmethod
    def can_rotate(cls) -> bool:
        return True
    
    @classmethod
    def get_anchor_definitions(cls, item: Dict) -> List[Tuple[Point, AnchorDef]]:
        start, end = cls.get_start_end(item)
        if start is None or end is None:
            return []
        
        x0, y0 = start
        x1, y1 = end
        center = ((x0 + x1) / 2, (y0 + y1) / 2)
        
        corners = [
            ((x0, y0), 0),
            ((x1, y0), 1),
            ((x0, y1), 2),
            ((x1, y1), 3),
        ]
        
        anchors = []
        for (pos, idx) in corners:
            anchors.append((pos, AnchorDef("corner", "#999999", size=4, 
                                           meta={"kind": "corner", "corner": idx})))
        
        anchors.append((center, AnchorDef("center", "#777777", size=5,
                                          meta={"kind": "center", "center": center})))
        
        return anchors


class PointBasedType(ObjectType):
    """Base class for point-based objects (SpawnPoint, RoomTone, Cubicle)."""
    
    @classmethod
    def get_bounds(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[BBox]:
        start, _ = cls.get_start_end(item)
        if start is None:
            return None
        return (start[0], start[1], start[0], start[1])
    
    @classmethod
    def get_center(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[Point]:
        start, _ = cls.get_start_end(item)
        return start
    
    @classmethod
    def rotate(cls, item: Dict, delta_deg: float, center: Optional[Point] = None):
        """Rotate point around a center. Only moves position if center is different."""
        if "Yaw" in item or cls.type_name in ("SpawnPoint", "Cubicle"):
            current_yaw = float(item.get("Yaw", 0.0))
            item["Yaw"] = (current_yaw + delta_deg) % 360
        
        if center is not None:
            start, _ = cls.get_start_end(item)
            if start is not None:
                cos_a = math.cos(math.radians(delta_deg))
                sin_a = math.sin(math.radians(delta_deg))
                rx = start[0] - center[0]
                ry = start[1] - center[1]
                nx = rx * cos_a - ry * sin_a + center[0]
                ny = rx * sin_a + ry * cos_a + center[1]
                item["Start"]["X"] = nx
                item["Start"]["Y"] = ny
    
    @classmethod
    def can_rotate(cls) -> bool:
        return True


# ============================================================================
# Concrete object type implementations
# ============================================================================

class FloorType(RectBasedType):
    type_name = "Floor"
    colors = {"body": "#f9f9f9", "outline": "#cccccc"}
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("End", "End Position", "section", keys=["X", "Y"]),
        ]


class CeilingType(RectBasedType):
    type_name = "Ceiling"
    colors = {"outline": "#6a7aea"}
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("End", "End Position", "section", keys=["X", "Y"]),
        ]


class WallType(LineBasedType):
    type_name = "Wall"
    colors = {"line": "#222222"}
    defaults = {"Thickness": 25.0}
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("End", "End Position", "section", keys=["X", "Y"]),
            PropertyDef("Thickness", "Thickness", "float", step=5.0),
        ]


class DoorType(LineBasedType):
    type_name = "Door"
    colors = {"line": "#2b8a45", "frame": "#444444"}
    defaults = {"Thickness": 1.0}
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("End", "End Position", "section", keys=["X", "Y"]),
            PropertyDef("Thickness", "Thickness", "float", step=1.0),
        ]


class WindowType(LineBasedType):
    type_name = "Window"
    colors = {"line": "#1d6bd6"}
    defaults = {"Thickness": 1.0, "SectionCount": 1}
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("End", "End Position", "section", keys=["X", "Y"]),
            PropertyDef("Thickness", "Thickness", "float", step=1.0),
            PropertyDef("SectionCount", "Section Count", "int", minimum=1, maximum=100),
        ]


class ElevatorType(LineBasedType):
    type_name = "Elevator"
    colors = {"line": "#9b59b6", "cab": "#e8daef", "cab_outline": "#c39bd3"}
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("End", "End Position", "section", keys=["X", "Y"]),
        ]
    
    @classmethod
    def get_anchor_definitions(cls, item: Dict) -> List[Tuple[Point, AnchorDef]]:
        start, end = cls.get_start_end(item)
        if start is None or end is None:
            return []
        
        center = ((start[0] + end[0]) / 2, (start[1] + end[1]) / 2)
        
        return [
            (start, AnchorDef("endpoint", "#9b59b6", meta={"kind": "endpoint", "endpoint": "start"})),
            (end, AnchorDef("endpoint", "#9b59b6", meta={"kind": "endpoint", "endpoint": "end"})),
            (center, AnchorDef("center", "#777777", size=5, meta={"kind": "center", "center": center})),
        ]


class CeilingLightType(RectBasedType):
    type_name = "CeilingLight"
    colors = {"outline": "#ffa500"}
    defaults = {
        "Spacing": {"X": 500.0, "Y": 600.0},
        "Padding": {"X": 200.0, "Y": 200.0},
        "Yaw": 0.0,
    }
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("End", "End Position", "section", keys=["X", "Y"]),
            PropertyDef("Spacing", "Spacing", "section", keys=["X", "Y"]),
            PropertyDef("Padding", "Padding", "section", keys=["X", "Y"]),
            PropertyDef("Yaw", "Yaw", "yaw"),
        ]


class CubicleType(PointBasedType):
    type_name = "Cubicle"
    colors = {"body": "#dddddd", "outline": "black", "arrow": "#ff6600"}
    defaults = {"Yaw": 0.0}  # Cubicles use global width/depth, not per-item Dimensions
    
    @classmethod
    def get_bounds(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[BBox]:
        start, _ = cls.get_start_end(item)
        if start is None:
            return None
        
        # Use global display_dims (width, depth) - width is along back wall, depth is forward
        w = display_dims[0] if display_dims else 300.0
        h = display_dims[1] if display_dims else 250.0
        
        yaw = float(item.get("Yaw", 0.0))
        
        # Adjust for visual offset - depth extends forward from back wall
        vis_yaw = (yaw + 90.0) % 360
        steps = int(round(vis_yaw / 90.0)) % 4
        if steps % 2 != 0:
            w, h = h, w
        
        return (start[0], start[1], start[0] + h, start[1] + w)  # depth (h) is X, width (w) is Y
    
    @classmethod
    def get_center(cls, item: Dict, display_dims: Optional[Tuple[float, float]] = None) -> Optional[Point]:
        bounds = cls.get_bounds(item, display_dims)
        if bounds is None:
            return None
        x0, y0, x1, y1 = bounds
        return ((x0 + x1) / 2, (y0 + y1) / 2)
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        # Cubicles only have position and yaw - dimensions are global
        return [
            PropertyDef("Start", "Position", "section", keys=["X", "Y"]),
            PropertyDef("Yaw", "Yaw", "yaw"),
        ]
    
    @classmethod
    def get_anchor_definitions(cls, item: Dict) -> List[Tuple[Point, AnchorDef]]:
        # Anchors are created dynamically based on display dims
        return []  # Handled specially in draw code


class SpawnPointType(PointBasedType):
    type_name = "SpawnPoint"
    colors = {"body": "#00cc66", "direction": "#006633"}
    defaults = {"HeightOffset": 0.0, "Yaw": 0.0}
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("HeightOffset", "Height Offset", "float"),
            PropertyDef("Yaw", "Yaw", "yaw"),
        ]


class RoomToneType(PointBasedType):
    type_name = "RoomTone"
    colors = {"body": "#ff9900", "radius": "#ffcc66"}
    defaults = {
        "HeightOffset": 150.0,
        "AudioId": "Office",
        "bOmnidirectional": True,
        "AttenuationRadius": 1600.0,
        "VolumeMultiplier": 1.0,
    }
    
    @classmethod
    def get_properties(cls) -> List[PropertyDef]:
        return [
            PropertyDef("Start", "Start Position", "section", keys=["X", "Y"]),
            PropertyDef("HeightOffset", "Height Offset", "float"),
            PropertyDef("AudioId", "Audio ID", "string"),
            PropertyDef("bOmnidirectional", "Omnidirectional", "bool"),
            PropertyDef("AttenuationRadius", "Attenuation Radius", "float", step=50.0),
            PropertyDef("VolumeMultiplier", "Volume Multiplier", "float", 
                       minimum=0.0, maximum=10.0, step=0.1, precision=2),
        ]
    
    @classmethod
    def can_rotate(cls) -> bool:
        return False  # RoomTone doesn't have Yaw


# ============================================================================
# Type Registry
# ============================================================================

OBJECT_TYPES: Dict[str, type] = {
    "Floor": FloorType,
    "Ceiling": CeilingType,
    "Wall": WallType,
    "Door": DoorType,
    "Window": WindowType,
    "Elevator": ElevatorType,
    "CeilingLight": CeilingLightType,
    "Cubicle": CubicleType,
    "SpawnPoint": SpawnPointType,
    "RoomTone": RoomToneType,
}

# Alias for backward compatibility
TYPE_MAP = OBJECT_TYPES


def get_object_type(type_name: str) -> type:
    """Get the ObjectType class for a given type name."""
    return OBJECT_TYPES.get(type_name, ObjectType)


def get_type_names() -> List[str]:
    """Get list of all registered type names."""
    return list(OBJECT_TYPES.keys())


# Mode to type mapping for creating new objects
MODE_TO_TYPE: Dict[str, str] = {
    "add_wall": "Wall",
    "add_door": "Door",
    "add_window": "Window",
    "add_elevator": "Elevator",
    "add_cubicle": "Cubicle",
    "add_spawn": "SpawnPoint",
    "add_roomtone": "RoomTone",
    "add_floor_ceiling": "Floor",  # Creates Floor + Ceiling pair
    "add_ceiling_light": "CeilingLight",
}

# Modes that use line-based input (click-drag for start/end)
LINE_MODES = {"add_wall", "add_door", "add_window", "add_elevator"}

# Modes that use rectangle-based input (click-drag for corners)
RECT_MODES = {"add_floor_ceiling", "add_ceiling_light"}

# Modes that place a point object on click
POINT_MODES = {"add_cubicle", "add_spawn", "add_roomtone"}

# Mode preview colors
MODE_COLORS: Dict[str, str] = {
    "add_wall": "#222222",
    "add_door": "#2b8a45",
    "add_window": "#1d6bd6",
    "add_elevator": "#9b59b6",
    "add_floor_ceiling": "#9966cc",
    "add_ceiling_light": "#ffa500",
}


def get_mode_color(mode: str) -> str:
    """Get the preview color for a mode."""
    return MODE_COLORS.get(mode, "#555555")
