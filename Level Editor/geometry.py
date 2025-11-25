"""
Geometry and transformation utilities for the Floorplan Editor.
"""

import math
from typing import Tuple, Optional, List, Dict, Any

Point = Tuple[float, float]
BBox = Tuple[float, float, float, float]


def normalize_yaw(yaw: float) -> float:
    """Normalize yaw to 0, 90, 180, or 270 degrees."""
    try:
        angle = float(yaw)
    except (TypeError, ValueError):
        angle = 0.0
    steps = int(round(angle / 90.0)) % 4
    return float(steps * 90)


def get_axis_size(dimensions: Dict[str, float], yaw: float) -> Tuple[float, float]:
    """Get axis-aligned size after rotation."""
    w = float(dimensions.get("X", 0.0))
    h = float(dimensions.get("Y", 0.0))
    steps = int(round(yaw / 90.0)) % 4
    if steps % 2 == 0:
        return abs(w), abs(h)
    return abs(h), abs(w)


def get_axis_vectors_for_yaw(yaw: float) -> Tuple[Point, Point]:
    """Return forward and right unit vectors for a yaw snapped to 90° increments."""
    normalized = normalize_yaw(yaw)
    steps = int(normalized / 90.0) % 4
    if steps == 0:
        forward = (0.0, 1.0)
        right = (1.0, 0.0)
    elif steps == 1:
        forward = (1.0, 0.0)
        right = (0.0, -1.0)
    elif steps == 2:
        forward = (0.0, -1.0)
        right = (-1.0, 0.0)
    else:
        forward = (-1.0, 0.0)
        right = (0.0, 1.0)
    return forward, right


def rotate_point(point: Point, center: Point, angle_deg: float) -> Point:
    """Rotate a point around a center by angle in degrees."""
    cos_a = math.cos(math.radians(angle_deg))
    sin_a = math.sin(math.radians(angle_deg))
    rx = point[0] - center[0]
    ry = point[1] - center[1]
    nx = rx * cos_a - ry * sin_a + center[0]
    ny = rx * sin_a + ry * cos_a + center[1]
    return (nx, ny)


def distance(p1: Point, p2: Point) -> float:
    """Calculate distance between two points."""
    return math.hypot(p2[0] - p1[0], p2[1] - p1[1])


def midpoint(p1: Point, p2: Point) -> Point:
    """Calculate midpoint between two points."""
    return ((p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2)


def perpendicular_offset(p1: Point, p2: Point, offset: float) -> Tuple[float, float]:
    """Get perpendicular offset direction from a line."""
    dx = p2[0] - p1[0]
    dy = p2[1] - p1[1]
    length = math.hypot(dx, dy)
    if length < 1e-6:
        return (0.0, 0.0)
    return (-dy / length * offset, dx / length * offset)


def snap_value(value: float, grid_size: float) -> float:
    """Snap a value to the nearest grid point."""
    if grid_size <= 0:
        return float(value)
    return round(float(value) / grid_size) * grid_size


def snap_point(point: Point, grid_size: float) -> Point:
    """Snap a point to the nearest grid intersection."""
    return (snap_value(point[0], grid_size), snap_value(point[1], grid_size))


def bbox_union(bboxes: List[BBox]) -> Optional[BBox]:
    """Calculate the union of multiple bounding boxes."""
    if not bboxes:
        return None
    
    min_x = min(b[0] for b in bboxes)
    min_y = min(b[1] for b in bboxes)
    max_x = max(b[2] for b in bboxes)
    max_y = max(b[3] for b in bboxes)
    
    return (min_x, min_y, max_x, max_y)


def bbox_center(bbox: BBox) -> Point:
    """Get the center of a bounding box."""
    return ((bbox[0] + bbox[2]) / 2, (bbox[1] + bbox[3]) / 2)


def bbox_contains_point(bbox: BBox, point: Point, tolerance: float = 0.0) -> bool:
    """Check if a point is inside a bounding box."""
    return (bbox[0] - tolerance <= point[0] <= bbox[2] + tolerance and
            bbox[1] - tolerance <= point[1] <= bbox[3] + tolerance)


def bboxes_overlap(a: BBox, b: BBox) -> bool:
    """Check if two bounding boxes overlap."""
    return not (a[2] < b[0] or a[0] > b[2] or a[3] < b[1] or a[1] > b[3])


def expand_bbox(bbox: BBox, padding: float) -> BBox:
    """Expand a bounding box by a padding amount."""
    return (bbox[0] - padding, bbox[1] - padding, 
            bbox[2] + padding, bbox[3] + padding)


def point_to_line_distance(point: Point, line_start: Point, line_end: Point) -> float:
    """Calculate the shortest distance from a point to a line segment."""
    dx = line_end[0] - line_start[0]
    dy = line_end[1] - line_start[1]
    length_sq = dx * dx + dy * dy
    
    if length_sq < 1e-10:
        # Line is actually a point
        return distance(point, line_start)
    
    # Project point onto line
    t = max(0, min(1, ((point[0] - line_start[0]) * dx + 
                        (point[1] - line_start[1]) * dy) / length_sq))
    
    proj_x = line_start[0] + t * dx
    proj_y = line_start[1] + t * dy
    
    return distance(point, (proj_x, proj_y))
