"""
Canvas drawing helpers for the Floorplan Editor.

Provides utilities for:
- Coordinate transforms (world <-> screen)
- Drawing primitives (lines, rectangles, markers)
- Canvas element management
"""

import math
import tkinter as tk
from typing import Tuple, Optional, Dict, List, Any, Callable, Literal

Point = Tuple[float, float]
BBox = Tuple[float, float, float, float]


class CanvasHelper:
    """Helper class for canvas drawing operations."""
    
    def __init__(self, canvas: tk.Canvas, get_world_bbox: Callable[[], BBox]):
        self.canvas = canvas
        self.get_world_bbox = get_world_bbox
        self._canvas_width = 1200
        self._canvas_height = 800
    
    def safe_delete(self, *canvas_ids: Optional[int]) -> None:
        """Safely delete canvas items, ignoring TclError if already deleted."""
        for cid in canvas_ids:
            if cid is not None:
                try:
                    self.canvas.delete(cid)
                except tk.TclError:
                    pass
    
    def safe_configure(self, canvas_id: Optional[int], **kwargs) -> None:
        """Safely configure a canvas item, ignoring TclError if item doesn't exist."""
        if canvas_id is not None:
            try:
                self.canvas.itemconfigure(canvas_id, **kwargs)
            except tk.TclError:
                pass
    
    def safe_move(self, canvas_id: Optional[int], dx: float, dy: float) -> None:
        """Safely move a canvas item, ignoring TclError if item doesn't exist."""
        if canvas_id is not None:
            try:
                self.canvas.move(canvas_id, dx, dy)
            except tk.TclError:
                pass
    
    def safe_tag_raise(self, canvas_id: Optional[int]) -> None:
        """Safely raise a canvas item to top, ignoring TclError if item doesn't exist."""
        if canvas_id is not None:
            try:
                self.canvas.tag_raise(canvas_id)
            except tk.TclError:
                pass
    
    def current_canvas_size(self) -> Tuple[int, int]:
        """Get current canvas dimensions."""
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        if width <= 1 or height <= 1:
            return self._canvas_width, self._canvas_height
        self._canvas_width = width
        self._canvas_height = height
        return width, height
    
    def world_to_screen(self, x: float, y: float) -> Tuple[float, float]:
        """Convert world coordinates to screen coordinates."""
        wx0, wy0, wx1, wy1 = self.get_world_bbox()
        if wx1 == wx0:
            wx1 = wx0 + 1
        if wy1 == wy0:
            wy1 = wy0 + 1
        width, height = self.current_canvas_size()
        sx = (x - wx0) / (wx1 - wx0) * width
        sy = height - (y - wy0) / (wy1 - wy0) * height
        return sx, sy
    
    def screen_to_world(self, sx: float, sy: float) -> Tuple[float, float]:
        """Convert screen coordinates to world coordinates."""
        wx0, wy0, wx1, wy1 = self.get_world_bbox()
        width, height = self.current_canvas_size()
        x = wx0 + sx / width * (wx1 - wx0)
        y = wy0 + (height - sy) / height * (wy1 - wy0)
        return x, y
    
    def world_to_screen_scale(self) -> float:
        """Get the current world-to-screen scale factor."""
        wx0, wy0, wx1, wy1 = self.get_world_bbox()
        width, _ = self.current_canvas_size()
        if wx1 == wx0:
            return 1.0
        return width / (wx1 - wx0)
    
    def create_anchor_marker(
        self,
        wx: float,
        wy: float,
        color: str = "#ff8844",
        size: int = 5,
        state: str = "hidden",
        tags: Tuple[str, ...] = ("anchor",),
        meta: Optional[Dict[str, Any]] = None,
    ) -> int:
        """Create an anchor marker at world coordinates."""
        sx, sy = self.world_to_screen(wx, wy)
        cid = self.canvas.create_rectangle(
            sx - size, sy - size, sx + size, sy + size,
            outline="", fill=color, tags=tags
        )
        if state != "normal":
            self.canvas.itemconfigure(cid, state=state)  # type: ignore[arg-type]
        # Ensure anchors are always on top
        self.canvas.tag_raise(cid)
        return cid
    
    def create_line(
        self,
        start: Point,
        end: Point,
        color: str = "#000000",
        width: float = 1.0,
        dash: Optional[Tuple[int, ...]] = None,
        capstyle: str = tk.ROUND,
        tags: Tuple[str, ...] = (),
    ) -> int:
        """Create a line from world coordinates."""
        sx0, sy0 = self.world_to_screen(*start)
        sx1, sy1 = self.world_to_screen(*end)
        kwargs: Dict[str, Any] = {
            "fill": color,
            "width": width,
            "capstyle": capstyle,
            "tags": tags,
        }
        if dash:
            kwargs["dash"] = dash
        return self.canvas.create_line(sx0, sy0, sx1, sy1, **kwargs)
    
    def create_rectangle(
        self,
        corner1: Point,
        corner2: Point,
        fill: str = "",
        outline: str = "#000000",
        width: float = 1.0,
        dash: Optional[Tuple[int, ...]] = None,
        tags: Tuple[str, ...] = (),
    ) -> int:
        """Create a rectangle from world coordinates."""
        sx0, sy0 = self.world_to_screen(*corner1)
        sx1, sy1 = self.world_to_screen(*corner2)
        kwargs: Dict[str, Any] = {
            "fill": fill,
            "outline": outline,
            "width": width,
            "tags": tags,
        }
        if dash:
            kwargs["dash"] = dash
        return self.canvas.create_rectangle(sx0, sy0, sx1, sy1, **kwargs)
    
    def create_polygon(
        self,
        points: List[Point],
        fill: str = "",
        outline: str = "#000000",
        width: float = 1.0,
        tags: Tuple[str, ...] = (),
    ) -> int:
        """Create a polygon from world coordinates."""
        screen_points = []
        for p in points:
            sx, sy = self.world_to_screen(*p)
            screen_points.extend([sx, sy])
        return self.canvas.create_polygon(
            screen_points, fill=fill, outline=outline, width=width, tags=tags
        )
    
    def create_oval(
        self,
        center: Point,
        radius_x: float,
        radius_y: Optional[float] = None,
        fill: str = "",
        outline: str = "#000000",
        width: float = 1.0,
        dash: Optional[Tuple[int, ...]] = None,
        tags: Tuple[str, ...] = (),
    ) -> int:
        """Create an oval/circle from world coordinates and radius."""
        if radius_y is None:
            radius_y = radius_x
        
        # Convert radius to screen space
        scale = self.world_to_screen_scale()
        sr_x = radius_x * scale
        sr_y = radius_y * scale
        
        cx, cy = self.world_to_screen(*center)
        kwargs: Dict[str, Any] = {
            "fill": fill,
            "outline": outline,
            "width": width,
            "tags": tags,
        }
        if dash:
            kwargs["dash"] = dash
        return self.canvas.create_oval(
            cx - sr_x, cy - sr_y, cx + sr_x, cy + sr_y, **kwargs
        )
    
    def create_text(
        self,
        position: Point,
        text: str,
        color: str = "#000000",
        font: Tuple[str, int, str] = ("Segoe UI", 9, "normal"),
        anchor: Literal["nw", "n", "ne", "w", "center", "e", "sw", "s", "se"] = "center",
        tags: Tuple[str, ...] = (),
    ) -> int:
        """Create text at world coordinates."""
        sx, sy = self.world_to_screen(*position)
        return self.canvas.create_text(
            sx, sy, text=text, fill=color, font=font, anchor=anchor, tags=tags
        )
    
    def apply_colors(
        self,
        cid: int,
        fill: Optional[str] = None,
        outline: Optional[str] = None,
    ) -> None:
        """Apply fill and/or outline colors to a canvas item, handling item type differences."""
        options: Dict[str, str] = {}
        if fill is not None:
            options["fill"] = fill
        if outline is not None:
            options["outline"] = outline
        
        if not options:
            return
        
        try:
            self.canvas.itemconfig(cid, **options)  # type: ignore[arg-type]
            return
        except tk.TclError:
            pass
        
        # Handle line items (only have fill, no outline)
        item_type = self.canvas.type(cid)
        if item_type == "line":
            if fill is not None:
                try:
                    self.canvas.itemconfig(cid, fill=fill)
                except tk.TclError:
                    pass
            return
        
        # Try individual options
        if fill is not None:
            try:
                self.canvas.itemconfig(cid, fill=fill)
            except tk.TclError:
                pass
        
        if outline is not None:
            try:
                self.canvas.itemconfig(cid, outline=outline)
            except tk.TclError:
                pass


