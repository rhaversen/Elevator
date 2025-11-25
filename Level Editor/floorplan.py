"""
Floorplan Editor - Main Application

A tkinter-based level editor for creating and editing floorplan layouts.
Refactored to use modular helper classes.
"""

import json
import math
import os
import tkinter as tk
from tkinter import filedialog, messagebox

# Import helper modules
from geometry import normalize_yaw, get_axis_size, snap_value, snap_point, rotate_point
from file_io import load_layout_file, save_layout_file, get_display_name
from object_types import get_object_type, get_mode_color, LINE_MODES, RECT_MODES, POINT_MODES, TYPE_MAP
from canvas_helper import CanvasHelper, draw_grid
from object_renderer import ObjectRenderer
from editor_state import EditorState
from properties_panel import PropertiesPanel
from object_factory import create_floor_ceiling_pair, create_line_object, create_rect_object, create_point_object


class FloorplanEditor:
    """Main floorplan editor application."""
    
    def __init__(self, root):
        self.root = root
        self.root.title("Floorplan Editor")

        self.canvas_width = 1200
        self.canvas_height = 800

        # Editor state (data, selection, undo/redo, clipboard, floor/ceiling links)
        self.state = EditorState(max_undo=50)

        # Canvas objects (visual representations, not data)
        self.objects = []
        self.id_to_obj = {}
        self.anchor_meta = {}
        self.canvas_grid_ids = []

        # Selection UI state
        self._unified_anchor_ids = []  # Unified anchors for multi-selection
        self._drag_pending_wx = 0.0  # Pending drag delta for snapping
        self._drag_pending_wy = 0.0

        # View state
        self.world_bbox = (-1600.0, -900.0, 1600.0, 900.0)
        self.min_zoom_span = 50.0
        self.max_zoom_span = 50000.0
        self.pan_active = False
        self.pan_last_sx = 0
        self.pan_last_sy = 0

        # Interaction state
        self.pending_line = None
        self.pending_preview_id = None
        self.dragging = False
        self.drag_start_sx = 0
        self.drag_start_sy = 0
        self.drag_saved_state = False
        self.dragging_anchor = None  # {"item": data_dict, "meta": anchor_meta_dict}
        self.box_selecting = False
        self.box_select_start = None
        self.box_select_rect_id = None

        # UI variables
        self.current_mode = tk.StringVar(value="select")
        self.show_grid = tk.BooleanVar(value=True)
        self.snap_enabled = tk.BooleanVar(value=True)
        self.grid_size = tk.DoubleVar(value=100.0)
        self.show_lamps = tk.BooleanVar(value=True)
        self.lock_floor_ceiling = tk.BooleanVar(value=True)
        self.cubicle_display_width = tk.DoubleVar(value=300.0)
        self.cubicle_display_depth = tk.DoubleVar(value=250.0)

        # Set up traces
        self.current_mode.trace_add("write", self.on_mode_changed)
        self.grid_size.trace_add("write", self.on_grid_setting_changed)
        self.show_grid.trace_add("write", self.on_grid_setting_changed)
        self.cubicle_display_width.trace_add("write", self.on_cubicle_display_changed)
        self.cubicle_display_depth.trace_add("write", self.on_cubicle_display_changed)

        self._build_ui()
        
        # Initialize helpers after canvas is created
        self.canvas_helper = CanvasHelper(self.canvas, lambda: self.world_bbox)
        self.renderer = ObjectRenderer(
            self.canvas_helper,
            lambda: (self.cubicle_display_width.get(), self.cubicle_display_depth.get()),
            lambda: self.show_lamps.get()
        )

    # =========================================================================
    # State Property Accessors (delegate to EditorState)
    # =========================================================================

    @property
    def data(self):
        return self.state.data

    @data.setter
    def data(self, value):
        self.state.data = value

    @property
    def root_json(self):
        return self.state.root_json

    @root_json.setter
    def root_json(self, value):
        self.state.root_json = value

    @property
    def current_file_path(self):
        return self.state.current_file_path

    @current_file_path.setter
    def current_file_path(self, value):
        self.state.current_file_path = value

    @property
    def selected_objects(self):
        return self.state.selected_objects

    @selected_objects.setter
    def selected_objects(self, value):
        self.state.selected_objects = value

    @property
    def selected_obj(self):
        return self.state.primary_selected

    @selected_obj.setter
    def selected_obj(self, value):
        self.state.primary_selected = value

    @property
    def clipboard(self):
        return self.state.clipboard

    @clipboard.setter
    def clipboard(self, value):
        self.state.clipboard = value

    @property
    def undo_stack(self):
        return self.state.undo_stack

    @property
    def redo_stack(self):
        return self.state.redo_stack

    @property
    def floor_ceiling_links(self):
        return self.state.floor_ceiling_links

    def _build_ui(self):
        """Build the user interface."""
        bg_color = "#f5f5f5"
        toolbar_bg = "#ffffff"
        accent_color = "#0078d7"

        self.root.configure(bg=bg_color)

        # Toolbar
        toolbar = tk.Frame(self.root, bg=toolbar_bg, relief=tk.FLAT, bd=1)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=2, pady=2)

        toolbar_top = tk.Frame(toolbar, bg=toolbar_bg)
        toolbar_top.pack(side=tk.TOP, fill=tk.X)
        toolbar_bottom = tk.Frame(toolbar, bg=toolbar_bg)
        toolbar_bottom.pack(side=tk.TOP, fill=tk.X)

        self._build_mode_frame(toolbar_top, toolbar_bg)
        self._build_view_frame(toolbar_top, toolbar_bg, accent_color)
        self._build_rotate_frame(toolbar_top, toolbar_bg, accent_color)
        self._build_selection_frame(toolbar_top, toolbar_bg)
        self._build_grid_frame(toolbar_bottom, toolbar_bg)
        self._build_display_frame(toolbar_bottom, toolbar_bg)
        self._build_properties_panel()
        self._build_status_bar()
        self._build_canvas()
        self._build_menu()

    def _build_mode_frame(self, parent, bg):
        """Build mode selection frame."""
        frame = tk.LabelFrame(parent, text="Mode", padx=8, pady=4, bg=bg,
                              relief=tk.GROOVE, bd=1)
        frame.pack(side=tk.LEFT, padx=5, pady=4)

        modes = [
            ("Select", "select"), ("Cubicle", "add_cubicle"),
            ("Wall", "add_wall"), ("Door", "add_door"),
            ("Window", "add_window"), ("Elevator", "add_elevator"),
            ("Spawn", "add_spawn"), ("RoomTone", "add_roomtone"),
            ("Floor+Ceiling", "add_floor_ceiling"),
            ("CeilingLight", "add_ceiling_light")
        ]
        for text, value in modes:
            tk.Radiobutton(frame, text=text, variable=self.current_mode, value=value,
                          bg=bg, activebackground=bg, selectcolor=bg
                          ).pack(side=tk.LEFT, padx=2)

    def _build_view_frame(self, parent, bg, accent):
        """Build view controls frame."""
        frame = tk.LabelFrame(parent, text="View", padx=8, pady=4, bg=bg,
                              relief=tk.GROOVE, bd=1)
        frame.pack(side=tk.LEFT, padx=5, pady=4)

        for text, cmd in [("Reset", self.reset_view), ("Zoom +", self.zoom_in),
                          ("Zoom −", self.zoom_out)]:
            tk.Button(frame, text=text, command=cmd, bg=bg,
                      activebackground=accent, relief=tk.RAISED, bd=1,
                      padx=8, pady=2).pack(side=tk.LEFT, padx=2)

    def _build_rotate_frame(self, parent, bg, accent):
        """Build rotation controls frame."""
        frame = tk.LabelFrame(parent, text="Rotate", padx=8, pady=4, bg=bg,
                              relief=tk.GROOVE, bd=1)
        frame.pack(side=tk.LEFT, padx=5, pady=4)

        for text, deg in [("⟳ 90°", 90), ("⟲ 90°", -90)]:
            tk.Button(frame, text=text, command=lambda d=deg: self.rotate_selection(d),
                      bg=bg, activebackground=accent, relief=tk.RAISED, bd=1,
                      padx=8, pady=2).pack(side=tk.LEFT, padx=2)

    def _build_selection_frame(self, parent, bg):
        """Build selection behavior frame."""
        frame = tk.LabelFrame(parent, text="Selection", padx=8, pady=4, bg=bg,
                              relief=tk.GROOVE, bd=1)
        frame.pack(side=tk.LEFT, padx=5, pady=4)

        tk.Checkbutton(frame, text="Link Floor/Ceiling", variable=self.lock_floor_ceiling,
                       command=self.on_floor_ceiling_link_changed,
                       bg=bg, activebackground=bg, selectcolor=bg).pack(side=tk.LEFT, padx=2)

    def _build_grid_frame(self, parent, bg):
        """Build grid controls frame."""
        frame = tk.LabelFrame(parent, text="Grid", padx=8, pady=4, bg=bg,
                              relief=tk.GROOVE, bd=1)
        frame.pack(side=tk.LEFT, padx=5, pady=4)

        tk.Checkbutton(frame, text="Snap", variable=self.snap_enabled,
                       bg=bg, activebackground=bg, selectcolor=bg).pack(side=tk.LEFT, padx=2)
        tk.Label(frame, text="Size:", bg=bg).pack(side=tk.LEFT, padx=(5, 2))
        tk.Spinbox(frame, from_=10, to=2000, increment=10, width=6,
                   textvariable=self.grid_size).pack(side=tk.LEFT, padx=2)
        tk.Checkbutton(frame, text="Show", variable=self.show_grid,
                       bg=bg, activebackground=bg, selectcolor=bg).pack(side=tk.LEFT, padx=(5, 2))

    def _build_display_frame(self, parent, bg):
        """Build display options frame."""
        frame = tk.LabelFrame(parent, text="Display", padx=8, pady=4, bg=bg,
                              relief=tk.GROOVE, bd=1)
        frame.pack(side=tk.LEFT, padx=5, pady=4)

        tk.Checkbutton(frame, text="Lamps", variable=self.show_lamps,
                       command=lambda: self.rebuild_canvas(preserve_selection=True),
                       bg=bg, activebackground=bg, selectcolor=bg).pack(side=tk.LEFT, padx=2)

        tk.Label(frame, text="Cubicle W:", bg=bg).pack(side=tk.LEFT, padx=(8, 2))
        tk.Spinbox(frame, from_=50, to=1000, increment=10, width=7,
                   textvariable=self.cubicle_display_width).pack(side=tk.LEFT, padx=2)
        tk.Label(frame, text="D:", bg=bg).pack(side=tk.LEFT, padx=(2, 2))
        tk.Spinbox(frame, from_=50, to=1000, increment=10, width=7,
                   textvariable=self.cubicle_display_depth).pack(side=tk.LEFT, padx=2)

    def _build_properties_panel(self):
        """Build the properties panel."""
        self.properties_panel = PropertiesPanel(
            parent=self.root,
            on_property_changed=lambda: self.rebuild_canvas(preserve_selection=True),
            get_lock_floor_ceiling=lambda: self.lock_floor_ceiling.get(),
            sync_floor_ceiling_partner=self.sync_floor_ceiling_partner,
        )

    def _build_status_bar(self):
        """Build the status bar."""
        status_frame = tk.Frame(self.root, relief=tk.FLAT, bd=1, bg="#e1e1e1", height=28)
        status_frame.pack(side=tk.BOTTOM, fill=tk.X)
        self.status_label = tk.Label(
            status_frame,
            text="Pan: Right/Middle drag | Zoom: Wheel | Undo: Ctrl+Z | Copy/Paste: Ctrl+C/V",
            anchor=tk.W, bg="#e1e1e1", fg="#333333", font=("Segoe UI", 9))
        self.status_label.pack(side=tk.LEFT, padx=8, pady=4)

    def _build_canvas(self):
        """Build the main canvas."""
        self.canvas = tk.Canvas(self.root, width=self.canvas_width,
                                height=self.canvas_height, bg="#ffffff",
                                highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        # Mouse bindings
        self.canvas.bind("<Button-1>", self.on_left_click)
        self.canvas.bind("<B1-Motion>", self.on_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)
        self.canvas.bind("<ButtonPress-2>", self.on_pan_start)
        self.canvas.bind("<B2-Motion>", self.on_pan_move)
        self.canvas.bind("<ButtonRelease-2>", self.on_pan_end)
        self.canvas.bind("<ButtonPress-3>", self.on_pan_start)
        self.canvas.bind("<B3-Motion>", self.on_pan_move)
        self.canvas.bind("<ButtonRelease-3>", self.on_pan_end)
        self.canvas.bind("<MouseWheel>", self.on_mousewheel)
        self.canvas.bind("<Button-4>", lambda e: self.on_mousewheel(e, 1))
        self.canvas.bind("<Button-5>", lambda e: self.on_mousewheel(e, -1))
        self.canvas.bind("<Configure>", self.on_canvas_configure)

        # Keyboard bindings
        self.root.bind("<Delete>", self.on_delete)
        self.root.bind("<BackSpace>", self.on_delete)
        self.root.bind("<Escape>", lambda e: self.cancel_transient_actions())
        self.root.bind("<r>", lambda e: self.rotate_selection(90))
        self.root.bind("<R>", lambda e: self.rotate_selection(-90))
        self.root.bind("<Control-c>", self.on_copy)
        self.root.bind("<Control-v>", self.on_paste)
        self.root.bind("<Control-a>", self.on_select_all)
        self.root.bind("<Control-z>", self.undo)
        self.root.bind("<Control-y>", self.redo)

    def _build_menu(self):
        """Build the menu bar."""
        menubar = tk.Menu(self.root)
        filemenu = tk.Menu(menubar, tearoff=0)
        filemenu.add_command(label="Open...", command=self.open_file)
        filemenu.add_command(label="Reload", command=self.reload_file)
        filemenu.add_command(label="Save As...", command=self.save_file_as)
        menubar.add_cascade(label="File", menu=filemenu)
        self.root.config(menu=menubar)

    # =========================================================================
    # Coordinate Transforms
    # =========================================================================

    def current_canvas_size(self):
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        if width <= 1 or height <= 1:
            return self.canvas_width, self.canvas_height
        self.canvas_width = width
        self.canvas_height = height
        return width, height

    def world_to_screen(self, x, y):
        return self.canvas_helper.world_to_screen(x, y)

    def screen_to_world(self, sx, sy):
        return self.canvas_helper.screen_to_world(sx, sy)

    # =========================================================================
    # Grid & Snapping
    # =========================================================================

    def get_grid_size(self):
        try:
            value = float(self.grid_size.get())
        except (tk.TclError, ValueError):
            value = 100.0
        return max(value, 1.0)

    def snap_point_to_grid(self, x, y):
        """Snap a point to grid if snapping is enabled."""
        if not self.snap_enabled.get():
            return float(x), float(y)
        grid = self.get_grid_size()
        return snap_point((x, y), grid)

    def on_grid_setting_changed(self, *_):
        if hasattr(self, "canvas"):
            self.rebuild_canvas(preserve_selection=True)

    def on_cubicle_display_changed(self, *_):
        if hasattr(self, "canvas"):
            self.rebuild_canvas(preserve_selection=True)

    def on_floor_ceiling_link_changed(self, *_):
        # Sync UI state to EditorState
        self.state.lock_floor_ceiling = self.lock_floor_ceiling.get()
        if hasattr(self, "canvas"):
            if self.lock_floor_ceiling.get():
                self.ensure_floor_ceiling_pairs()
            self.rebuild_canvas(preserve_selection=True)

    def on_mode_changed(self, *_):
        mode = self.current_mode.get()
        if self.pending_line:
            pending_mode = self.pending_line.get("mode")
            if pending_mode != mode or mode not in (LINE_MODES | RECT_MODES):
                self.clear_pending_line()

    # =========================================================================
    # View Controls
    # =========================================================================

    def on_canvas_configure(self, event):
        new_w = max(event.width, 1)
        new_h = max(event.height, 1)
        if (new_w != self.canvas_width) or (new_h != self.canvas_height):
            self.canvas_width = new_w
            self.canvas_height = new_h
            if self.data:
                self.rebuild_canvas(preserve_selection=True)

    def on_mousewheel(self, event, wheel_delta=None):
        delta = wheel_delta if wheel_delta is not None else event.delta
        if delta == 0:
            return
        direction = 1 if delta > 0 else -1
        scale = 0.9 if direction > 0 else 1.1
        steps = max(1, int(abs(delta) / 120)) if wheel_delta is None else 1
        self.zoom_at(scale ** steps, event.x, event.y)

    def zoom_in(self):
        w, h = self.current_canvas_size()
        self.zoom_at(0.9, w / 2, h / 2)

    def zoom_out(self):
        w, h = self.current_canvas_size()
        self.zoom_at(1.1, w / 2, h / 2)

    def zoom_at(self, scale, cx_screen, cy_screen):
        if scale <= 0:
            return
        wx0, wy0, wx1, wy1 = self.world_bbox
        w, h = wx1 - wx0, wy1 - wy0
        if w <= 0 or h <= 0:
            return

        # Clamp scale
        min_scale = max(self.min_zoom_span / w, self.min_zoom_span / h, 1e-6)
        max_scale = min(self.max_zoom_span / w, self.max_zoom_span / h)
        scale = max(min_scale, min(scale, max_scale))
        if abs(scale - 1.0) < 1e-6:
            return

        cx, cy = self.screen_to_world(cx_screen, cy_screen)
        new_w, new_h = w * scale, h * scale
        cx_ratio = (cx - wx0) / w if w else 0.5
        cy_ratio = (cy - wy0) / h if h else 0.5

        self.world_bbox = (
            cx - cx_ratio * new_w,
            cy - cy_ratio * new_h,
            cx - cx_ratio * new_w + new_w,
            cy - cy_ratio * new_h + new_h
        )
        self.rebuild_canvas(preserve_selection=True)

    def on_pan_start(self, event):
        self.clear_pending_line()
        self.pan_active = True
        self.pan_last_sx = event.x
        self.pan_last_sy = event.y
        self.canvas.configure(cursor="fleur")

    def on_pan_move(self, event):
        if not self.pan_active:
            return
        dx = event.x - self.pan_last_sx
        dy = event.y - self.pan_last_sy
        if dx == 0 and dy == 0:
            return

        bx, by = self.screen_to_world(self.pan_last_sx, self.pan_last_sy)
        ax, ay = self.screen_to_world(event.x, event.y)
        self.canvas.move("all", dx, dy)

        wx0, wy0, wx1, wy1 = self.world_bbox
        self.world_bbox = (wx0 + bx - ax, wy0 + by - ay, wx1 + bx - ax, wy1 + by - ay)
        self.pan_last_sx = event.x
        self.pan_last_sy = event.y

    def on_pan_end(self, event):
        if self.pan_active:
            self.pan_active = False
            self.canvas.configure(cursor="")

    def reset_view(self):
        self.update_world_bbox_from_floor()
        self.rebuild_canvas(preserve_selection=True)

    # =========================================================================
    # File I/O
    # =========================================================================

    def open_file(self):
        path = filedialog.askopenfilename(
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")])
        if path:
            self.load_file(path)

    def reload_file(self):
        if self.current_file_path:
            self.load_file(self.current_file_path)
        else:
            messagebox.showinfo("Info", "Open a layout first to reload it.")

    def load_file(self, path):
        root_json, elements, error = load_layout_file(path)
        if error:
            messagebox.showerror("Error", f"Failed to open file:\n{error}")
            return
        
        self.state.set_data(elements, root_json)
        self.state.current_file_path = path
        self.reset_view()
        self.root.title(f"Floorplan Editor - {get_display_name(path)}")

    def save_file_as(self):
        if not self.data:
            messagebox.showinfo("Info", "Nothing to save.")
            return
        path = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")])
        if not path:
            return
        
        error = save_layout_file(path, self.data, self.root_json)
        if error:
            messagebox.showerror("Error", f"Failed to save file:\n{error}")
        else:
            self.current_file_path = path
            self.root.title(f"Floorplan Editor - {get_display_name(path)}")

    def update_world_bbox_from_floor(self):
        for item in self.data:
            if item.get("Type") == "Floor":
                s, e = item.get("Start", {}), item.get("End", {})
                self.world_bbox = (
                    float(s.get("X", -1600)), float(s.get("Y", -900)),
                    float(e.get("X", 1600)), float(e.get("Y", 900)))
                return
        self.world_bbox = (-1600.0, -900.0, 1600.0, 900.0)

    # =========================================================================
    # Canvas Drawing
    # =========================================================================

    def clear_pending_line(self):
        if not hasattr(self, "canvas"):
            self.pending_line = None
            return
        if self.pending_line:
            self.canvas_helper.safe_delete(
                self.pending_line.get("preview_id"),
                self.pending_line.get("start_marker_id")
            )
        self.pending_line = None
        self.pending_preview_id = None

    def cancel_transient_actions(self):
        self.clear_pending_line()
        if self.pan_active:
            self.pan_active = False
            self.canvas.configure(cursor="")
        self.dragging = False
        self.dragging_anchor = None
        self.drag_saved_state = False
        if self.box_selecting:
            self.canvas_helper.safe_delete(self.box_select_rect_id)
            self.box_selecting = False
            self.box_select_start = None
            self.box_select_rect_id = None

    def rebuild_canvas(self, preserve_selection=False):
        self.clear_pending_line()
        
        # Remember selection
        selected_data = [o["data"] for o in self.selected_objects] if preserve_selection else []
        primary_data = self.selected_obj["data"] if preserve_selection and self.selected_obj else None
        
        # Clear selection styling
        for obj in self.selected_objects:
            self.style_object(obj, selected=False)
        self.selected_obj = None
        self.selected_objects.clear()

        self.refresh_floor_ceiling_links()

        # Clear canvas
        self.canvas.delete("all")
        self.canvas_grid_ids.clear()
        self.objects.clear()
        self.id_to_obj.clear()
        self.anchor_meta.clear()

        # Draw grid
        if self.show_grid.get():
            self.canvas_grid_ids = draw_grid(self.canvas_helper, self.get_grid_size())

        # Draw items
        to_select = []
        for item in self.data:
            obj = self.renderer.draw_item(item)
            if obj:
                self.objects.append(obj)
                # Register IDs
                for cid in obj["canvas_ids"]:
                    self.id_to_obj[cid] = obj
                # Register anchor meta from renderer
                for anchor_id, meta in obj.get("anchor_meta", {}).items():
                    self.anchor_meta[anchor_id] = meta
                if item in selected_data:
                    to_select.append(obj)

        # Restore selection
        if to_select:
            combined = []
            for obj in to_select:
                if self.lock_floor_ceiling.get() and obj["data"].get("Type") in ("Floor", "Ceiling"):
                    group = self._get_linked_selection_group(obj)
                else:
                    group = [obj]
                for m in group:
                    if m not in combined:
                        combined.append(m)
            
            self.selected_objects = combined
            for obj in combined:
                self.style_object(obj, selected=True)
            
            if primary_data:
                for obj in combined:
                    if obj["data"] is primary_data:
                        self.selected_obj = obj
                        break
            if not self.selected_obj and combined:
                self.selected_obj = combined[0]

        self.update_properties_panel()
        self.draw_selection_anchors()

    def create_anchor_marker(self, wx, wy, color="#ff8844", size=5, state="hidden", meta=None):
        sx, sy = self.world_to_screen(wx, wy)
        cid = self.canvas.create_rectangle(sx - size, sy - size, sx + size, sy + size,
                                           outline="", fill=color, tags=("anchor",))
        if state != "normal":
            self.canvas.itemconfigure(cid, state=state)
        if meta:
            self.anchor_meta[cid] = dict(meta)
        # Ensure anchors are always on top
        self.canvas.tag_raise(cid)
        return cid

    def draw_selection_anchors(self):
        """Show anchors for selected objects or unified anchors for multi-selection."""
        # Clear any existing unified selection anchors
        self._clear_unified_selection_anchors()
        
        if len(self.selected_objects) == 0:
            return
        
        if len(self.selected_objects) == 1:
            # Single selection: show individual object anchors
            obj = self.selected_objects[0]
            for anchor_id in obj.get("anchors", []):
                self.canvas_helper.safe_configure(anchor_id, state="normal")
                self.canvas_helper.safe_tag_raise(anchor_id)  # Ensure on top
        else:
            # Multi-selection: hide individual anchors, show unified bounding box anchors
            for obj in self.selected_objects:
                for anchor_id in obj.get("anchors", []):
                    self.canvas_helper.safe_configure(anchor_id, state="hidden")
            
            # Create unified bounding box anchors
            self._create_unified_selection_anchors()
    
    def _clear_unified_selection_anchors(self):
        """Remove any unified selection anchors."""
        for cid in getattr(self, '_unified_anchor_ids', []):
            self.canvas_helper.safe_delete(cid)
        self._unified_anchor_ids = []
    
    def _get_selection_bounds(self):
        """Get the world bounding box of all selected objects."""
        min_x, min_y = float('inf'), float('inf')
        max_x, max_y = float('-inf'), float('-inf')
        
        for obj in self.selected_objects:
            item = obj["data"]
            item_type = item.get("Type", "")
            
            if "Start" in item:
                sx = float(item["Start"].get("X", 0))
                sy = float(item["Start"].get("Y", 0))
                min_x, min_y = min(min_x, sx), min(min_y, sy)
                max_x, max_y = max(max_x, sx), max(max_y, sy)
            
            if "End" in item:
                ex = float(item["End"].get("X", 0))
                ey = float(item["End"].get("Y", 0))
                min_x, min_y = min(min_x, ex), min(min_y, ey)
                max_x, max_y = max(max_x, ex), max(max_y, ey)
            
            # For point-based types, add some size
            if item_type in ("SpawnPoint", "RoomTone") and "End" not in item:
                min_x -= 50
                min_y -= 50
                max_x += 50
                max_y += 50
        
        if min_x == float('inf'):
            return None
        return (min_x, min_y, max_x, max_y)
    
    def _create_unified_selection_anchors(self):
        """Create unified anchors for multi-selection."""
        bounds = self._get_selection_bounds()
        if bounds is None:
            return
        
        min_x, min_y, max_x, max_y = bounds
        cx, cy = (min_x + max_x) / 2, (min_y + max_y) / 2
        
        self._unified_anchor_ids = []
        
        # Corner anchors for resizing (not implemented yet, just visual)
        corners = [(min_x, min_y), (max_x, min_y), (min_x, max_y), (max_x, max_y)]
        for idx, (wx, wy) in enumerate(corners):
            cid = self.create_anchor_marker(wx, wy, color="#ff8844", size=5, state="normal",
                                           meta={"kind": "unified_corner", "corner": idx})
            self._unified_anchor_ids.append(cid)
            self.id_to_obj[cid] = self.selected_objects[0]  # Associate with first object
        
        # Center anchor for moving all
        cid = self.create_anchor_marker(cx, cy, color="#44aaff", size=6, state="normal",
                                       meta={"kind": "unified_center"})
        self._unified_anchor_ids.append(cid)
        self.id_to_obj[cid] = self.selected_objects[0]

    # =========================================================================
    # Floor/Ceiling Linking
    # =========================================================================

    def refresh_floor_ceiling_links(self):
        """Delegate to EditorState."""
        self.state.refresh_floor_ceiling_links()

    def ensure_floor_ceiling_pairs(self):
        """Delegate to EditorState."""
        self.state.ensure_floor_ceiling_pairs()

    def sync_floor_ceiling_partner(self, item):
        """Delegate to EditorState."""
        self.state.sync_floor_ceiling_partner(item)

    # =========================================================================
    # Selection Management
    # =========================================================================

    def _get_linked_selection_group(self, obj):
        """Get selection group including floor/ceiling partner if linked."""
        item = obj["data"]
        group = [obj]
        partner_data = self.floor_ceiling_links.get(id(item))
        if partner_data:
            for o in self.objects:
                if o["data"] is partner_data and o not in group:
                    group.append(o)
                    break
        return group

    def set_selected(self, obj, add=False):
        """Set selection to obj, optionally adding to existing selection."""
        if not add:
            for o in self.selected_objects:
                self.style_object(o, selected=False)
            self.selected_objects.clear()
            self.selected_obj = None
            self._clear_unified_selection_anchors()

        if not obj:
            self.update_properties_panel()
            return

        # Get linked group
        if self.lock_floor_ceiling.get() and obj["data"].get("Type") in ("Floor", "Ceiling"):
            group = self._get_linked_selection_group(obj)
        else:
            group = [obj]

        for o in group:
            if o not in self.selected_objects:
                self.selected_objects.append(o)
                self.style_object(o, selected=True)

        if not add or not self.selected_obj:
            self.selected_obj = obj

        # Update anchor visibility for single vs multi-selection
        self.draw_selection_anchors()
        
        self.canvas.update_idletasks()  # Force immediate visual update
        self.update_properties_panel()

    def add_to_selection(self, obj):
        """Add obj to selection without clearing existing selection."""
        self.set_selected(obj, add=True)

    def remove_from_selection(self, obj):
        """Remove obj from selection."""
        if obj in self.selected_objects:
            self.style_object(obj, selected=False)
            self.selected_objects.remove(obj)
            if self.selected_obj is obj:
                self.selected_obj = self.selected_objects[0] if self.selected_objects else None
        # Update anchor visibility for single vs multi-selection
        self.draw_selection_anchors()
        self.canvas.update_idletasks()  # Force immediate visual update
        self.update_properties_panel()

    def clear_selection(self):
        """Clear all selection."""
        self._clear_unified_selection_anchors()
        self.set_selected(None)

    def select_all(self, event=None):
        """Select all objects."""
        self._clear_unified_selection_anchors()
        for o in self.selected_objects:
            self.style_object(o, selected=False)
        self.selected_objects.clear()

        for o in self.objects:
            self.selected_objects.append(o)
            self.style_object(o, selected=True)

        self.selected_obj = self.selected_objects[0] if self.selected_objects else None
        # Update anchor visibility for single vs multi-selection
        self.draw_selection_anchors()
        self.canvas.update_idletasks()  # Force immediate visual update
        self.update_properties_panel()

    def style_object(self, obj, selected=False):
        """Apply selected or normal styling to object, including anchor visibility."""
        if not obj:
            return
        parts = obj.get("parts", {})
        base_styles = obj.get("base_styles", {})
        
        # Selection outline/fill color
        sel_outline = "#ffffff"
        sel_fill = "#33ccff"
        
        # Show/hide individual anchors based on selection state
        # For multi-selection, anchors are always hidden (unified anchors used instead)
        if selected and len(self.selected_objects) <= 1:
            anchor_state = "normal"
        else:
            anchor_state = "hidden"
        
        for anchor_id in obj.get("anchors", []):
            self.canvas_helper.safe_configure(anchor_id, state=anchor_state)
        
        for part_type, ids in parts.items():
            for cid in ids:
                if part_type == "outline":
                    base = base_styles.get(cid, {})
                    if selected:
                        self.canvas.itemconfigure(cid, outline=sel_outline, width=3)
                    else:
                        self.canvas.itemconfigure(cid, outline=base.get("outline", "#cccccc"),
                                                  width=base.get("width", 2))
                elif part_type == "fill":
                    base = base_styles.get(cid, {})
                    if selected:
                        self.canvas.itemconfigure(cid, fill=sel_fill, stipple="gray50")
                    else:
                        self.canvas.itemconfigure(cid, fill=base.get("fill", ""),
                                                  stipple=base.get("stipple", ""))
                elif part_type == "direction":
                    base = base_styles.get(cid, {})
                    if selected:
                        self.canvas.itemconfigure(cid, fill=sel_outline)
                    else:
                        self.canvas.itemconfigure(cid, fill=base.get("fill", "#888888"))
                elif part_type == "body":
                    base = base_styles.get(cid, {})
                    if selected:
                        self.canvas.itemconfigure(cid, outline=sel_outline, width=3)
                    else:
                        self.canvas.itemconfigure(cid, outline=base.get("outline", "#cccccc"),
                                                  width=base.get("width", 2))

    def find_object_at(self, screen_x, screen_y):
        """Find the topmost object at screen coordinates."""
        items = self.canvas.find_overlapping(screen_x - 3, screen_y - 3,
                                              screen_x + 3, screen_y + 3)
        for cid in reversed(items):
            if cid in self.id_to_obj:
                return self.id_to_obj[cid]
        return None

    def find_anchor_at(self, screen_x, screen_y, ignore_data=None):
        """Find anchor at screen coordinates.
        
        Returns: (obj, meta) tuple or (None, None) if not found.
        """
        items = self.canvas.find_overlapping(screen_x - 3, screen_y - 3,
                                              screen_x + 3, screen_y + 3)
        for cid in reversed(items):
            if "anchor" not in self.canvas.gettags(cid):
                continue
            meta = self.anchor_meta.get(cid)
            if meta is None:
                continue
            obj = self.id_to_obj.get(cid)
            if obj is None:
                continue
            if ignore_data and obj.get("data") is ignore_data:
                continue
            return obj, meta
        return None, None

    def box_select_objects_in_rect(self, wx0, wy0, wx1, wy1):
        """Select objects within the world-space rectangle."""
        min_x, max_x = min(wx0, wx1), max(wx0, wx1)
        min_y, max_y = min(wy0, wy1), max(wy0, wy1)

        matching = []
        for obj in self.objects:
            item = obj["data"]
            t = item.get("Type", "")
            type_def = TYPE_MAP.get(t)
            if not type_def:
                continue

            # Get object center
            cx, cy = type_def.get_center(item)
            if min_x <= cx <= max_x and min_y <= cy <= max_y:
                matching.append(obj)

        # Update selection
        for o in self.selected_objects:
            self.style_object(o, selected=False)
        self.selected_objects.clear()

        for obj in matching:
            if self.lock_floor_ceiling.get() and obj["data"].get("Type") in ("Floor", "Ceiling"):
                for member in self._get_linked_selection_group(obj):
                    if member not in self.selected_objects:
                        self.selected_objects.append(member)
            else:
                if obj not in self.selected_objects:
                    self.selected_objects.append(obj)

        for o in self.selected_objects:
            self.style_object(o, selected=True)

        self.selected_obj = self.selected_objects[0] if self.selected_objects else None
        self.update_properties_panel()

    # =========================================================================
    # Properties Panel
    # =========================================================================

    def update_properties_panel(self):
        """Update the properties panel for current selection."""
        if not self.selected_objects:
            self.properties_panel.show_no_selection()
        elif len(self.selected_objects) > 1:
            self.properties_panel.show_multi_selection(
                self.selected_objects,
                self.clear_selection,
                self.remove_from_selection,
            )
        elif self.selected_obj:
            self.properties_panel.show_single_selection(self.selected_obj)
        else:
            self.properties_panel.show_no_selection()

    # =========================================================================
    # Event Handlers
    # =========================================================================

    def on_left_click(self, event):
        """Handle left click on canvas."""
        sx, sy = event.x, event.y
        wx, wy = self.screen_to_world(sx, sy)

        mode = self.current_mode.get()

        # Clear pending line if switching contexts
        if mode not in LINE_MODES and mode not in RECT_MODES:
            self.clear_pending_line()

        # Select mode
        if mode == "select":
            ctrl_held = event.state & 0x4

            # Check for anchor click first
            anchor_obj, anchor_meta = self.find_anchor_at(sx, sy)
            if anchor_obj is not None and anchor_meta is not None:
                # Start anchor drag
                if anchor_obj not in self.selected_objects:
                    self.set_selected(anchor_obj)
                # Initialize anchor drag with starting world position for smooth dragging
                wx, wy = self.screen_to_world(sx, sy)
                self.dragging_anchor = {
                    "item": anchor_obj["data"], 
                    "meta": anchor_meta,
                    "last_wx": wx,
                    "last_wy": wy
                }
                self.drag_saved_state = False
                self.dragging = True
                self.drag_start_sx = sx
                self.drag_start_sy = sy
                return

            # Check for object click
            obj = self.find_object_at(sx, sy)
            if obj:
                if ctrl_held:
                    if obj in self.selected_objects:
                        self.remove_from_selection(obj)
                    else:
                        self.add_to_selection(obj)
                elif obj in self.selected_objects:
                    # Already selected, prepare for drag
                    pass
                else:
                    self.set_selected(obj)
                
                self.dragging = True
                self.drag_start_sx = sx
                self.drag_start_sy = sy
                self.drag_saved_state = False
                self._drag_pending_wx = 0.0
                self._drag_pending_wy = 0.0
            else:
                # Start box selection
                if not ctrl_held:
                    self.clear_selection()
                self.box_selecting = True
                self.box_select_start = (sx, sy)
                self.box_select_rect_id = self.canvas.create_rectangle(
                    sx, sy, sx, sy, outline="#0078d7", dash=(3, 3), width=1
                )
            return

        # Drawing modes (line/rect)
        if mode in LINE_MODES or mode in RECT_MODES:
            if self.snap_enabled.get():
                wx, wy = self.snap_point_to_grid(wx, wy)

            pending = self.pending_line
            
            # If pending exists and was dragged, start fresh
            if pending and pending.get("dragged"):
                self.clear_pending_line()
                pending = None
            
            # If no pending line, start one
            if pending is None:
                self._begin_pending_line(mode, wx, wy)
                return
            
            # Second click - complete the object
            start_x = pending["start_x"]
            start_y = pending["start_y"]
            if abs(wx - start_x) > 1 or abs(wy - start_y) > 1:
                self._complete_pending_object(wx, wy, mode)
            self.clear_pending_line()
            return

        # Point modes (single click to place)
        if mode in POINT_MODES:
            if self.snap_enabled.get():
                wx, wy = self.snap_point_to_grid(wx, wy)
            self._create_point_object(wx, wy, mode)

    def _begin_pending_line(self, mode, wx, wy):
        """Start a new pending line/rect."""
        color = get_mode_color(mode)
        sx, sy = self.world_to_screen(wx, wy)
        
        # Create preview shape
        if mode in RECT_MODES:
            preview_id = self.canvas.create_rectangle(
                sx, sy, sx, sy, outline=color, dash=(8, 4), width=2)
        else:
            preview_id = self.canvas.create_line(
                sx, sy, sx, sy, fill=color, dash=(8, 4), width=2)
        
        # Create start marker
        marker_id = self.create_anchor_marker(wx, wy, color=color, size=6, state="normal")
        
        self.pending_line = {
            "mode": mode,
            "start_x": wx, "start_y": wy,
            "preview_id": preview_id,
            "start_marker_id": marker_id,
            "dragged": False,
        }

    def on_drag(self, event):
        """Handle mouse drag."""
        sx, sy = event.x, event.y
        wx, wy = self.screen_to_world(sx, sy)

        mode = self.current_mode.get()

        # Box selection drag
        if self.box_selecting and self.box_select_start is not None:
            bsx, bsy = self.box_select_start
            if self.box_select_rect_id is not None:
                self.canvas.coords(self.box_select_rect_id, bsx, bsy, sx, sy)
            return

        # Pending line preview
        if self.pending_line and self.pending_line.get("preview_id") is not None:
            self._update_pending_line(event)
            return

        # Anchor drag (resize/move via anchor)
        if self.dragging_anchor is not None:
            if not self.drag_saved_state:
                self.push_undo()
                self.drag_saved_state = True
            self._handle_anchor_drag(event)
            return

        # Object drag (move selected objects)
        if not self.dragging or not self.selected_objects:
            return
        
        # Save state on first actual movement
        if not self.drag_saved_state:
            self.push_undo()
            self.drag_saved_state = True
        
        # For snapping, we track a floating-point "pending" position
        # and only move when snap threshold is reached
        if not hasattr(self, '_drag_pending_wx'):
            self._drag_pending_wx = 0.0
            self._drag_pending_wy = 0.0
        
        # Calculate world delta
        dx_pix = sx - self.drag_start_sx
        dy_pix = sy - self.drag_start_sy
        
        wx0, wy0, wx1, wy1 = self.world_bbox
        width, height = self.current_canvas_size()
        dx_world = dx_pix / width * (wx1 - wx0)
        dy_world = -dy_pix / height * (wy1 - wy0)
        
        self.drag_start_sx = sx
        self.drag_start_sy = sy
        
        # Apply snapping if enabled
        if self.snap_enabled.get():
            grid = self.get_grid_size()
            # Accumulate delta
            self._drag_pending_wx += dx_world
            self._drag_pending_wy += dy_world
            # Snap to grid increments
            snapped_dx = round(self._drag_pending_wx / grid) * grid
            snapped_dy = round(self._drag_pending_wy / grid) * grid
            if abs(snapped_dx) >= grid or abs(snapped_dy) >= grid:
                self._drag_pending_wx -= snapped_dx
                self._drag_pending_wy -= snapped_dy
                self._move_selected_objects(snapped_dx, snapped_dy)
        else:
            # Move all selected objects
            self._move_selected_objects(dx_world, dy_world)

    def _update_pending_line(self, event):
        """Update the preview for pending line/rect."""
        if not self.pending_line:
            return
            
        wx, wy = self.screen_to_world(event.x, event.y)
        if self.snap_enabled.get():
            wx, wy = self.snap_point_to_grid(wx, wy)
        
        start_x, start_y = self.pending_line["start_x"], self.pending_line["start_y"]
        sx0, sy0 = self.world_to_screen(start_x, start_y)
        sx1, sy1 = self.world_to_screen(wx, wy)
        
        mode = self.pending_line.get("mode", "")
        preview_id = self.pending_line.get("preview_id")
        if preview_id:
            self.canvas.coords(preview_id, sx0, sy0, sx1, sy1)
        
        # Mark as dragged if moved
        if abs(wx - start_x) > 1 or abs(wy - start_y) > 1:
            self.pending_line["dragged"] = True

    def _handle_anchor_drag(self, event):
        """Handle dragging of an anchor for resize/move."""
        if not self.dragging_anchor:
            return

        item = self.dragging_anchor.get("item")
        meta = self.dragging_anchor.get("meta", {})
        if item is None:
            return

        wx, wy = self.screen_to_world(event.x, event.y)
        if self.snap_enabled.get():
            wx, wy = self.snap_point_to_grid(wx, wy)

        kind = meta.get("kind")
        
        # For center or unified_center anchor (move), use the smooth object dragging method
        if kind in ("center", "unified_center"):
            # Calculate delta from last position
            last_wx = self.dragging_anchor.get("last_wx", wx)
            last_wy = self.dragging_anchor.get("last_wy", wy)
            dx = wx - last_wx
            dy = wy - last_wy
            
            # Store current position for next delta
            self.dragging_anchor["last_wx"] = wx
            self.dragging_anchor["last_wy"] = wy
            
            # Use the same smooth movement as direct object dragging
            if abs(dx) > 1e-6 or abs(dy) > 1e-6:
                self._move_selected_objects(dx, dy)
            return
        
        # For unified_corner (multi-selection resize) - scale all selected objects
        if kind == "unified_corner":
            self._handle_unified_corner_drag(wx, wy, meta.get("corner", 0))
            return
        
        # For resize anchors (endpoint, corner, point), update data and rebuild
        item_type = item.get("Type")
        changed = False

        if kind == "endpoint":
            endpoint = meta.get("endpoint")
            target = item.get("Start") if endpoint == "start" else item.get("End")
            if target is not None:
                old_x = float(target.get("X", 0.0))
                old_y = float(target.get("Y", 0.0))
                if abs(old_x - wx) > 1e-6 or abs(old_y - wy) > 1e-6:
                    target["X"] = wx
                    target["Y"] = wy
                    changed = True

        elif kind == "corner":
            corner_index = meta.get("corner")
            if corner_index is not None and item_type in ("Floor", "Ceiling", "CeilingLight"):
                start = item.setdefault("Start", {})
                end = item.setdefault("End", {})
                
                # corner_index: 0=(x0,y0), 1=(x1,y0), 2=(x0,y1), 3=(x1,y1)
                target_x = start if corner_index in (0, 2) else end
                target_y = start if corner_index in (0, 1) else end
                
                old_x = float(target_x.get("X", 0.0))
                old_y = float(target_y.get("Y", 0.0))
                
                if abs(old_x - wx) > 1e-6:
                    target_x["X"] = wx
                    changed = True
                if abs(old_y - wy) > 1e-6:
                    target_y["Y"] = wy
                    changed = True

        elif kind == "point":
            start = item.setdefault("Start", {})
            old_x = float(start.get("X", 0.0))
            old_y = float(start.get("Y", 0.0))
            if abs(old_x - wx) > 1e-6 or abs(old_y - wy) > 1e-6:
                start["X"] = wx
                start["Y"] = wy
                changed = True

        if changed:
            # Sync floor/ceiling partner if needed
            if self.lock_floor_ceiling.get() and item_type in ("Floor", "Ceiling"):
                self.sync_floor_ceiling_partner(item)
            
            self.rebuild_canvas(preserve_selection=True)
    
    def _handle_unified_corner_drag(self, wx, wy, corner_index):
        """Handle dragging unified corner anchor to scale all selected objects."""
        bounds = self._get_selection_bounds()
        if bounds is None:
            return
        
        old_min_x, old_min_y, old_max_x, old_max_y = bounds
        old_width = old_max_x - old_min_x
        old_height = old_max_y - old_min_y
        
        if old_width < 1e-6 or old_height < 1e-6:
            return
        
        # Calculate new bounds based on which corner is being dragged
        # corner_index: 0=(min,min), 1=(max,min), 2=(min,max), 3=(max,max)
        if corner_index == 0:  # Top-left
            new_min_x, new_min_y = wx, wy
            new_max_x, new_max_y = old_max_x, old_max_y
        elif corner_index == 1:  # Top-right
            new_min_x, new_min_y = old_min_x, wy
            new_max_x, new_max_y = wx, old_max_y
        elif corner_index == 2:  # Bottom-left
            new_min_x, new_min_y = wx, old_min_y
            new_max_x, new_max_y = old_max_x, wy
        else:  # Bottom-right (3)
            new_min_x, new_min_y = old_min_x, old_min_y
            new_max_x, new_max_y = wx, wy
        
        new_width = new_max_x - new_min_x
        new_height = new_max_y - new_min_y
        
        if new_width < 10 or new_height < 10:
            return  # Minimum size
        
        scale_x = new_width / old_width
        scale_y = new_height / old_height
        
        # Scale all selected objects relative to the fixed corner
        for obj in self.selected_objects:
            item = obj["data"]
            
            if "Start" in item:
                sx = float(item["Start"].get("X", 0))
                sy = float(item["Start"].get("Y", 0))
                # Scale relative to fixed corner
                if corner_index in (0, 2):  # Left corners fixed on right
                    item["Start"]["X"] = old_max_x - (old_max_x - sx) * scale_x
                else:  # Right corners fixed on left
                    item["Start"]["X"] = old_min_x + (sx - old_min_x) * scale_x
                if corner_index in (0, 1):  # Top corners fixed on bottom
                    item["Start"]["Y"] = old_max_y - (old_max_y - sy) * scale_y
                else:  # Bottom corners fixed on top
                    item["Start"]["Y"] = old_min_y + (sy - old_min_y) * scale_y
            
            if "End" in item:
                ex = float(item["End"].get("X", 0))
                ey = float(item["End"].get("Y", 0))
                if corner_index in (0, 2):
                    item["End"]["X"] = old_max_x - (old_max_x - ex) * scale_x
                else:
                    item["End"]["X"] = old_min_x + (ex - old_min_x) * scale_x
                if corner_index in (0, 1):
                    item["End"]["Y"] = old_max_y - (old_max_y - ey) * scale_y
                else:
                    item["End"]["Y"] = old_min_y + (ey - old_min_y) * scale_y
        
        self.rebuild_canvas(preserve_selection=True)

    def _move_selected_objects(self, dx, dy):
        """Move all selected objects by delta."""
        handled_items = set()
        
        # Collect all selected item data first
        selected_item_ids = {id(obj["data"]) for obj in self.selected_objects}
        
        # Calculate pixel delta once
        width, height = self.current_canvas_size()
        wx0, wy0, wx1, wy1 = self.world_bbox
        dx_pix = dx / (wx1 - wx0) * width
        dy_pix = -dy / (wy1 - wy0) * height
        
        for obj in self.selected_objects:
            item = obj["data"]
            if id(item) in handled_items:
                continue
            handled_items.add(id(item))
            
            item_type = item.get("Type")
            
            if "Start" in item:
                start = item["Start"]
                start["X"] = float(start.get("X", 0)) + dx
                start["Y"] = float(start.get("Y", 0)) + dy
            
            if "End" in item:
                end = item["End"]
                end["X"] = float(end.get("X", 0)) + dx
                end["Y"] = float(end.get("Y", 0)) + dy
            
            # Sync floor/ceiling partner only if partner is NOT already selected
            # (if both are selected, they'll each be moved individually)
            if self.lock_floor_ceiling.get() and item_type in ("Floor", "Ceiling"):
                partner = self.floor_ceiling_links.get(id(item))
                if partner and id(partner) not in selected_item_ids:
                    # Partner not selected, sync it
                    self.sync_floor_ceiling_partner(item)
                    handled_items.add(id(partner))
                    # Also move partner's visual representation
                    for pobj in self.objects:
                        if pobj["data"] is partner:
                            for cid in pobj["canvas_ids"]:
                                self.canvas_helper.safe_move(cid, dx_pix, dy_pix)
                            break
            
            # Move canvas items visually
            for cid in obj["canvas_ids"]:
                self.canvas_helper.safe_move(cid, dx_pix, dy_pix)
        
        # Also move unified selection anchors if they exist
        for cid in getattr(self, '_unified_anchor_ids', []):
            self.canvas_helper.safe_move(cid, dx_pix, dy_pix)

    def on_release(self, event):
        """Handle mouse release."""
        sx, sy = event.x, event.y
        wx, wy = self.screen_to_world(sx, sy)
        
        mode = self.current_mode.get()

        # Finalize pending line if dragged
        if self.pending_line:
            if self.pending_line.get("dragged"):
                if self.snap_enabled.get():
                    wx, wy = self.snap_point_to_grid(wx, wy)
                start_x = self.pending_line["start_x"]
                start_y = self.pending_line["start_y"]
                if abs(wx - start_x) > 1 or abs(wy - start_y) > 1:
                    self._complete_pending_object(wx, wy, mode)
                self.clear_pending_line()
            self.dragging = False
            return

        # Complete box selection
        if self.box_selecting:
            self._finish_box_selection(event)
            return

        # End anchor drag
        if self.dragging_anchor is not None:
            self.dragging_anchor = None
            self.dragging = False
            self.drag_saved_state = False
            # Refresh unified anchors after drag (position may have changed)
            self.draw_selection_anchors()
            self.update_properties_panel()
            return

        # End object drag
        if self.dragging and self.selected_objects:
            # Snap objects to grid after drag
            any_snapped = False
            for obj in self.selected_objects:
                if self._snap_object(obj["data"]):
                    any_snapped = True
            if any_snapped:
                self.rebuild_canvas(preserve_selection=True)
            else:
                # Refresh unified anchors to correct positions
                self.draw_selection_anchors()
        self.dragging = False
        self.drag_saved_state = False

    def _finish_box_selection(self, event):
        """Complete box selection."""
        if self.box_select_rect_id is not None:
            self.canvas.delete(self.box_select_rect_id)
            self.box_select_rect_id = None
        
        if self.box_select_start is None:
            self.box_selecting = False
            return
        
        bsx, bsy = self.box_select_start
        x0, x1 = min(bsx, event.x), max(bsx, event.x)
        y0, y1 = min(bsy, event.y), max(bsy, event.y)
        
        self.box_selecting = False
        self.box_select_start = None
        
        # Select objects overlapping the box
        for obj in self.objects:
            if obj in self.selected_objects:
                continue
            
            for cid in obj["canvas_ids"]:
                if cid in obj.get("anchors", []):
                    continue
                
                try:
                    bbox = self.canvas.bbox(cid)
                    if bbox is None:
                        continue
                    
                    ix0, iy0, ix1, iy1 = bbox
                    
                    # Check overlap
                    if not (ix1 < x0 or ix0 > x1 or iy1 < y0 or iy0 > y1):
                        group = self._get_linked_selection_group(obj)
                        for member in group:
                            if member not in self.selected_objects:
                                self.selected_objects.append(member)
                                self.style_object(member, selected=True)
                        if self.selected_obj is None:
                            self.selected_obj = obj
                        break
                except tk.TclError:
                    continue
        
        # Update anchor visibility for single vs multi-selection
        self.draw_selection_anchors()
        self.update_properties_panel()

    def _snap_object(self, item):
        """Snap an object's coordinates to grid."""
        if not self.snap_enabled.get():
            return False
        
        changed = False
        grid = self.get_grid_size()
        
        if "Start" in item:
            start = item["Start"]
            old_x = float(start.get("X", 0))
            old_y = float(start.get("Y", 0))
            new_x = round(old_x / grid) * grid
            new_y = round(old_y / grid) * grid
            if abs(new_x - old_x) > 0.01 or abs(new_y - old_y) > 0.01:
                start["X"] = new_x
                start["Y"] = new_y
                changed = True
        
        if "End" in item:
            end = item["End"]
            old_x = float(end.get("X", 0))
            old_y = float(end.get("Y", 0))
            new_x = round(old_x / grid) * grid
            new_y = round(old_y / grid) * grid
            if abs(new_x - old_x) > 0.01 or abs(new_y - old_y) > 0.01:
                end["X"] = new_x
                end["Y"] = new_y
                changed = True
        
        return changed

    def on_delete(self, event=None):
        """Delete selected objects."""
        if not self.selected_objects:
            return
        count = self.state.delete_selected()
        if count > 0:
            self._clear_unified_selection_anchors()
            self.rebuild_canvas()

    def on_copy(self, event=None):
        """Copy selected objects to clipboard."""
        self.state.copy_selected()

    def on_paste(self, event=None):
        """Paste objects from clipboard."""
        new_items = self.state.paste(offset=(50.0, 50.0))
        if not new_items:
            return
        self.rebuild_canvas()
        # Select pasted items
        for obj in self.objects:
            if obj["data"] in new_items:
                self.add_to_selection(obj)

    def on_select_all(self, event=None):
        """Select all objects."""
        self.select_all()

    def on_key_escape(self, event=None):
        """Cancel current operation."""
        self.cancel_transient_actions()
        self.current_mode.set("select")

    # =========================================================================
    # Object Creation
    # =========================================================================

    def _complete_pending_object(self, wx, wy, mode):
        """Complete a pending line/rect object."""
        if not self.pending_line:
            return

        start_x = self.pending_line["start_x"]
        start_y = self.pending_line["start_y"]

        self.push_undo()

        if mode == "add_floor_ceiling":
            # Special case: creates a pair
            floor, ceiling = create_floor_ceiling_pair(start_x, start_y, wx, wy)
            self.data.append(floor)
            self.data.append(ceiling)
            self.floor_ceiling_links[id(floor)] = ceiling
            self.floor_ceiling_links[id(ceiling)] = floor
        else:
            # Try line-based factory
            obj = create_line_object(mode, start_x, start_y, wx, wy)
            if obj is None:
                # Try rect-based factory
                obj = create_rect_object(mode, start_x, start_y, wx, wy)
            if obj:
                self.data.append(obj)

        self.clear_pending_line()
        self.rebuild_canvas()

    def _create_point_object(self, wx, wy, mode):
        """Create a point-based object."""
        self.push_undo()

        obj = create_point_object(mode, wx, wy)
        if obj:
            self.data.append(obj)

        self.rebuild_canvas()

    # =========================================================================
    # Movement & Rotation
    # =========================================================================

    def _move_selection(self, dx, dy):
        """Move all selected objects by delta."""
        for obj in self.selected_objects:
            item = obj["data"]
            if "Start" in item:
                item["Start"]["X"] = item["Start"].get("X", 0) + dx
                item["Start"]["Y"] = item["Start"].get("Y", 0) + dy
            if "End" in item:
                item["End"]["X"] = item["End"].get("X", 0) + dx
                item["End"]["Y"] = item["End"].get("Y", 0) + dy
            if self.lock_floor_ceiling.get() and item.get("Type") in ("Floor", "Ceiling"):
                self.sync_floor_ceiling_partner(item)
        self.rebuild_canvas(preserve_selection=True)

    def rotate_selection(self, degrees):
        """Rotate selected objects by degrees around their common center."""
        if not self.selected_objects:
            return

        self.push_undo()

        # Get common center
        all_x = []
        all_y = []
        for obj in self.selected_objects:
            item = obj["data"]
            type_def = TYPE_MAP.get(item.get("Type"))
            if type_def:
                center = type_def.get_center(item)
                if center:
                    all_x.append(center[0])
                    all_y.append(center[1])

        if not all_x:
            return

        center_x = sum(all_x) / len(all_x)
        center_y = sum(all_y) / len(all_y)

        # Rotate each object
        for obj in self.selected_objects:
            item = obj["data"]
            type_def = TYPE_MAP.get(item.get("Type"))
            if type_def and hasattr(type_def, 'rotate'):
                type_def.rotate(item, degrees, (center_x, center_y))
            if self.lock_floor_ceiling.get() and item.get("Type") in ("Floor", "Ceiling"):
                self.sync_floor_ceiling_partner(item)

        self.rebuild_canvas(preserve_selection=True)

    # =========================================================================
    # Undo/Redo
    # =========================================================================

    def push_undo(self):
        """Push current state to undo stack."""
        self.state.save_state()

    def undo(self, event=None):
        """Undo last action."""
        if self.state.undo():
            if self.root_json:
                self.root_json["Elements"] = self.data
            self._clear_unified_selection_anchors()
            self.rebuild_canvas()

    def redo(self, event=None):
        """Redo last undone action."""
        if self.state.redo():
            if self.root_json:
                self.root_json["Elements"] = self.data
            self._clear_unified_selection_anchors()
            self.rebuild_canvas()


# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    """Main entry point."""
    root = tk.Tk()
    root.geometry("1600x900")
    app = FloorplanEditor(root)
    root.mainloop()


if __name__ == "__main__":
    main()
