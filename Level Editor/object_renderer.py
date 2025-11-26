"""
Object rendering for the Floorplan Editor.

Provides drawing functions for each object type.
"""

import math
import tkinter as tk
from typing import Dict, List, Tuple, Any, Optional, Callable

try:
    from .canvas_helper import CanvasHelper, draw_direction_arrow
    from .geometry import (
        normalize_yaw,
        perpendicular_offset,
        get_axis_vectors_for_yaw,
    )
    from .object_types import get_object_type, LineBasedType
except ImportError:
    from canvas_helper import CanvasHelper, draw_direction_arrow
    from geometry import (
        normalize_yaw,
        perpendicular_offset,
        get_axis_vectors_for_yaw,
    )
    from object_types import get_object_type, LineBasedType


# Type aliases
Point = Tuple[float, float]

# Z-order layer tags (bottom to top)
# Objects are placed in these layers to ensure consistent visual stacking
Z_ORDER = [
    "floor_layer",      # Floor rectangles (bottommost)
    "ceiling_layer",    # Ceiling rectangles
    "cubicle_layer",    # Cubicles
    "light_layer",      # Ceiling lights
    "elevator_layer",   # Elevators
    "grid",             # Background grid
    "wall_layer",       # Walls
    "door_layer",       # Doors
    "window_layer",     # Windows
    "roomtone_layer",   # Room tones
    "spawn_layer",      # Spawn points
    "label_layer",      # ID labels and text
    "anchor_layer",     # Anchors (topmost, for interaction)
]