def draw_grid(
    helper: CanvasHelper,
    grid_size: float,
    axis_color: str = "#d0d0d0",
    grid_color: str = "#eeeeee",
) -> List[int]:
    """Draw a grid and return the canvas IDs."""
    if grid_size <= 0:
        return []
    
    ids = []
    wx0, wy0, wx1, wy1 = helper.get_world_bbox()
    if wx0 > wx1:
        wx0, wx1 = wx1, wx0
    if wy0 > wy1:
        wy0, wy1 = wy1, wy0
    
    start_x = math.floor(wx0 / grid_size)
    end_x = math.ceil(wx1 / grid_size)
    start_y = math.floor(wy0 / grid_size)
    end_y = math.ceil(wy1 / grid_size)
    
    for i in range(start_x, end_x + 1):
        x = i * grid_size
        sx0, sy0 = helper.world_to_screen(x, wy0)
        sx1, sy1 = helper.world_to_screen(x, wy1)
        is_axis = abs(x) < 1e-6
        color = axis_color if is_axis else grid_color
        width = 2 if is_axis else 1
        line = helper.canvas.create_line(
            sx0, sy0, sx1, sy1, fill=color, width=width, tags=("grid",)
        )
        ids.append(line)
    
    for j in range(start_y, end_y + 1):
        y = j * grid_size
        sx0, sy0 = helper.world_to_screen(wx0, y)
        sx1, sy1 = helper.world_to_screen(wx1, y)
        is_axis = abs(y) < 1e-6
        color = axis_color if is_axis else grid_color
        width = 2 if is_axis else 1
        line = helper.canvas.create_line(
            sx0, sy0, sx1, sy1, fill=color, width=width, tags=("grid",)
        )
        ids.append(line)
    
    helper.canvas.tag_lower("grid")
    return ids


def draw_direction_arrow(
    helper: CanvasHelper,
    position: Point,
    yaw: float,
    length: float = 40.0,
    color: str = "#ff6600",
    width: float = 3.0,
) -> int:
    """Draw an arrow indicating direction."""
    angle_rad = math.radians(-yaw)  # Negative for screen coords
    sx, sy = helper.world_to_screen(*position)
    
    # Arrow points from center outward
    end_x = sx + length * math.cos(angle_rad)
    end_y = sy + length * math.sin(angle_rad)
    
    return helper.canvas.create_line(
        sx, sy, end_x, end_y,
        fill=color, width=width,
        arrow=tk.LAST, arrowshape=(10, 12, 5)
    )