class ObjectRenderer:
    """Renders layout objects to canvas."""
    
    def __init__(
        self,
        helper: CanvasHelper,
        get_display_dims: Callable[[], Tuple[float, float]],
        show_lamps: Callable[[], bool],
    ):
        self.helper = helper
        self.get_display_dims = get_display_dims
        self.show_lamps = show_lamps
        # Anchor meta will be collected during draw and returned
        self._anchor_meta: Dict[int, Dict[str, Any]] = {}
    
    def draw_item(self, item: Dict) -> Optional[Dict]:
        """
        Draw an item and return an object dict with canvas IDs.
        
        Returns:
            Dict with keys: data, canvas_ids, anchors, parts, base_styles, anchor_meta
            Or None if drawing failed
        """
        item_type = item.get("Type", "")
        canvas_ids: List[int] = []
        anchors: List[int] = []
        parts: Dict[str, List[int]] = {}
        base_styles: Dict[int, Dict[str, str]] = {}
        self._anchor_meta = {}  # Reset for this item
        
        def register_part(cid: int, role: Optional[str] = None, capture: bool = True):
            """Register a canvas element."""
            if role is not None:
                parts.setdefault(role, []).append(cid)
            if capture:
                try:
                    base_styles[cid] = {
                        "fill": self.helper.canvas.itemcget(cid, "fill"),
                        "outline": self.helper.canvas.itemcget(cid, "outline"),
                    }
                except tk.TclError:
                    base_styles[cid] = {}
        
        # Dispatch to type-specific drawing
        if item_type == "Floor":
            self._draw_floor(item, canvas_ids, anchors, register_part)
        elif item_type == "Ceiling":
            self._draw_ceiling(item, canvas_ids, anchors, register_part)
        elif item_type in ("Wall", "Door", "Window"):
            self._draw_wall_door_window(item, canvas_ids, anchors, register_part)
        elif item_type == "Elevator":
            self._draw_elevator(item, canvas_ids, anchors, register_part)
        elif item_type == "CeilingLight":
            self._draw_ceiling_light(item, canvas_ids, anchors, register_part)
        elif item_type == "Cubicle":
            self._draw_cubicle(item, canvas_ids, anchors, register_part)
        elif item_type == "SpawnPoint":
            self._draw_spawn_point(item, canvas_ids, anchors, register_part)
        elif item_type == "RoomTone":
            self._draw_room_tone(item, canvas_ids, anchors, register_part)
        
        # Draw ID label if present
        self._draw_id_label(item, canvas_ids, register_part)
        
        if not canvas_ids:
            return None
        
        return {
            "data": item,
            "canvas_ids": canvas_ids,
            "anchors": anchors,
            "parts": parts,
            "base_styles": base_styles,
            "anchor_meta": dict(self._anchor_meta),
        }
    
    def _get_start_end(self, item: Dict) -> Tuple[Optional[Point], Optional[Point]]:
        """Get start and end points from item."""
        start = None
        end = None
        if "Start" in item:
            s = item["Start"]
            start = (float(s.get("X", 0.0)), float(s.get("Y", 0.0)))
        if "End" in item:
            e = item["End"]
            end = (float(e.get("X", 0.0)), float(e.get("Y", 0.0)))
        return start, end
    
    def _place_in_layer(self, cid: int, layer: str):
        """Place a canvas item in the appropriate z-order layer.
        
        Strategy: Try to lower this item below items in higher layers.
        This ensures proper ordering regardless of creation order.
        """
        # Add tag for the layer
        self.helper.canvas.addtag_withtag(layer, cid)
        
        # Find the layer index
        try:
            layer_idx = Z_ORDER.index(layer)
        except ValueError:
            return  # Unknown layer, leave as is
        
        # Try to lower below layers above this one (from lowest higher layer to highest)
        for i in range(layer_idx + 1, len(Z_ORDER)):
            try:
                self.helper.canvas.tag_lower(cid, Z_ORDER[i])
                return  # Successfully placed below a higher layer
            except tk.TclError:
                continue  # That layer doesn't have any items yet
        
        # No higher layers exist yet, try to raise above lower layers
        for i in range(layer_idx - 1, -1, -1):
            try:
                self.helper.canvas.tag_raise(cid, Z_ORDER[i])
                return  # Successfully placed above a lower layer
            except tk.TclError:
                continue  # That layer doesn't have any items yet
    
    def _create_anchor(
        self,
        wx: float,
        wy: float,
        color: str,
        size: int,
        meta: Dict[str, Any],
        anchors: List[int],
        canvas_ids: List[int],
    ):
        """Create an anchor marker and register it."""
        anchor_id = self.helper.create_anchor_marker(wx, wy, color=color, size=size, meta=meta)
        anchors.append(anchor_id)
        canvas_ids.append(anchor_id)
        # Store meta for later lookup by floorplan editor
        self._anchor_meta[anchor_id] = dict(meta)
        # Place anchors in anchor layer (topmost)
        self._place_in_layer(anchor_id, "anchor_layer")
        return anchor_id
    
    def _draw_floor(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw a Floor element."""
        start, end = self._get_start_end(item)
        if start is None or end is None:
            return
        
        x0, y0 = start
        x1, y1 = end
        
        sx0, sy0 = self.helper.world_to_screen(x0, y0)
        sx1, sy1 = self.helper.world_to_screen(x1, y1)
        
        cid = self.helper.canvas.create_rectangle(
            sx0, sy0, sx1, sy1,
            outline="#cccccc", fill="#f9f9f9",
            tags=("floor_layer",)
        )
        canvas_ids.append(cid)
        register_part(cid, "body")
        self._place_in_layer(cid, "floor_layer")
        
        # Corner anchors
        corners = [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]
        for idx, (wx, wy) in enumerate(corners):
            self._create_anchor(wx, wy, "#999999", 4,
                               {"kind": "corner", "corner": idx},
                               anchors, canvas_ids)
        
        # Center anchor
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        self._create_anchor(cx, cy, "#777777", 5,
                           {"kind": "center", "center": (cx, cy)},
                           anchors, canvas_ids)
    
    def _draw_ceiling(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw a Ceiling element."""
        start, end = self._get_start_end(item)
        if start is None or end is None:
            return
        
        x0, y0 = start
        x1, y1 = end
        
        sx0, sy0 = self.helper.world_to_screen(x0, y0)
        sx1, sy1 = self.helper.world_to_screen(x1, y1)
        
        cid = self.helper.canvas.create_rectangle(
            sx0, sy0, sx1, sy1,
            outline="#6a7aea", dash=(4, 3), fill="#e8eafc", width=2,
            stipple="gray25",  # Light transparency effect
            tags=("ceiling_layer",)
        )
        canvas_ids.append(cid)
        register_part(cid, "outline")
        self._place_in_layer(cid, "ceiling_layer")
        
        # Corner anchors
        corners = [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]
        for idx, (wx, wy) in enumerate(corners):
            self._create_anchor(wx, wy, "#999999", 4,
                               {"kind": "corner", "corner": idx},
                               anchors, canvas_ids)
        
        # Center anchor
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        self._create_anchor(cx, cy, "#777777", 5,
                           {"kind": "center", "center": (cx, cy)},
                           anchors, canvas_ids)
    
    def _draw_wall_door_window(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw Wall, Door, or Window elements."""
        item_type = item.get("Type", "Wall")
        start, end = self._get_start_end(item)
        if start is None or end is None:
            return
        
        sx_world, sy_world = start
        ex_world, ey_world = end
        
        x0, y0 = self.helper.world_to_screen(sx_world, sy_world)
        x1, y1 = self.helper.world_to_screen(ex_world, ey_world)
        
        thickness = float(item.get("Thickness", 1.0))
        width = max(1, thickness / 12.0)
        
        colors = {
            "Wall": "#222222",
            "Door": "#2b8a45",
            "Window": "#1d6bd6",
        }
        color = colors.get(item_type, "#222222")
        
        # Main line
        main_line = self.helper.canvas.create_line(
            x0, y0, x1, y1,
            fill="#444444" if item_type == "Door" else color,
            width=width, capstyle=tk.ROUND
        )
        canvas_ids.append(main_line)
        register_part(main_line, "line")
        
        # Window sections
        if item_type == "Window":
            section_count = int(item.get("SectionCount", 1))
            if section_count > 1:
                dx, dy = x1 - x0, y1 - y0
                length = math.hypot(dx, dy)
                if length > 1e-6:
                    for i in range(1, section_count):
                        ratio = i / section_count
                        sect_x = x0 + dx * ratio
                        sect_y = y0 + dy * ratio
                        perp_x = -dy / length * 8
                        perp_y = dx / length * 8
                        sect_line = self.helper.canvas.create_line(
                            sect_x - perp_x, sect_y - perp_y,
                            sect_x + perp_x, sect_y + perp_y,
                            fill=color, width=max(1, width)
                        )
                        canvas_ids.append(sect_line)
                        register_part(sect_line, "line")
        
        # Door decoration
        if item_type == "Door":
            dx, dy = x1 - x0, y1 - y0
            length = math.hypot(dx, dy)
            if length > 1e-6:
                self.helper.canvas.itemconfigure(main_line, dash=(8, 4))
                mx, my = (x0 + x1) / 2, (y0 + y1) / 2
                ux, uy = dx / length, dy / length
                door_length = min(length * 0.6, 140)
                door_half = door_length / 2
                
                door_line = self.helper.canvas.create_line(
                    mx - ux * door_half, my - uy * door_half,
                    mx + ux * door_half, my + uy * door_half,
                    fill=color, width=max(width, width + 1), capstyle=tk.ROUND
                )
                canvas_ids.append(door_line)
                register_part(door_line, "line")
                
                tick_length = min(door_length * 0.4, 35)
                tick_x = mx - uy * tick_length
                tick_y = my + ux * tick_length
                tick_line = self.helper.canvas.create_line(
                    mx, my, tick_x, tick_y,
                    fill=color, width=max(1, width - 1), capstyle=tk.ROUND
                )
                canvas_ids.append(tick_line)
                register_part(tick_line, "line")
        
        # Anchors
        self._create_anchor(sx_world, sy_world, "#ff8c00", 5,
                           {"kind": "endpoint", "endpoint": "start"},
                           anchors, canvas_ids)
        self._create_anchor(ex_world, ey_world, "#1f78d1", 5,
                           {"kind": "endpoint", "endpoint": "end"},
                           anchors, canvas_ids)
        
        cx, cy = (sx_world + ex_world) / 2, (sy_world + ey_world) / 2
        self._create_anchor(cx, cy, "#777777", 5,
                           {"kind": "center", "center": (cx, cy)},
                           anchors, canvas_ids)
        
        # Place all non-anchor elements in type-specific layer
        layer_map = {"Wall": "wall_layer", "Door": "door_layer", "Window": "window_layer"}
        layer = layer_map.get(item_type, "wall_layer")
        for cid in canvas_ids:
            if cid not in anchors:
                self._place_in_layer(cid, layer)
    
    def _draw_elevator(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw an Elevator element."""
        start, end = self._get_start_end(item)
        if start is None or end is None:
            return
        
        sx_world, sy_world = start
        ex_world, ey_world = end
        
        x0, y0 = self.helper.world_to_screen(sx_world, sy_world)
        x1, y1 = self.helper.world_to_screen(ex_world, ey_world)
        
        wall_color = "#9b59b6"
        cab_color = "#e8daef"
        
        dx, dy = x1 - x0, y1 - y0
        length = math.hypot(dx, dy)
        
        # Draw cab rectangle first (so it's behind the wall line)
        cab_rect = None
        if length > 1e-6:
            perp_x = -dy / length
            perp_y = dx / length
            
            cab_depth_world = 250.0
            scale = self.helper.world_to_screen_scale()
            cab_depth_screen = cab_depth_world * scale
            
            cab_corners = [
                x0, y0,
                x1, y1,
                x1 + perp_x * cab_depth_screen, y1 + perp_y * cab_depth_screen,
                x0 + perp_x * cab_depth_screen, y0 + perp_y * cab_depth_screen,
            ]
            
            cab_rect = self.helper.canvas.create_polygon(
                cab_corners, fill=cab_color, outline="#c39bd3", width=1
            )
            canvas_ids.append(cab_rect)
            register_part(cab_rect, "cab")
        
        wall_line = self.helper.canvas.create_line(
            x0, y0, x1, y1, fill=wall_color, width=4, capstyle=tk.PROJECTING
        )
        canvas_ids.append(wall_line)
        register_part(wall_line, "line")
        
        if length > 1e-6:
            mx, my = (x0 + x1) / 2, (y0 + y1) / 2
            tick_len = 8
            perp_nx = -dy / length
            perp_ny = dx / length
            tick_line = self.helper.canvas.create_line(
                mx - perp_nx * tick_len, my - perp_ny * tick_len,
                mx + perp_nx * tick_len, my + perp_ny * tick_len,
                fill=wall_color, width=2
            )
            canvas_ids.append(tick_line)
            register_part(tick_line, "line")
        
        # Anchors
        self._create_anchor(sx_world, sy_world, "#9b59b6", 5,
                           {"kind": "endpoint", "endpoint": "start"},
                           anchors, canvas_ids)
        self._create_anchor(ex_world, ey_world, "#9b59b6", 5,
                           {"kind": "endpoint", "endpoint": "end"},
                           anchors, canvas_ids)
        
        cx, cy = (sx_world + ex_world) / 2, (sy_world + ey_world) / 2
        self._create_anchor(cx, cy, "#777777", 5,
                           {"kind": "center", "center": (cx, cy)},
                           anchors, canvas_ids)
        
        # Place all non-anchor elements in elevator layer
        for cid in canvas_ids:
            if cid not in anchors:
                self._place_in_layer(cid, "elevator_layer")
    
    def _draw_ceiling_light(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw a CeilingLight element."""
        start, end = self._get_start_end(item)
        if start is None or end is None:
            return
        
        x0, y0 = start
        x1, y1 = end
        
        sx0, sy0 = self.helper.world_to_screen(x0, y0)
        sx1, sy1 = self.helper.world_to_screen(x1, y1)
        
        cid = self.helper.canvas.create_rectangle(
            sx0, sy0, sx1, sy1,
            outline="#ffa500", dash=(4, 4), fill="", width=1
        )
        canvas_ids.append(cid)
        register_part(cid, "frame")
        
        # Draw lamp positions if enabled
        if self.show_lamps():
            spacing = item.get("Spacing", {})
            padding = item.get("Padding", {})
            yaw = float(item.get("Yaw", 0.0))
            
            space_along = float(spacing.get("X", 300.0))
            space_between = float(spacing.get("Y", 300.0))
            pad_x = float(padding.get("X", 0.0))
            pad_y = float(padding.get("Y", 0.0))
            
            # Calculate padded bounds
            min_x, max_x = min(x0, x1), max(x0, x1)
            min_y, max_y = min(y0, y1), max(y0, y1)
            padded_min_x = min_x + pad_x
            padded_max_x = max_x - pad_x
            padded_min_y = min_y + pad_y
            padded_max_y = max_y - pad_y
            
            if padded_max_x > padded_min_x and padded_max_y > padded_min_y:
                center_x = (padded_min_x + padded_max_x) / 2
                center_y = (padded_min_y + padded_max_y) / 2
                
                # Calculate line direction from yaw
                rad = math.radians(yaw)
                line_dx = math.cos(rad)
                line_dy = math.sin(rad)
                perp_dx = -line_dy
                perp_dy = line_dx
                
                # Calculate number of lines
                cross_extent = max(padded_max_x - padded_min_x, padded_max_y - padded_min_y) / 2
                if space_between > 1e-6:
                    n_lines = int(cross_extent * 2 / space_between) + 1
                else:
                    n_lines = 1
                
                half_lines = (n_lines - 1) / 2
                
                for line_idx in range(n_lines):
                    offset = (line_idx - half_lines) * space_between
                    line_center_x = center_x + perp_dx * offset
                    line_center_y = center_y + perp_dy * offset
                    
                    # Calculate lights along this line
                    line_extent = cross_extent
                    if space_along > 1e-6:
                        n_lights = int(line_extent * 2 / space_along) + 1
                    else:
                        n_lights = 1
                    
                    half_lights = (n_lights - 1) / 2
                    
                    for light_idx in range(n_lights):
                        light_offset = (light_idx - half_lights) * space_along
                        light_x = line_center_x + line_dx * light_offset
                        light_y = line_center_y + line_dy * light_offset
                        
                        # Check bounds
                        if (padded_min_x <= light_x <= padded_max_x and
                            padded_min_y <= light_y <= padded_max_y):
                            lsx, lsy = self.helper.world_to_screen(light_x, light_y)
                            lamp = self.helper.canvas.create_oval(
                                lsx - 5, lsy - 5, lsx + 5, lsy + 5,
                                fill="#fffacd", outline="#ffa500", width=1
                            )
                            canvas_ids.append(lamp)
                            register_part(lamp, "lamp")
        
        # Corner anchors
        corners = [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]
        for idx, (wx, wy) in enumerate(corners):
            self._create_anchor(wx, wy, "#ffa500", 4,
                               {"kind": "corner", "corner": idx},
                               anchors, canvas_ids)
        
        # Center anchor
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        self._create_anchor(cx, cy, "#777777", 5,
                           {"kind": "center", "center": (cx, cy)},
                           anchors, canvas_ids)
        
        # Place all non-anchor elements in light layer
        for cid in canvas_ids:
            if cid not in anchors:
                self._place_in_layer(cid, "light_layer")
    
    def _draw_cubicle(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw a Cubicle element."""
        start, _ = self._get_start_end(item)
        if start is None:
            return
        
        display_width, display_depth = self.get_display_dims()
        yaw = normalize_yaw(float(item.get("Yaw", 0.0)))
        item["Yaw"] = yaw

        forward, right = get_axis_vectors_for_yaw(yaw)
        half_width = display_width / 2.0

        back_center = start
        back_left = (
            back_center[0] - right[0] * half_width,
            back_center[1] - right[1] * half_width,
        )
        back_right = (
            back_center[0] + right[0] * half_width,
            back_center[1] + right[1] * half_width,
        )
        front_left = (
            back_left[0] + forward[0] * display_depth,
            back_left[1] + forward[1] * display_depth,
        )
        front_right = (
            back_right[0] + forward[0] * display_depth,
            back_right[1] + forward[1] * display_depth,
        )

        world_corners = [back_left, back_right, front_right, front_left]
        screen_corners = [self.helper.world_to_screen(px, py) for px, py in world_corners]
        sx_values = [sx for sx, _ in screen_corners]
        sy_values = [sy for _, sy in screen_corners]

        sx0, sx1 = min(sx_values), max(sx_values)
        sy0, sy1 = min(sy_values), max(sy_values)

        cid = self.helper.canvas.create_rectangle(
            sx0, sy0, sx1, sy1, outline="black", fill="#dddddd"
        )
        canvas_ids.append(cid)
        register_part(cid, "body")
        
        # Direction arrow (points in the direction the desk faces)
        indicator_size = min(abs(sx1 - sx0), abs(sy1 - sy0)) * 0.12
        cx, cy = (sx0 + sx1) / 2, (sy0 + sy1) / 2
        vis_yaw = normalize_yaw(yaw + 90.0)
        angle_rad = math.radians(-vis_yaw)
        arrow_len = indicator_size * 2
        end_x = cx + arrow_len * math.cos(angle_rad)
        end_y = cy + arrow_len * math.sin(angle_rad)
        
        arrow_id = self.helper.canvas.create_line(
            cx, cy, end_x, end_y,
            fill="#ff6600", width=3, arrow=tk.LAST, arrowshape=(10, 12, 5)
        )
        canvas_ids.append(arrow_id)
        register_part(arrow_id, "arrow")
        
        # Place cubicle body elements in cubicle layer
        for c in [cid, arrow_id]:
            self._place_in_layer(c, "cubicle_layer")
        
        # Corner anchors (axis-aligned bounds suffice for 90° increments)
        xs = [corner[0] for corner in world_corners]
        ys = [corner[1] for corner in world_corners]
        x0, x1 = min(xs), max(xs)
        y0, y1 = min(ys), max(ys)
        corners = [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]
        for idx, (wx, wy) in enumerate(corners):
            color = "#ff8c00" if idx == 0 else ("#1f78d1" if idx == 3 else "#666666")
            self._create_anchor(wx, wy, color, 5,
                               {"kind": "corner", "corner": idx},
                               anchors, canvas_ids)
        
        # Center anchor
        center_x, center_y = (x0 + x1) / 2, (y0 + y1) / 2
        self._create_anchor(center_x, center_y, "#777777", 5,
                           {"kind": "center", "center": (center_x, center_y)},
                           anchors, canvas_ids)
    
    def _draw_spawn_point(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw a SpawnPoint element."""
        start, _ = self._get_start_end(item)
        if start is None:
            return
        
        wx, wy = start
        yaw = float(item.get("Yaw", 0.0))
        
        sx, sy = self.helper.world_to_screen(wx, wy)
        
        # Body circle
        radius = 12
        cid = self.helper.canvas.create_oval(
            sx - radius, sy - radius, sx + radius, sy + radius,
            fill="#00cc66", outline="#006633", width=2
        )
        canvas_ids.append(cid)
        register_part(cid, "body")
        
        # Direction indicator (Yaw 0 = facing +X in UE world = right on screen)
        # UE Yaw: 0=+X, 90=+Y, 180=-X, 270=-Y
        # Screen: 0=right, 90=down (Y inverted), etc.
        angle_rad = math.radians(-yaw + 90)  # Rotate 90 CW to correct offset
        arrow_len = 25
        end_x = sx + arrow_len * math.sin(angle_rad)
        end_y = sy - arrow_len * math.cos(angle_rad)
        
        arrow_id = self.helper.canvas.create_line(
            sx, sy, end_x, end_y,
            fill="#006633", width=3, arrow=tk.LAST, arrowshape=(8, 10, 4)
        )
        canvas_ids.append(arrow_id)
        register_part(arrow_id, "arrow")
        
        # Place spawn point elements in spawn layer
        for c in [cid, arrow_id]:
            self._place_in_layer(c, "spawn_layer")
        
        # Center anchor
        self._create_anchor(wx, wy, "#00cc66", 5,
                           {"kind": "center", "center": (wx, wy)},
                           anchors, canvas_ids)
    
    def _draw_room_tone(
        self,
        item: Dict,
        canvas_ids: List[int],
        anchors: List[int],
        register_part: Callable,
    ):
        """Draw a RoomTone element."""
        start, _ = self._get_start_end(item)
        if start is None:
            return
        
        wx, wy = start
        radius = float(item.get("AttenuationRadius", 1600.0))
        
        sx, sy = self.helper.world_to_screen(wx, wy)
        
        # Attenuation radius circle
        scale = self.helper.world_to_screen_scale()
        screen_radius = radius * scale
        
        radius_cid = self.helper.canvas.create_oval(
            sx - screen_radius, sy - screen_radius,
            sx + screen_radius, sy + screen_radius,
            outline="#ffcc66", dash=(6, 4), fill="", width=1
        )
        canvas_ids.append(radius_cid)
        register_part(radius_cid, "radius")
        
        # Body marker
        marker_size = 8
        cid = self.helper.canvas.create_oval(
            sx - marker_size, sy - marker_size,
            sx + marker_size, sy + marker_size,
            fill="#ff9900", outline="#cc6600", width=2
        )
        canvas_ids.append(cid)
        register_part(cid, "body")
        
        # Place room tone elements in roomtone layer
        for c in [radius_cid, cid]:
            self._place_in_layer(c, "roomtone_layer")
        
        # Audio ID label
        audio_id = item.get("AudioId", "")
        if audio_id:
            text_id = self.helper.canvas.create_text(
                sx, sy + marker_size + 10,
                text=audio_id, fill="#cc6600", font=("Arial", 8)
            )
            canvas_ids.append(text_id)
            register_part(text_id, "label")
            self._place_in_layer(text_id, "label_layer")
        
        # Center anchor
        self._create_anchor(wx, wy, "#ff9900", 5,
                           {"kind": "center", "center": (wx, wy)},
                           anchors, canvas_ids)
    
    def _draw_id_label(
        self,
        item: Dict,
        canvas_ids: List[int],
        register_part: Callable,
    ):
        """Draw an ID label if the item has one."""
        item_id = item.get("Id")
        if not item_id or not canvas_ids:
            return
        
        item_type = item.get("Type", "")
        obj_type = get_object_type(item_type)
        display_dims = self.get_display_dims() if item_type == "Cubicle" else None
        
        label_pos = obj_type.get_id_label_position(item, display_dims)
        if label_pos is None:
            return
        
        sx, sy = self.helper.world_to_screen(*label_pos)
        
        # Create background for readability
        label_font = ("Segoe UI", 10, "bold")
        temp_text = self.helper.canvas.create_text(
            0, 0, text=item_id, font=label_font
        )
        bbox = self.helper.canvas.bbox(temp_text)
        self.helper.canvas.delete(temp_text)
        
        bg_id = None
        if bbox:
            tw = bbox[2] - bbox[0]
            th = bbox[3] - bbox[1]
            pad = 4
            bg_id = self.helper.canvas.create_rectangle(
                sx - tw/2 - pad, sy - th/2 - pad,
                sx + tw/2 + pad, sy + th/2 + pad,
                fill="#ffffff", outline="#333333", width=1
            )
            canvas_ids.append(bg_id)
            register_part(bg_id, "id_label_bg")
            self._place_in_layer(bg_id, "label_layer")
        
        text_id = self.helper.canvas.create_text(
            sx, sy, text=item_id, fill="#000000",
            font=label_font, anchor="center"
        )
        canvas_ids.append(text_id)
        register_part(text_id, "id_label")
        self._place_in_layer(text_id, "label_layer")
        
        # Ensure text is above background
        if bg_id is not None:
            self.helper.canvas.tag_raise(text_id, bg_id)
