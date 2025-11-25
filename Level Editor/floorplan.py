import json
import math
import os
import tkinter as tk
from tkinter import filedialog, messagebox


class FloorplanEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Floorplan Editor")

        self.canvas_width = 1200
        self.canvas_height = 800

        # root_json: None if top-level is a list, dict if we have {"Elements": [...]}
        self.root_json = None
        self.current_file_path = None
        self.data = []  # this is always the list of elements we edit

        self.objects = []      # list of { "data": dict, "canvas_ids": [int, ...] }
        self.id_to_obj = {}    # canvas_id -> object
        self.anchor_meta = {}  # canvas_id -> anchor metadata for resizing/moving
        self.floor_ceiling_links = {}  # id(item) -> partner item (in-memory only)
        
        self.selected_objects = []  # list of selected objects for multi-select
        self.clipboard = []    # clipboard for copy/paste
        
        # Undo/Redo history
        self.undo_stack = []
        self.redo_stack = []
        self.max_undo = 50

        # default world bbox if no Floor found
        self.world_bbox = (-1600.0, -900.0, 1600.0, 900.0)
        self.min_zoom_span = 50.0
        self.max_zoom_span = 50000.0

        self.pan_active = False
        self.pan_last_sx = 0
        self.pan_last_sy = 0

        self.show_grid = tk.BooleanVar(value=True)
        self.snap_to_grid = tk.BooleanVar(value=True)
        self.grid_size = tk.DoubleVar(value=100.0)
        self.show_lamps = tk.BooleanVar(value=True)  # for CeilingLight visualization
        self.lock_floor_ceiling = tk.BooleanVar(value=True)  # Lock Floor/Ceiling from accidental editing
        # Cubicle display dimensions (UI only, not saved to JSON)
        self.cubicle_display_width = tk.DoubleVar(value=300.0)
        self.cubicle_display_depth = tk.DoubleVar(value=250.0)
        self.grid_size.trace_add("write", self.on_grid_setting_changed)
        self.cubicle_display_width.trace_add("write", self.on_cubicle_display_changed)
        self.cubicle_display_depth.trace_add("write", self.on_cubicle_display_changed)
        self.show_grid.trace_add("write", self.on_grid_setting_changed)
        self.canvas_grid_ids = []

        self.pending_line = None  # dict with start/end data while drawing
        self.pending_preview_id = None

        self.selected_obj = None
        self.dragging = False
        self.drag_start_sx = 0
        self.drag_start_sy = 0
        self.drag_saved_state = False  # Track if state was saved for this drag
        self.dragging_anchor = None    # metadata for active anchor drag
        self.box_selecting = False     # True when doing box selection
        self.box_select_start = None   # (sx, sy) screen coords of box start
        self.box_select_rect_id = None # Canvas ID of selection rectangle

        self.mode = tk.StringVar(value="select")
        self.mode.trace_add("write", self.on_mode_changed)

        self._build_ui()

    def _build_ui(self):
        # Modern color scheme
        bg_color = "#f5f5f5"
        toolbar_bg = "#ffffff"
        accent_color = "#0078d7"

        self.root.configure(bg=bg_color)

        # Main toolbar with modern styling
        toolbar = tk.Frame(self.root, bg=toolbar_bg, relief=tk.FLAT, bd=1)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=2, pady=2)

        toolbar_top = tk.Frame(toolbar, bg=toolbar_bg)
        toolbar_top.pack(side=tk.TOP, fill=tk.X)
        toolbar_bottom = tk.Frame(toolbar, bg=toolbar_bg)
        toolbar_bottom.pack(side=tk.TOP, fill=tk.X)

        # Mode selection frame
        mode_frame = tk.LabelFrame(toolbar_top, text="Mode", padx=8, pady=4, bg=toolbar_bg,
                                   relief=tk.GROOVE, bd=1)
        mode_frame.pack(side=tk.LEFT, padx=5, pady=4)

        for text, value in [("Select", "select"), ("Cubicle", "add_cubicle"),
                            ("Wall", "add_wall"), ("Door", "add_door"),
                            ("Window", "add_window"), ("Elevator", "add_elevator"),
                            ("Spawn", "add_spawn"), ("RoomTone", "add_roomtone"),
                            ("Floor+Ceiling", "add_floor_ceiling")]:
            tk.Radiobutton(
                mode_frame,
                text=text,
                variable=self.mode,
                value=value,
                bg=toolbar_bg,
                activebackground=toolbar_bg,
                selectcolor=toolbar_bg,
            ).pack(side=tk.LEFT, padx=2)

        # View controls frame
        view_frame = tk.LabelFrame(toolbar_top, text="View", padx=8, pady=4, bg=toolbar_bg,
                                   relief=tk.GROOVE, bd=1)
        view_frame.pack(side=tk.LEFT, padx=5, pady=4)

        for text, cmd in [("Reset", self.reset_view), ("Zoom +", self.zoom_in),
                          ("Zoom −", self.zoom_out)]:
            tk.Button(view_frame, text=text, command=cmd, bg=toolbar_bg,
                      activebackground=accent_color, relief=tk.RAISED, bd=1,
                      padx=8, pady=2).pack(side=tk.LEFT, padx=2)

        # Rotation controls frame
        rotate_frame = tk.LabelFrame(toolbar_top, text="Rotate", padx=8, pady=4, bg=toolbar_bg,
                                     relief=tk.GROOVE, bd=1)
        rotate_frame.pack(side=tk.LEFT, padx=5, pady=4)

        for text, deg in [("⟳ 90°", 90), ("⟲ 90°", -90)]:
            tk.Button(rotate_frame, text=text, command=lambda d=deg: self.rotate_selection(d),
                      bg=toolbar_bg, activebackground=accent_color, relief=tk.RAISED, bd=1,
                      padx=8, pady=2).pack(side=tk.LEFT, padx=2)

        # Grid controls frame
        grid_frame = tk.LabelFrame(toolbar_bottom, text="Grid", padx=8, pady=4, bg=toolbar_bg,
                                   relief=tk.GROOVE, bd=1)
        grid_frame.pack(side=tk.LEFT, padx=5, pady=4)

        tk.Checkbutton(
            grid_frame,
            text="Snap",
            variable=self.snap_to_grid,
            bg=toolbar_bg,
            activebackground=toolbar_bg,
            selectcolor=toolbar_bg,
        ).pack(side=tk.LEFT, padx=2)
        tk.Label(grid_frame, text="Size:", bg=toolbar_bg).pack(side=tk.LEFT, padx=(5, 2))
        tk.Spinbox(grid_frame, from_=10, to=2000, increment=10,
                   width=6, textvariable=self.grid_size,
                   command=self.on_grid_setting_changed).pack(side=tk.LEFT, padx=2)
        tk.Checkbutton(
            grid_frame,
            text="Show",
            variable=self.show_grid,
            bg=toolbar_bg,
            activebackground=toolbar_bg,
            selectcolor=toolbar_bg,
        ).pack(side=tk.LEFT, padx=(5, 2))

        # Display options frame
        display_frame = tk.LabelFrame(toolbar_bottom, text="Display", padx=8, pady=4, bg=toolbar_bg,
                                      relief=tk.GROOVE, bd=1)
        display_frame.pack(side=tk.LEFT, padx=5, pady=4)

        tk.Checkbutton(
            display_frame,
            text="Lamps",
            variable=self.show_lamps,
            command=lambda: self.rebuild_canvas(preserve_selection=True),
            bg=toolbar_bg,
            activebackground=toolbar_bg,
            selectcolor=toolbar_bg,
        ).pack(side=tk.LEFT, padx=2)
        tk.Checkbutton(
            display_frame,
            text="Link Floor/Ceiling",
            variable=self.lock_floor_ceiling,
            command=self.on_floor_ceiling_link_changed,
            bg=toolbar_bg,
            activebackground=toolbar_bg,
            selectcolor=toolbar_bg,
        ).pack(side=tk.LEFT, padx=2)

        # Cubicle display size controls
        tk.Label(display_frame, text="Cubicle W:", bg=toolbar_bg).pack(side=tk.LEFT, padx=(8, 2))
        tk.Spinbox(display_frame, from_=50, to=1000, increment=10,
                   width=7, justify="right", textvariable=self.cubicle_display_width).pack(side=tk.LEFT, padx=2)
        tk.Label(display_frame, text="D:", bg=toolbar_bg).pack(side=tk.LEFT, padx=(2, 2))
        tk.Spinbox(display_frame, from_=50, to=1000, increment=10,
                   width=7, justify="right", textvariable=self.cubicle_display_depth).pack(side=tk.LEFT, padx=2)

        # Properties panel on the right with modern styling
        self.props_frame = tk.Frame(self.root, width=280, relief=tk.FLAT, bd=1, bg="#fafafa")
        self.props_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=0, pady=0)
        self.props_frame.pack_propagate(False)

        props_header = tk.Frame(self.props_frame, bg="#0078d7", height=35)
        props_header.pack(fill=tk.X)
        props_title = tk.Label(props_header, text="Properties", font=("Segoe UI", 11, "bold"),
                               bg="#0078d7", fg="white")
        props_title.pack(pady=8)

        # Scrollable properties area
        self.props_canvas = tk.Canvas(self.props_frame, highlightthickness=0, bg="#fafafa")
        props_scrollbar = tk.Scrollbar(self.props_frame, orient="vertical", command=self.props_canvas.yview)
        self.props_inner = tk.Frame(self.props_canvas, bg="#fafafa")

        self.props_canvas.configure(yscrollcommand=props_scrollbar.set)
        props_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.props_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.props_canvas_window = self.props_canvas.create_window((0, 0), window=self.props_inner, anchor="nw")
        self.props_inner.bind("<Configure>", lambda e: self.props_canvas.configure(scrollregion=self.props_canvas.bbox("all")))

        # Status bar with modern styling
        status_frame = tk.Frame(self.root, relief=tk.FLAT, bd=1, bg="#e1e1e1", height=28)
        status_frame.pack(side=tk.BOTTOM, fill=tk.X)
        self.status_label = tk.Label(
            status_frame,
            text="Pan: Right/Middle drag | Zoom: Wheel | Undo: Ctrl+Z | Redo: Ctrl+Y | Copy/Paste: Ctrl+C/V",
            anchor=tk.W,
            bg="#e1e1e1",
            fg="#333333",
            font=("Segoe UI", 9),
        )
        self.status_label.pack(side=tk.LEFT, padx=8, pady=4)

        self.canvas = tk.Canvas(
            self.root,
            width=self.canvas_width,
            height=self.canvas_height,
            bg="#ffffff",
            highlightthickness=0,
        )
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

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

        self.root.bind("<Delete>", self.on_delete)
        self.root.bind("<BackSpace>", self.on_delete)
        self.root.bind("<Escape>", lambda e: self.cancel_transient_actions())
        self.root.bind("<r>", lambda e: self.rotate_selection(90))
        self.root.bind("<R>", lambda e: self.rotate_selection(-90))
        self.root.bind("<Control-c>", self.on_copy)
        self.root.bind("<Control-v>", self.on_paste)
        self.root.bind("<Control-a>", self.on_select_all)
        self.root.bind("<Control-z>", self.on_undo)

        menubar = tk.Menu(self.root)
        filemenu = tk.Menu(menubar, tearoff=0)
        filemenu.add_command(label="Open...", command=self.open_file)
        filemenu.add_command(label="Reload", command=self.reload_file)
        filemenu.add_command(label="Save As...", command=self.save_file_as)
        menubar.add_cascade(label="File", menu=filemenu)
        self.root.config(menu=menubar)

    # ---------- Coordinate transforms ----------

    def current_canvas_size(self):
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        if width <= 1 or height <= 1:
            width = self.canvas_width
            height = self.canvas_height
        else:
            self.canvas_width = width
            self.canvas_height = height
        return width, height

    def world_to_screen(self, x, y):
        wx0, wy0, wx1, wy1 = self.world_bbox
        if wx1 == wx0:
            wx1 = wx0 + 1
        if wy1 == wy0:
            wy1 = wy0 + 1
        width, height = self.current_canvas_size()
        sx = (x - wx0) / (wx1 - wx0) * width
        sy = height - (y - wy0) / (wy1 - wy0) * height
        return sx, sy

    def screen_to_world(self, sx, sy):
        wx0, wy0, wx1, wy1 = self.world_bbox
        width, height = self.current_canvas_size()
        x = wx0 + sx / width * (wx1 - wx0)
        y = wy0 + (height - sy) / height * (wy1 - wy0)
        return x, y

    # ---------- Grid & snapping ----------
    
    def on_cubicle_display_changed(self, *args):
        """Rebuild canvas when cubicle display dimensions change"""
        self.rebuild_canvas()

    def get_grid_size(self):
        try:
            value = float(self.grid_size.get())
        except (tk.TclError, ValueError):
            value = 100.0
        if value <= 0:
            value = 100.0
        return value

    def snap_value(self, value):
        if not self.snap_to_grid.get():
            return float(value)
        grid = self.get_grid_size()
        return round(float(value) / grid) * grid

    def snap_point(self, x, y):
        if not self.snap_to_grid.get():
            return float(x), float(y)
        return self.snap_value(x), self.snap_value(y)

    def on_grid_setting_changed(self, *_):
        if hasattr(self, "canvas"):
            self.rebuild_canvas(preserve_selection=True)

    def on_floor_ceiling_link_changed(self, *_):
        if not hasattr(self, "canvas"):
            return

        if self.lock_floor_ceiling.get():
            self.ensure_floor_ceiling_pairs()

        self.rebuild_canvas(preserve_selection=True)

    def normalize_yaw(self, yaw):
        try:
            angle = float(yaw)
        except (TypeError, ValueError):
            angle = 0.0
        steps = int(round(angle / 90.0)) % 4
        return float(steps * 90)

    def get_axis_size(self, dimensions, yaw):
        w = float(dimensions.get("X", 0.0))
        h = float(dimensions.get("Y", 0.0))
        steps = int(round(yaw / 90.0)) % 4
        if steps % 2 == 0:
            return abs(w), abs(h)
        return abs(h), abs(w)

    # ---------- View controls ----------

    def on_canvas_configure(self, event):
        new_width = max(event.width, 1)
        new_height = max(event.height, 1)
        resized = (new_width != self.canvas_width) or (new_height != self.canvas_height)
        self.canvas_width = new_width
        self.canvas_height = new_height
        if resized and self.data:
            self.rebuild_canvas(preserve_selection=True)

    def on_mousewheel(self, event, wheel_delta=None):
        delta = wheel_delta if wheel_delta is not None else event.delta
        if delta == 0:
            return
        direction = 1 if delta > 0 else -1
        base_scale = 0.9 if direction > 0 else 1.1
        if wheel_delta is None:
            steps = max(1, int(abs(delta) / 120))
        else:
            steps = max(1, int(abs(wheel_delta)))
        scale = base_scale ** steps
        self.zoom_at(scale, event.x, event.y)

    def zoom_in(self):
        width, height = self.current_canvas_size()
        self.zoom_at(0.9, width / 2, height / 2)

    def zoom_out(self):
        width, height = self.current_canvas_size()
        self.zoom_at(1.1, width / 2, height / 2)

    def zoom_at(self, scale, center_sx, center_sy):
        if scale <= 0:
            return
        wx0, wy0, wx1, wy1 = self.world_bbox
        width = wx1 - wx0
        height = wy1 - wy0
        if width <= 0 or height <= 0:
            return

        min_span = self.min_zoom_span
        max_span = self.max_zoom_span
        min_scale = max(min_span / width, min_span / height)
        max_scale = min(max_span / width, max_span / height)
        min_scale = max(min_scale, 1e-6)
        if max_scale < min_scale:
            max_scale = min_scale

        new_scale = scale
        if new_scale < min_scale:
            new_scale = min_scale
        if new_scale > max_scale:
            new_scale = max_scale

        if abs(new_scale - 1.0) < 1e-6:
            return

        cx, cy = self.screen_to_world(center_sx, center_sy)

        new_width = width * new_scale
        new_height = height * new_scale

        if width != 0:
            cx_ratio = (cx - wx0) / width
        else:
            cx_ratio = 0.5
        if height != 0:
            cy_ratio = (cy - wy0) / height
        else:
            cy_ratio = 0.5

        new_wx0 = cx - cx_ratio * new_width
        new_wx1 = new_wx0 + new_width
        new_wy0 = cy - cy_ratio * new_height
        new_wy1 = new_wy0 + new_height

        self.world_bbox = (new_wx0, new_wy0, new_wx1, new_wy1)
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
        dx_pix = event.x - self.pan_last_sx
        dy_pix = event.y - self.pan_last_sy
        if dx_pix == 0 and dy_pix == 0:
            return

        before_x, before_y = self.screen_to_world(self.pan_last_sx, self.pan_last_sy)
        after_x, after_y = self.screen_to_world(event.x, event.y)
        dx_world = before_x - after_x
        dy_world = before_y - after_y

        self.canvas.move("all", dx_pix, dy_pix)

        wx0, wy0, wx1, wy1 = self.world_bbox
        self.world_bbox = (wx0 + dx_world, wy0 + dy_world,
                           wx1 + dx_world, wy1 + dy_world)

        self.pan_last_sx = event.x
        self.pan_last_sy = event.y

    def on_pan_end(self, event):
        if not self.pan_active:
            return
        self.pan_active = False
        self.canvas.configure(cursor="")

    def reset_view(self):
        self.update_world_bbox_from_floor()
        self.rebuild_canvas(preserve_selection=True)

    # ---------- File I/O ----------

    def open_file(self):
        path = filedialog.askopenfilename(
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not path:
            return
        self.load_file(path)

    def reload_file(self):
        if not self.current_file_path:
            messagebox.showinfo("Info", "Open a layout first to reload it.")
            return
        self.load_file(self.current_file_path)

    def load_file(self, path):
        try:
            with open(path, "r") as f:
                root = json.load(f)

            # handle both:
            #   [ {...}, {...} ]
            #   { "Elements": [ {...}, {...} ], ... }
            if isinstance(root, dict) and "Elements" in root:
                self.root_json = root
                elements = root["Elements"]
                if not isinstance(elements, list):
                    raise ValueError('"Elements" must be a list')
                self.data = elements
            elif isinstance(root, list):
                self.root_json = None
                self.data = root
            else:
                raise ValueError("Expected a JSON array or an object with 'Elements'")

            self.current_file_path = path
            self.reset_view()
            display_name = os.path.basename(path)
            self.root.title(f"Floorplan Editor - {display_name}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to open file:\n{e}")

    def save_file_as(self):
        if not self.data:
            messagebox.showinfo("Info", "Nothing to save.")
            return
        path = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if not path:
            return
        try:
            if self.root_json is not None:
                out = dict(self.root_json)
                out["Elements"] = self.data
            else:
                out = self.data

            with open(path, "w") as f:
                json.dump(out, f, indent=2)
            self.current_file_path = path
            display_name = os.path.basename(path)
            self.root.title(f"Floorplan Editor - {display_name}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save file:\n{e}")

    def update_world_bbox_from_floor(self):
        for item in self.data:
            if item.get("Type") == "Floor":
                s = item.get("Start", {})
                e = item.get("End", {})
                x0, y0 = float(s.get("X", -1600.0)), float(s.get("Y", -900.0))
                x1, y1 = float(e.get("X", 1600.0)), float(e.get("Y", 900.0))
                self.world_bbox = (x0, y0, x1, y1)
                return

        bbox = self.compute_data_bbox()
        if bbox is not None:
            self.world_bbox = bbox
        else:
            self.world_bbox = (-1600.0, -900.0, 1600.0, 900.0)

    def compute_data_bbox(self, padding_ratio=0.1, padding_absolute=100.0):
        min_x = float("inf")
        min_y = float("inf")
        max_x = float("-inf")
        max_y = float("-inf")

        for item in self.data:
            coords = []
            t = item.get("Type")
            if "Start" in item:
                s = item.get("Start", {})
                coords.append((float(s.get("X", 0.0)), float(s.get("Y", 0.0))))
            if "End" in item:
                e = item.get("End", {})
                coords.append((float(e.get("X", 0.0)), float(e.get("Y", 0.0))))

            if t == "Cubicle":
                start = item.get("Start", {})
                yaw = float(item.get("Yaw", 0.0))
                # Use display dimensions for bounding box
                w = float(self.cubicle_display_width.get())
                h = float(self.cubicle_display_depth.get())
                # cubicles are axis-aligned, optionally rotated 90 degrees
                if abs(yaw) % 180 == 90:
                    w, h = h, w
                x0 = float(start.get("X", 0.0))
                y0 = float(start.get("Y", 0.0))
                coords.extend([(x0 + w, y0), (x0, y0 + h), (x0 + w, y0 + h)])

            for x, y in coords:
                if x < min_x:
                    min_x = x
                if y < min_y:
                    min_y = y
                if x > max_x:
                    max_x = x
                if y > max_y:
                    max_y = y

        if min_x == float("inf") or max_x == float("-inf"):
            return None

        if max_x == min_x:
            pad_x = padding_absolute
        else:
            pad_x = max((max_x - min_x) * padding_ratio, padding_absolute)

        if max_y == min_y:
            pad_y = padding_absolute
        else:
            pad_y = max((max_y - min_y) * padding_ratio, padding_absolute)

        return (min_x - pad_x, min_y - pad_y, max_x + pad_x, max_y + pad_y)

    # ---------- Drawing ----------

    def clear_pending_line(self):
        if not hasattr(self, "canvas"):
            self.pending_line = None
            self.pending_preview_id = None
            return
        if self.pending_line:
            preview_id = self.pending_line.get("preview_id")
            start_marker_id = self.pending_line.get("start_marker_id")
            for cid in (preview_id, start_marker_id):
                if cid is not None:
                    try:
                        self.canvas.delete(cid)
                    except tk.TclError:
                        pass
        self.pending_line = None
        self.pending_preview_id = None

    def on_mode_changed(self, *_):
        mode = self.mode.get()
        if self.pending_line:
            pending_mode = self.pending_line.get("mode")
            if pending_mode != mode or mode not in ("add_wall", "add_door", "add_window", "add_elevator"):
                self.clear_pending_line()

    def draw_background_grid(self):
        if not self.show_grid.get():
            return
        grid = self.get_grid_size()
        if grid <= 0:
            return

        wx0, wy0, wx1, wy1 = self.world_bbox
        if wx0 > wx1:
            wx0, wx1 = wx1, wx0
        if wy0 > wy1:
            wy0, wy1 = wy1, wy0

        start_x = math.floor(wx0 / grid)
        end_x = math.ceil(wx1 / grid)
        start_y = math.floor(wy0 / grid)
        end_y = math.ceil(wy1 / grid)

        for i in range(start_x, end_x + 1):
            x = i * grid
            sx0, sy0 = self.world_to_screen(x, wy0)
            sx1, sy1 = self.world_to_screen(x, wy1)
            is_axis = math.isclose(x, 0.0, abs_tol=1e-6)
            color = "#d0d0d0" if is_axis else "#eeeeee"
            width = 2 if is_axis else 1
            line = self.canvas.create_line(sx0, sy0, sx1, sy1,
                                           fill=color, width=width,
                                           tags=("grid",))
            self.canvas_grid_ids.append(line)

        for j in range(start_y, end_y + 1):
            y = j * grid
            sx0, sy0 = self.world_to_screen(wx0, y)
            sx1, sy1 = self.world_to_screen(wx1, y)
            is_axis = math.isclose(y, 0.0, abs_tol=1e-6)
            color = "#d0d0d0" if is_axis else "#eeeeee"
            width = 2 if is_axis else 1
            line = self.canvas.create_line(sx0, sy0, sx1, sy1,
                                           fill=color, width=width,
                                           tags=("grid",))
            self.canvas_grid_ids.append(line)

        self.canvas.tag_lower("grid")

    def create_anchor_marker(self, wx, wy, color="#ff8844", size=5, state="hidden", tags=("anchor",), meta=None):
        sx, sy = self.world_to_screen(wx, wy)
        cid = self.canvas.create_rectangle(sx - size, sy - size,
                                           sx + size, sy + size,
                                           outline="", fill=color,
                                           tags=tags)
        if state != "normal":
            self.canvas.itemconfigure(cid, state=state)
        if meta is not None:
            self.anchor_meta[cid] = dict(meta)
        return cid

    def cancel_transient_actions(self):
        self.clear_pending_line()
        if self.pan_active:
            self.pan_active = False
            if hasattr(self, "canvas"):
                self.canvas.configure(cursor="")
        self.dragging = False
        self.dragging_anchor = None
        # Cancel box selection if active
        if self.box_selecting:
            if self.box_select_rect_id is not None:
                try:
                    self.canvas.delete(self.box_select_rect_id)
                except tk.TclError:
                    pass
                self.box_select_rect_id = None
            self.box_selecting = False
            self.box_select_start = None

    def rebuild_canvas(self, preserve_selection=False):
        self.clear_pending_line()
        selected_data_list = [obj["data"] for obj in self.selected_objects] if preserve_selection and self.selected_objects else []
        primary_data = self.selected_obj["data"] if preserve_selection and self.selected_obj is not None else None
        if self.selected_obj is not None:
            self.style_object(self.selected_obj, selected=False)
        for obj in self.selected_objects:
            self.style_object(obj, selected=False)
        self.selected_obj = None
        self.selected_objects.clear()

        self.refresh_floor_ceiling_links()

        self.canvas.delete("all")
        self.canvas_grid_ids.clear()
        self.objects.clear()
        self.id_to_obj.clear()
        self.anchor_meta.clear()

        self.draw_background_grid()

        to_select = []
        for item in self.data:
            obj = self.draw_item(item)
            if obj is not None and item in selected_data_list:
                to_select.append(obj)

        if to_select:
            combined_selection = []
            for obj in to_select:
                if self.lock_floor_ceiling.get() and obj["data"].get("Type") in ("Floor", "Ceiling"):
                    group = self._get_linked_selection_group(obj)
                else:
                    group = [obj]
                for member in group:
                    if member not in combined_selection:
                        combined_selection.append(member)
            self.selected_objects = combined_selection
            for obj in combined_selection:
                self.style_object(obj, selected=True)

            if primary_data is not None:
                for obj in combined_selection:
                    if obj["data"] is primary_data:
                        self.selected_obj = obj
                        break

            if self.selected_obj is None and combined_selection:
                self.selected_obj = combined_selection[0]

        self.update_properties_panel()

    def draw_item(self, item):
        t = item.get("Type")
        canvas_ids = []
        anchors = []
        parts = {}
        base_styles = {}
        obj = None

        def register_part(cid, role=None, capture=True):
            if role is not None:
                parts.setdefault(role, []).append(cid)
            if capture:
                try:
                    base_styles[cid] = {
                        "fill": self.canvas.itemcget(cid, "fill"),
                        "outline": self.canvas.itemcget(cid, "outline"),
                    }
                except tk.TclError:
                    base_styles[cid] = {}

        if t == "Floor":
            s = item.get("Start", {})
            e = item.get("End", {})
            x0_world = float(s.get("X", -1600.0))
            y0_world = float(s.get("Y", -900.0))
            x1_world = float(e.get("X", 1600.0))
            y1_world = float(e.get("Y", 900.0))
            x0, y0 = self.world_to_screen(x0_world, y0_world)
            x1, y1 = self.world_to_screen(x1_world, y1_world)
            cid = self.canvas.create_rectangle(x0, y0, x1, y1,
                                               outline="#cccccc", fill="#f9f9f9")
            canvas_ids.append(cid)
            register_part(cid, "body")
            self.canvas.tag_lower(cid)
            floor_corners = [
                (x0_world, y0_world),
                (x1_world, y0_world),
                (x0_world, y1_world),
                (x1_world, y1_world),
            ]
            for idx, (wx, wy) in enumerate(floor_corners):
                anchor_id = self.create_anchor_marker(
                    wx,
                    wy,
                    color="#999999",
                    size=4,
                    meta={"kind": "corner", "corner": idx},
                )
                anchors.append(anchor_id)
                canvas_ids.append(anchor_id)

            center_x = (x0_world + x1_world) / 2.0
            center_y = (y0_world + y1_world) / 2.0
            center_anchor = self.create_anchor_marker(
                center_x,
                center_y,
                color="#777777",
                size=5,
                meta={"kind": "center", "center": (center_x, center_y)},
            )
            anchors.append(center_anchor)
            canvas_ids.append(center_anchor)

        elif t in ("Wall", "Window", "Door"):
            s = item.get("Start", {})
            e = item.get("End", {})
            sx_world = float(s.get("X", 0.0))
            sy_world = float(s.get("Y", 0.0))
            ex_world = float(e.get("X", 0.0))
            ey_world = float(e.get("Y", 0.0))
            x0, y0 = self.world_to_screen(sx_world, sy_world)
            x1, y1 = self.world_to_screen(ex_world, ey_world)
            thickness = float(item.get("Thickness", 1.0))
            width = max(1, thickness / 12.0)

            if t == "Wall":
                color = "#222222"
            elif t == "Door":
                color = "#2b8a45"
            else:  # Window
                color = "#1d6bd6"

            main_line = self.canvas.create_line(x0, y0, x1, y1,
                                                fill="#444444" if t == "Door" else color,
                                                width=width,
                                                capstyle=tk.ROUND)
            canvas_ids.append(main_line)
            register_part(main_line, "line")
            
            # Draw window sections if applicable
            if t == "Window":
                section_count = int(item.get("SectionCount", 1))
                if section_count > 1:
                    dx = x1 - x0
                    dy = y1 - y0
                    length = math.hypot(dx, dy)
                    if length > 1e-6:
                        # Draw perpendicular lines for window sections
                        for i in range(1, section_count):
                            ratio = i / section_count
                            sect_x = x0 + dx * ratio
                            sect_y = y0 + dy * ratio
                            # Perpendicular direction
                            perp_x = -dy / length * 8
                            perp_y = dx / length * 8
                            # Draw section divider as perpendicular line
                            sect_line = self.canvas.create_line(
                                sect_x - perp_x, sect_y - perp_y,
                                sect_x + perp_x, sect_y + perp_y,
                                fill=color, width=max(1, width))
                            canvas_ids.append(sect_line)
                            register_part(sect_line, "line")

            if t == "Door":
                dx = x1 - x0
                dy = y1 - y0
                length = math.hypot(dx, dy)
                if length > 1e-6:
                    self.canvas.itemconfigure(main_line, dash=(8, 4))
                    mx = (x0 + x1) / 2
                    my = (y0 + y1) / 2
                    ux = dx / length
                    uy = dy / length
                    door_length = min(length * 0.6, 140)
                    door_half = door_length / 2
                    sx_door = mx - ux * door_half
                    sy_door = my - uy * door_half
                    ex_door = mx + ux * door_half
                    ey_door = my + uy * door_half
                    door_line = self.canvas.create_line(sx_door, sy_door,
                                                        ex_door, ey_door,
                                                        fill=color, width=max(width, width + 1),
                                                        capstyle=tk.ROUND)
                    canvas_ids.append(door_line)
                    register_part(door_line, "line")
                    tick_length = min(door_length * 0.4, 35)
                    tick_x = mx - uy * tick_length
                    tick_y = my + ux * tick_length
                    tick_line = self.canvas.create_line(mx, my, tick_x, tick_y,
                                                        fill=color, width=max(1, width - 1),
                                                        capstyle=tk.ROUND)
                    canvas_ids.append(tick_line)
                    register_part(tick_line, "line")

            anchor_start = self.create_anchor_marker(
                sx_world,
                sy_world,
                color="#ff8c00",
                meta={"kind": "endpoint", "endpoint": "start"},
            )
            anchor_end = self.create_anchor_marker(
                ex_world,
                ey_world,
                color="#1f78d1",
                meta={"kind": "endpoint", "endpoint": "end"},
            )
            anchors.extend([anchor_start, anchor_end])
            canvas_ids.extend([anchor_start, anchor_end])

            center_world_x = (sx_world + ex_world) / 2.0
            center_world_y = (sy_world + ey_world) / 2.0
            center_anchor = self.create_anchor_marker(
                center_world_x,
                center_world_y,
                color="#777777",
                size=5,
                meta={"kind": "center", "center": (center_world_x, center_world_y)},
            )
            anchors.append(center_anchor)
            canvas_ids.append(center_anchor)

        elif t == "Elevator":
            # Draw elevator as a wall line with a cab (rectangle) behind it
            s = item.get("Start", {})
            e = item.get("End", {})
            sx_world = float(s.get("X", 0.0))
            sy_world = float(s.get("Y", 0.0))
            ex_world = float(e.get("X", 0.0))
            ey_world = float(e.get("Y", 0.0))
            x0, y0 = self.world_to_screen(sx_world, sy_world)
            x1, y1 = self.world_to_screen(ex_world, ey_world)

            wall_color = "#9b59b6"  # Purple for elevator wall
            cab_color = "#e8daef"   # Light purple for cab interior

            dx = x1 - x0
            dy = y1 - y0
            length = math.hypot(dx, dy)

            if length > 1e-6:
                # Perpendicular direction pointing "into" the cab (behind the wall)
                # The cab extends perpendicular to the wall line
                perp_x = -dy / length
                perp_y = dx / length

                # Cab depth in screen pixels (scaled based on world units)
                cab_depth_world = 250.0  # Approximate cab depth in world units
                # Convert to screen scale
                wx0, wy0, wx1, wy1 = self.world_bbox
                width, height = self.current_canvas_size()
                scale_x = width / (wx1 - wx0) if wx1 != wx0 else 1
                cab_depth_screen = cab_depth_world * scale_x

                # Calculate cab rectangle corners (behind the wall)
                cab_corners = [
                    x0, y0,
                    x1, y1,
                    x1 + perp_x * cab_depth_screen, y1 + perp_y * cab_depth_screen,
                    x0 + perp_x * cab_depth_screen, y0 + perp_y * cab_depth_screen,
                ]

                # Draw cab rectangle first (behind)
                cab_rect = self.canvas.create_polygon(
                    cab_corners,
                    fill=cab_color,
                    outline="#c39bd3",
                    width=1
                )
                canvas_ids.append(cab_rect)
                register_part(cab_rect, "cab")
                self.canvas.tag_lower(cab_rect)  # Send to back

            # Draw the elevator wall/door line (flat against the opening)
            wall_line = self.canvas.create_line(x0, y0, x1, y1,
                                                fill=wall_color,
                                                width=4,
                                                capstyle=tk.PROJECTING)
            canvas_ids.append(wall_line)
            register_part(wall_line, "line")

            # Draw door split indicator in the center (only if we have valid perpendicular)
            if length > 1e-6:
                mx = (x0 + x1) / 2
                my = (y0 + y1) / 2
                # Small perpendicular tick to indicate door split
                tick_len = 8
                perp_nx = -dy / length
                perp_ny = dx / length
                tick_line = self.canvas.create_line(
                    mx - perp_nx * tick_len, my - perp_ny * tick_len,
                    mx + perp_nx * tick_len, my + perp_ny * tick_len,
                    fill=wall_color, width=2)
                canvas_ids.append(tick_line)
                register_part(tick_line, "line")

            # Add anchors
            anchor_start = self.create_anchor_marker(
                sx_world,
                sy_world,
                color="#9b59b6",
                meta={"kind": "endpoint", "endpoint": "start"},
            )
            anchor_end = self.create_anchor_marker(
                ex_world,
                ey_world,
                color="#9b59b6",
                meta={"kind": "endpoint", "endpoint": "end"},
            )
            anchors.extend([anchor_start, anchor_end])
            canvas_ids.extend([anchor_start, anchor_end])

            center_world_x = (sx_world + ex_world) / 2.0
            center_world_y = (sy_world + ey_world) / 2.0
            center_anchor = self.create_anchor_marker(
                center_world_x,
                center_world_y,
                color="#777777",
                size=5,
                meta={"kind": "center", "center": (center_world_x, center_world_y)},
            )
            anchors.append(center_anchor)
            canvas_ids.append(center_anchor)

        elif t == "Cubicle":
            start = item.get("Start", {})
            yaw = self.normalize_yaw(float(item.get("Yaw", 0.0)))
            item["Yaw"] = yaw

            # Use display dimensions (UI-only, not saved)
            display_width = float(self.cubicle_display_width.get())
            display_depth = float(self.cubicle_display_depth.get())
            dim = {"X": display_width, "Y": display_depth}

            # Offset visualization by 90° to match in-game orientation
            vis_yaw = self.normalize_yaw(yaw + 90.0)

            w_world, h_world = self.get_axis_size(dim, vis_yaw)
            x0_world = float(start.get("X", 0.0))
            y0_world = float(start.get("Y", 0.0))
            x1_world = x0_world + w_world
            y1_world = y0_world + h_world

            x0, y0 = self.world_to_screen(x0_world, y0_world)
            x1, y1 = self.world_to_screen(x1_world, y1_world)

            cid = self.canvas.create_rectangle(x0, y0, x1, y1,
                                               outline="black",
                                               fill="#dddddd")
            canvas_ids.append(cid)
            register_part(cid, "body")
            
            # Add rotation indicator (small triangle showing front/orientation)
            indicator_size = min(abs(x1 - x0), abs(y1 - y0)) * 0.12
            
            # Calculate center for rotation indicator
            cx = (x0 + x1) / 2
            cy = (y0 + y1) / 2
            
            # Create arrow pointing in the direction of yaw
            # Yaw 0 now renders facing up to mirror gameplay orientation
            angle_rad = math.radians(-vis_yaw)  # Negative for screen coords
            arrow_len = indicator_size * 2
            
            # Arrow points from center outward
            end_x = cx + arrow_len * math.cos(angle_rad)
            end_y = cy + arrow_len * math.sin(angle_rad)
            
            # Draw arrow
            arrow_id = self.canvas.create_line(cx, cy, end_x, end_y,
                                               fill="#ff6600", width=3,
                                               arrow=tk.LAST, arrowshape=(10, 12, 5))
            canvas_ids.append(arrow_id)
            register_part(arrow_id, "arrow")

            corners = [
                (x0_world, y0_world),
                (x1_world, y0_world),
                (x0_world, y1_world),
                (x1_world, y1_world)
            ]
            for idx, (wx, wy) in enumerate(corners):
                color = "#ff8c00" if idx == 0 else ("#1f78d1" if idx == 3 else "#666666")
                anchor_id = self.create_anchor_marker(
                    wx,
                    wy,
                    color=color,
                    meta={"kind": "corner", "corner": idx},
                )
                anchors.append(anchor_id)
                canvas_ids.append(anchor_id)

            center_world_x = (x0_world + x1_world) / 2.0
            center_world_y = (y0_world + y1_world) / 2.0
            center_anchor = self.create_anchor_marker(
                center_world_x,
                center_world_y,
                color="#777777",
                size=5,
                meta={"kind": "center", "center": (center_world_x, center_world_y)},
            )
            anchors.append(center_anchor)
            canvas_ids.append(center_anchor)
            
            # Add dimension text label showing display dimensions
            center_x = (x0_world + x1_world) / 2
            center_y = (y0_world + y1_world) / 2
            cx, cy = self.world_to_screen(center_x, center_y)
            dim_text = f"{display_width:.0f}×{display_depth:.0f}"
            text_id = self.canvas.create_text(cx, cy, text=dim_text,
                                             fill="#555555", font=("Arial", 9))
            canvas_ids.append(text_id)
            register_part(text_id, "label")

        elif t in ("Ceiling", "CeilingLight"):
            # Visualize Ceiling or CeilingLight
            if t == "Ceiling":
                # Draw simple ceiling as a dashed rectangle
                s = item.get("Start", {})
                e = item.get("End", {})
                x0_world = float(s.get("X", 0.0))
                y0_world = float(s.get("Y", 0.0))
                x1_world = float(e.get("X", 0.0))
                y1_world = float(e.get("Y", 0.0))
                sx0, sy0 = self.world_to_screen(x0_world, y0_world)
                sx1, sy1 = self.world_to_screen(x1_world, y1_world)
                cid = self.canvas.create_rectangle(
                    sx0,
                    sy0,
                    sx1,
                    sy1,
                    outline="#6a7aea",
                    dash=(4, 3),
                    fill="",
                    width=2,
                )
                canvas_ids.append(cid)
                register_part(cid, "outline")
                
                # Add corner anchors
                for idx, (wx, wy) in enumerate(
                    [
                        (x0_world, y0_world),
                        (x1_world, y0_world),
                        (x0_world, y1_world),
                        (x1_world, y1_world),
                    ]
                ):
                    anchor_id = self.create_anchor_marker(
                        wx,
                        wy,
                        color="#999999",
                        size=4,
                        meta={"kind": "corner", "corner": idx},
                    )
                    anchors.append(anchor_id)
                    canvas_ids.append(anchor_id)

                center_world_x = (x0_world + x1_world) / 2.0
                center_world_y = (y0_world + y1_world) / 2.0
                center_anchor = self.create_anchor_marker(
                    center_world_x,
                    center_world_y,
                    color="#777777",
                    size=5,
                    meta={"kind": "center", "center": (center_world_x, center_world_y)},
                )
                anchors.append(center_anchor)
                canvas_ids.append(center_anchor)
            
            elif t == "CeilingLight":
                s = item.get("Start", {})
                e = item.get("End", {})
                spacing = item.get("Spacing", {})
                padding = item.get("Padding", {})
                yaw = float(item.get("Yaw", 0.0))  # Rotation angle for line generation
                
                x0_world = float(s.get("X", 0.0))
                y0_world = float(s.get("Y", 0.0))
                x1_world = float(e.get("X", 0.0))
                y1_world = float(e.get("Y", 0.0))
                
                # Draw bounding box for the CeilingLight area
                sx0, sy0 = self.world_to_screen(x0_world, y0_world)
                sx1, sy1 = self.world_to_screen(x1_world, y1_world)
                cid = self.canvas.create_rectangle(sx0, sy0, sx1, sy1,
                                                   outline="#ffa500", dash=(4, 4),
                                                   fill="", width=1)
                canvas_ids.append(cid)
                register_part(cid, "frame")
                
                # Draw lamp positions if enabled - now in lines along rotated axis
                if self.show_lamps.get():
                    
                    space_along_line = float(spacing.get("X", 300.0))  # Spacing between lights on a line
                    space_between_lines = float(spacing.get("Y", 300.0))  # Spacing between parallel lines
                    pad_x = float(padding.get("X", 0.0))
                    pad_y = float(padding.get("Y", 0.0))
                    
                    # Calculate the bounding box with padding
                    min_x = min(x0_world, x1_world)
                    max_x = max(x0_world, x1_world)
                    min_y = min(y0_world, y1_world)
                    max_y = max(y0_world, y1_world)
                    
                    width = max_x - min_x
                    height = max_y - min_y
                    
                    # Convert yaw to radians (yaw 0 = along X axis, positive is counter-clockwise)
                    yaw_rad = math.radians(yaw)
                    
                    # Direction vectors for the line axis (along which lights are placed)
                    line_dir_x = math.cos(yaw_rad)
                    line_dir_y = math.sin(yaw_rad)
                    
                    # Perpendicular direction (for spacing between lines)
                    perp_dir_x = -math.sin(yaw_rad)
                    perp_dir_y = math.cos(yaw_rad)
                    
                    # Calculate effective area after padding in the rotated coordinate system
                    # Apply padding in the direction of the axes
                    center_x = (min_x + max_x) / 2
                    center_y = (min_y + max_y) / 2
                    
                    # Calculate the corners of the padded area
                    # Start from center and work outward with padding adjustments
                    half_w = width / 2 - pad_x
                    half_h = height / 2 - pad_y
                    
                    if half_w <= 0 or half_h <= 0 or space_along_line <= 0 or space_between_lines <= 0:
                        # Skip if invalid dimensions
                        pass
                    else:
                        # Project the padded box onto the line axis to get the length along which we place lights
                        # For simplicity, we'll generate lines across the width perpendicular to yaw
                        # and lights along the yaw direction
                        
                        # Number of lines perpendicular to the yaw direction
                        num_lines = int(2 * half_h / space_between_lines) + 1
                        
                        # Draw lines and lamps
                        for line_idx in range(num_lines):
                            # Position along perpendicular axis (centered)
                            perp_offset = -half_h + line_idx * space_between_lines
                            
                            # Starting point of this line
                            line_start_x = center_x - half_w * line_dir_x + perp_offset * perp_dir_x
                            line_start_y = center_y - half_w * line_dir_y + perp_offset * perp_dir_y
                            
                            line_end_x = center_x + half_w * line_dir_x + perp_offset * perp_dir_x
                            line_end_y = center_y + half_w * line_dir_y + perp_offset * perp_dir_y
                            
                            # Draw the line to show structure
                            ls_x, ls_y = self.world_to_screen(line_start_x, line_start_y)
                            le_x, le_y = self.world_to_screen(line_end_x, line_end_y)
                            line_id = self.canvas.create_line(ls_x, ls_y, le_x, le_y,
                                                              fill="#ffa500", width=1, dash=(2, 2))
                            canvas_ids.append(line_id)
                            register_part(line_id, "gridline")
                            
                            # Place lamps along this line
                            line_length = 2 * half_w
                            num_lamps = int(line_length / space_along_line) + 1
                            
                            for lamp_idx in range(num_lamps):
                                along_offset = -half_w + lamp_idx * space_along_line
                                
                                lamp_x = center_x + along_offset * line_dir_x + perp_offset * perp_dir_x
                                lamp_y = center_y + along_offset * line_dir_y + perp_offset * perp_dir_y
                                
                                sx, sy = self.world_to_screen(lamp_x, lamp_y)
                                lamp_id = self.canvas.create_oval(sx - 3, sy - 3, sx + 3, sy + 3,
                                                                  fill="#ffff00", outline="#ffa500")
                                canvas_ids.append(lamp_id)
                                register_part(lamp_id, "lamp")
                
                # Add corner anchors
                anchor_start = self.create_anchor_marker(
                    x0_world,
                    y0_world,
                    color="#ffa500",
                    meta={"kind": "endpoint", "endpoint": "start"},
                )
                anchor_end = self.create_anchor_marker(
                    x1_world,
                    y1_world,
                    color="#ffa500",
                    meta={"kind": "endpoint", "endpoint": "end"},
                )
                anchors.extend([anchor_start, anchor_end])
                canvas_ids.extend([anchor_start, anchor_end])

                center_world_x = (x0_world + x1_world) / 2.0
                center_world_y = (y0_world + y1_world) / 2.0
                center_anchor = self.create_anchor_marker(
                    center_world_x,
                    center_world_y,
                    color="#777777",
                    size=5,
                    meta={"kind": "center", "center": (center_world_x, center_world_y)},
                )
                anchors.append(center_anchor)
                canvas_ids.append(center_anchor)
        
        elif t == "SpawnPoint":
            # Draw spawn point as a circle with direction indicator
            start = item.get("Start", {})
            yaw = float(item.get("Yaw", 0.0))
            x_world = float(start.get("X", 0.0))
            y_world = float(start.get("Y", 0.0))
            
            sx, sy = self.world_to_screen(x_world, y_world)
            
            # Draw spawn circle
            radius = 8
            spawn_circle = self.canvas.create_oval(sx - radius, sy - radius,
                                                   sx + radius, sy + radius,
                                                   fill="#00ff00", outline="#008800", width=2)
            canvas_ids.append(spawn_circle)
            register_part(spawn_circle, "body")
            
            # Draw direction arrow
            angle_rad = math.radians(yaw)
            arrow_len = 15
            end_x = sx + arrow_len * math.cos(angle_rad)
            end_y = sy - arrow_len * math.sin(angle_rad)
            arrow_line = self.canvas.create_line(sx, sy, end_x, end_y,
                                                 fill="#008800", width=2,
                                                 arrow=tk.LAST, arrowshape=(8, 10, 4))
            canvas_ids.append(arrow_line)
            register_part(arrow_line, "arrow")
            
            # Add anchor
            anchor_id = self.create_anchor_marker(
                x_world,
                y_world,
                color="#00ff00",
                meta={"kind": "point"},
            )
            anchors.append(anchor_id)
            canvas_ids.append(anchor_id)
        
        elif t == "RoomTone":
            # Draw RoomTone as a speaker icon with attenuation radius
            start = item.get("Start", {})
            x_world = float(start.get("X", 0.0))
            y_world = float(start.get("Y", 0.0))
            
            sx, sy = self.world_to_screen(x_world, y_world)
            
            # Get properties
            is_omni = item.get("bOmnidirectional", True)
            attenuation_radius = float(item.get("AttenuationRadius", 1000.0))
            
            # Draw attenuation radius circle (light blue, dashed)
            if attenuation_radius > 0:
                # Convert radius to screen coordinates
                ar_x0, ar_y0 = self.world_to_screen(x_world - attenuation_radius, y_world - attenuation_radius)
                ar_x1, ar_y1 = self.world_to_screen(x_world + attenuation_radius, y_world + attenuation_radius)
                ar_circle = self.canvas.create_oval(ar_x0, ar_y0, ar_x1, ar_y1,
                                                    outline="#87ceeb", dash=(4, 4), width=1)
                canvas_ids.append(ar_circle)
                register_part(ar_circle, "radius")
            
            # Draw speaker icon at center
            speaker_size = 10
            if is_omni:
                # Omnidirectional: solid circle with waves
                speaker = self.canvas.create_oval(sx - speaker_size, sy - speaker_size,
                                                  sx + speaker_size, sy + speaker_size,
                                                  fill="#ff6b6b", outline="#c92a2a", width=2)
                canvas_ids.append(speaker)
                register_part(speaker, "icon")
                
                # Draw sound waves (3 arcs)
                for i in range(1, 4):
                    wave_radius = speaker_size + i * 6
                    arc = self.canvas.create_arc(sx - wave_radius, sy - wave_radius,
                                                 sx + wave_radius, sy + wave_radius,
                                                 start=45, extent=90, style=tk.ARC,
                                                 outline="#ff6b6b", width=1)
                    canvas_ids.append(arc)
                    register_part(arc, "wave")
            else:
                # Directional: cone shape
                cone_points = [
                    sx, sy,  # tip
                    sx - speaker_size, sy - speaker_size * 1.5,
                    sx - speaker_size, sy + speaker_size * 1.5
                ]
                cone = self.canvas.create_polygon(cone_points,
                                                  fill="#ff6b6b", outline="#c92a2a", width=2)
                canvas_ids.append(cone)
                register_part(cone, "icon")
            
            # Add anchor
            anchor_id = self.create_anchor_marker(
                x_world,
                y_world,
                color="#ff6b6b",
                meta={"kind": "point"},
            )
            anchors.append(anchor_id)
            canvas_ids.append(anchor_id)

        if canvas_ids:
            obj = {
                "data": item,
                "canvas_ids": canvas_ids,
                "anchors": anchors,
                "parts": parts,
                "base_styles": base_styles,
            }
            self.objects.append(obj)
            for cid in canvas_ids:
                self.id_to_obj[cid] = obj

        return obj

    # ---------- Selection & styling ----------

    def set_selected(self, obj, multi=False):
        if multi:
            if obj is None:
                return
            group = self._get_linked_selection_group(obj)
            if not group:
                return
            all_selected = all(member in self.selected_objects for member in group)
            if all_selected:
                for member in group:
                    if member in self.selected_objects:
                        self.selected_objects.remove(member)
                        self.style_object(member, selected=False)
                if self.selected_obj in group:
                    self.selected_obj = self.selected_objects[-1] if self.selected_objects else None
            else:
                for member in group:
                    if member not in self.selected_objects:
                        self.selected_objects.append(member)
                        self.style_object(member, selected=True)
                self.selected_obj = obj
            self.update_properties_panel()
            return

        group = self._get_linked_selection_group(obj) if obj is not None else []
        if (
            obj is not None
            and group
            and self.selected_obj is obj
            and len(group) == len(self.selected_objects)
            and all(member in self.selected_objects for member in group)
        ):
            return

        for sobj in self.selected_objects:
            self.style_object(sobj, selected=False)
        self.selected_objects.clear()
        self.selected_obj = None

        if group:
            self.selected_obj = obj
            for member in group:
                if member not in self.selected_objects:
                    self.selected_objects.append(member)
                    self.style_object(member, selected=True)
        elif obj is not None:
            self.selected_objects = [obj]
            self.selected_obj = obj
            self.style_object(obj, selected=True)

        self.update_properties_panel()
    
    def update_properties_panel(self):
        """Update properties panel based on selection"""
        # Clear existing properties
        for widget in self.props_inner.winfo_children():
            widget.destroy()
        
        obj = self.selected_obj
        if not obj:
            tk.Label(self.props_inner, text="No selection", fg="#999999", bg="#fafafa",
                    font=("Segoe UI", 10)).pack(pady=30)
            return

        effective_obj = obj
        pair_primary = None
        if len(self.selected_objects) > 1:
            if self.lock_floor_ceiling.get():
                pair_primary = self._get_floor_ceiling_primary()
            if pair_primary is None:
                tk.Label(self.props_inner, text=f"{len(self.selected_objects)} objects selected",
                        font=("Segoe UI", 10, "bold"), bg="#fafafa").pack(pady=10, padx=10)
                return
            effective_obj = pair_primary

        item = effective_obj["data"]
        item_type = item.get("Type", "Unknown")
        
        type_frame = tk.Frame(self.props_inner, bg="#e3f2fd", relief=tk.FLAT, bd=1)
        type_frame.pack(fill=tk.X, padx=8, pady=8)
        tk.Label(type_frame, text=f"Type: {item_type}", 
                font=("Segoe UI", 10, "bold"), bg="#e3f2fd", fg="#0078d7").pack(pady=6, padx=8)

        if pair_primary is not None:
            tk.Label(type_frame, text="Linked Floor/Ceiling", font=("Segoe UI", 9),
                     bg="#e3f2fd", fg="#005a9e").pack(pady=(0, 4))
        
        # Common properties
        if "Start" in item:
            self._add_property_section("Start Position", item, item.get("Start", {}), ["X", "Y"])
        
        if "End" in item:
            self._add_property_section("End Position", item, item.get("End", {}), ["X", "Y"])
        
        # Type-specific properties
        if item_type == "Cubicle":
            # Cubicles only have Yaw (no Dimensions - they're just points)
            if "Yaw" in item:
                self._add_yaw_property(item)
        
        elif item_type == "SpawnPoint":
            if "HeightOffset" in item:
                self._add_float_property("HeightOffset", item, "HeightOffset")
            if "Yaw" in item:
                self._add_yaw_property(item)
        
        elif item_type in ("Wall", "Door", "Window"):
            if "Thickness" in item:
                self._add_float_property("Thickness", item, "Thickness")
            if item_type == "Window" and "SectionCount" in item:
                self._add_int_property("SectionCount", item, "SectionCount")
        
        elif item_type == "CeilingLight":
            if "Spacing" in item:
                self._add_property_section("Spacing", item, item["Spacing"], ["X", "Y"])
            if "Padding" in item:
                self._add_property_section("Padding", item, item["Padding"], ["X", "Y"])
            if "Yaw" in item:
                self._add_yaw_property(item)
            elif item.get("Yaw") is None:
                # Add Yaw if it doesn't exist (default to 0)
                item["Yaw"] = 0.0
                self._add_yaw_property(item)
        
        elif item_type == "RoomTone":
            if "HeightOffset" in item:
                self._add_float_property("HeightOffset", item, "HeightOffset")
            if "AudioId" in item:
                self._add_string_property("AudioId", item, "AudioId")
            if "bOmnidirectional" in item:
                self._add_bool_property("Omnidirectional", item, "bOmnidirectional")
            if "AttenuationRadius" in item:
                self._add_float_property("AttenuationRadius", item, "AttenuationRadius")
            if "VolumeMultiplier" in item:
                self._add_float_property(
                    "VolumeMultiplier",
                    item,
                    "VolumeMultiplier",
                    minimum=0.0,
                    maximum=10.0,
                    step=0.1,
                    precision=2,
                )
    
    def _add_property_section(self, title, parent_item, data_dict, keys):
        """Add a property section with multiple fields"""
        frame = tk.LabelFrame(self.props_inner, text=title, padx=8, pady=6, bg="#fafafa",
                             font=("Segoe UI", 9, "bold"), relief=tk.GROOVE, bd=1)
        frame.pack(fill=tk.X, padx=8, pady=4)
        
        for key in keys:
            row = tk.Frame(frame, bg="#fafafa")
            row.pack(fill=tk.X, pady=3)
            tk.Label(row, text=f"{key}:", width=10, anchor="w", bg="#fafafa",
                    font=("Segoe UI", 9)).pack(side=tk.LEFT)
            
            # Use Spinbox instead of Entry for better UX
            var = tk.DoubleVar(value=float(data_dict.get(key, 0.0)))
            spinbox = tk.Spinbox(row, textvariable=var, width=10, from_=-10000, to=10000, increment=10)
            spinbox.pack(side=tk.LEFT)

            # Debounce to avoid excessive saves
            timer_id = [None]

            def make_callback(d, k, v, tid):
                def callback(*args):
                    if tid[0] is not None:
                        self.root.after_cancel(tid[0])

                    def apply_change():
                        try:
                            old_val = d.get(k, 0.0)
                            new_val = float(v.get())
                            if abs(old_val - new_val) > 0.001:
                                self.save_state()
                                d[k] = new_val
                                if (
                                    self.lock_floor_ceiling.get()
                                    and parent_item is not None
                                    and parent_item.get("Type") in ("Floor", "Ceiling")
                                ):
                                    self.sync_floor_ceiling_partner(parent_item)
                                self.rebuild_canvas(preserve_selection=True)
                        except (ValueError, tk.TclError):
                            pass
                        tid[0] = None

                    tid[0] = self.root.after(300, apply_change)

                return callback

            var.trace_add("write", make_callback(data_dict, key, var, timer_id))

    def _add_float_property(
        self,
        label,
        item,
        key,
        *,
        minimum=-10000.0,
        maximum=10000.0,
        step=10.0,
        precision=2,
    ):
        """Add a single float property"""
        frame = tk.Frame(self.props_inner, bg="#fafafa")
        frame.pack(fill=tk.X, padx=8, pady=3)
        tk.Label(frame, text=f"{label}:", width=10, anchor="w", bg="#fafafa",
                font=("Segoe UI", 9)).pack(side=tk.LEFT)

        var = tk.DoubleVar(value=float(item.get(key, 0.0)))
        spinbox = tk.Spinbox(
            frame,
            textvariable=var,
            width=10,
            from_=minimum,
            to=maximum,
            increment=step,
            format=f"%.{precision}f",
        )
        spinbox.pack(side=tk.LEFT)

        timer_id = [None]

        def callback(*args):
            if timer_id[0] is not None:
                self.root.after_cancel(timer_id[0])

            def apply_change():
                try:
                    old_val = item.get(key, 0.0)
                    new_val = float(var.get())
                    if abs(old_val - new_val) > 0.001:
                        self.save_state()
                        item[key] = new_val
                        self.rebuild_canvas(preserve_selection=True)
                except (ValueError, tk.TclError):
                    pass
                timer_id[0] = None

            timer_id[0] = self.root.after(300, apply_change)

        var.trace_add("write", callback)
    
    def _add_int_property(self, label, item, key):
        """Add a single integer property"""
        frame = tk.Frame(self.props_inner, bg="#fafafa")
        frame.pack(fill=tk.X, padx=8, pady=3)
        tk.Label(frame, text=f"{label}:", width=10, anchor="w", bg="#fafafa",
                font=("Segoe UI", 9)).pack(side=tk.LEFT)
        
        var = tk.IntVar(value=int(item.get(key, 1)))
        spinbox = tk.Spinbox(frame, textvariable=var, width=10, from_=1, to=100, increment=1)
        spinbox.pack(side=tk.LEFT)
        
        # Debounce to avoid excessive saves
        timer_id = [None]
        
        def callback(*args):
            # Cancel previous timer
            if timer_id[0] is not None:
                self.root.after_cancel(timer_id[0])
            
            # Set new timer for 300ms delay
            def apply_change():
                try:
                    old_val = item.get(key, 1)
                    new_val = int(var.get())
                    if old_val != new_val:  # Only if actually changed
                        self.save_state()
                        item[key] = new_val
                        self.rebuild_canvas(preserve_selection=True)
                except (ValueError, tk.TclError):
                    pass
                timer_id[0] = None
            
            timer_id[0] = self.root.after(300, apply_change)
        
        var.trace_add("write", callback)
    
    def _add_string_property(self, label, item, key):
        """Add a single string property"""
        frame = tk.Frame(self.props_inner, bg="#fafafa")
        frame.pack(fill=tk.X, padx=8, pady=3)
        tk.Label(frame, text=f"{label}:", width=10, anchor="w", bg="#fafafa",
                font=("Segoe UI", 9)).pack(side=tk.LEFT)
        
        var = tk.StringVar(value=str(item.get(key, "")))
        entry = tk.Entry(frame, textvariable=var, width=15)
        entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        # Debounce to avoid excessive saves
        timer_id = [None]
        
        def callback(*args):
            # Cancel previous timer
            if timer_id[0] is not None:
                self.root.after_cancel(timer_id[0])
            
            # Set new timer for 300ms delay
            def apply_change():
                try:
                    old_val = item.get(key, "")
                    new_val = var.get()
                    if old_val != new_val:  # Only if actually changed
                        self.save_state()
                        item[key] = new_val
                        self.rebuild_canvas(preserve_selection=True)
                except (ValueError, tk.TclError):
                    pass
                timer_id[0] = None
            
            timer_id[0] = self.root.after(300, apply_change)
        
        var.trace_add("write", callback)
    
    def _add_bool_property(self, label, item, key):
        """Add a single boolean property"""
        frame = tk.Frame(self.props_inner, bg="#fafafa")
        frame.pack(fill=tk.X, padx=8, pady=3)
        
        var = tk.BooleanVar(value=bool(item.get(key, False)))
        
        def callback():
            try:
                old_val = item.get(key, False)
                new_val = var.get()
                if old_val != new_val:  # Only if actually changed
                    self.save_state()
                    item[key] = new_val
                    self.rebuild_canvas(preserve_selection=True)
            except (ValueError, tk.TclError):
                pass
        
        checkbutton = tk.Checkbutton(
            frame,
            text=label,
            variable=var,
            command=callback,
            font=("Segoe UI", 9),
            bg="#fafafa",
            activebackground="#fafafa",
            selectcolor="#fafafa",
        )
        checkbutton.pack(side=tk.LEFT)
    
    def _add_yaw_property(self, item):
        """Add yaw property with rotation buttons"""
        frame = tk.LabelFrame(self.props_inner, text="Rotation (Yaw)", padx=8, pady=6, bg="#fafafa",
                             font=("Segoe UI", 9, "bold"), relief=tk.GROOVE, bd=1)
        frame.pack(fill=tk.X, padx=8, pady=4)
        
        row = tk.Frame(frame, bg="#fafafa")
        row.pack(fill=tk.X, pady=3)
        tk.Label(row, text="Degrees:", width=10, anchor="w", bg="#fafafa",
                font=("Segoe UI", 9)).pack(side=tk.LEFT)
        
        var = tk.DoubleVar(value=float(item.get("Yaw", 0.0)))
        spinbox = tk.Spinbox(row, textvariable=var, width=10, from_=0, to=360, increment=15)
        spinbox.pack(side=tk.LEFT)
        
        # Debounce to avoid excessive saves
        timer_id = [None]
        
        def callback(*args):
            # Cancel previous timer
            if timer_id[0] is not None:
                self.root.after_cancel(timer_id[0])
            
            # Set new timer for 300ms delay
            def apply_change():
                try:
                    old_val = item.get("Yaw", 0.0)
                    new_val = float(var.get())
                    if abs(old_val - new_val) > 0.001:  # Only if actually changed
                        self.save_state()
                        item["Yaw"] = new_val
                        self.rebuild_canvas(preserve_selection=True)
                except (ValueError, tk.TclError):
                    pass
                timer_id[0] = None
            
            timer_id[0] = self.root.after(300, apply_change)
        
        var.trace_add("write", callback)
        
        # Rotation buttons
        btn_row = tk.Frame(frame, bg="#fafafa")
        btn_row.pack(fill=tk.X, pady=4)
        tk.Button(btn_row, text="⟳ 90°", command=lambda: self.rotate_selection(90),
                 bg="#0078d7", fg="white", activebackground="#005a9e",
                 relief=tk.RAISED, bd=1, padx=10, pady=3,
                 font=("Segoe UI", 9)).pack(side=tk.LEFT, padx=2)
        tk.Button(btn_row, text="⟲ 90°", command=lambda: self.rotate_selection(-90),
                 bg="#0078d7", fg="white", activebackground="#005a9e",
                 relief=tk.RAISED, bd=1, padx=10, pady=3,
                 font=("Segoe UI", 9)).pack(side=tk.LEFT, padx=2)

    def _apply_canvas_colors(self, cid, *, fill=None, outline=None):
        options = {}
        if fill is not None:
            options["fill"] = fill
        if outline is not None:
            options["outline"] = outline
        if not options:
            return

        try:
            self.canvas.itemconfig(cid, **options)
            return
        except tk.TclError:
            pass

        item_type = self.canvas.type(cid)

        if item_type == "line":
            if fill is not None:
                try:
                    self.canvas.itemconfig(cid, fill=fill)
                except tk.TclError:
                    pass
            return

        fallback = {}
        if fill is not None:
            fallback["fill"] = fill
        if fallback:
            try:
                self.canvas.itemconfig(cid, **fallback)
                return
            except tk.TclError:
                pass

        if outline is not None:
            try:
                self.canvas.itemconfig(cid, outline=outline)
            except tk.TclError:
                pass

    def style_object(self, obj, selected=False):
        item = obj["data"]
        t = item.get("Type")
        anchors = set(obj.get("anchors", []))
        base_styles = obj.get("base_styles", {})
        parts = obj.get("parts", {})

        for cid in anchors:
            try:
                self.canvas.itemconfigure(cid, state="normal" if selected else "hidden")
            except tk.TclError:
                pass

        def restore_style(cid):
            style = base_styles.get(cid)
            if not style:
                return
            self._apply_canvas_colors(
                cid,
                fill=style.get("fill"),
                outline=style.get("outline"),
            )

        if not selected:
            for cid in obj["canvas_ids"]:
                if cid in anchors:
                    continue
                restore_style(cid)
            return

        touched = set()

        if t in ("Wall", "Door", "Window"):
            if t == "Wall":
                color = "#ff5252"
            elif t == "Door":
                color = "#2fb46d"
            else:
                color = "#5393ff"
            for cid in obj["canvas_ids"]:
                if cid in anchors:
                    continue
                self._apply_canvas_colors(cid, fill=color, outline=color)
                touched.add(cid)
        elif t == "Floor":
            for cid in parts.get("body", []):
                self._apply_canvas_colors(cid, outline="#ff6666")
                touched.add(cid)
        elif t == "Ceiling":
            for cid in parts.get("outline", []):
                self._apply_canvas_colors(cid, outline="#ff6666")
                touched.add(cid)
        elif t == "Cubicle":
            for cid in parts.get("body", []):
                self._apply_canvas_colors(cid, fill="#fde7c7", outline="#ff5a5f")
                touched.add(cid)
            for cid in parts.get("arrow", []):
                self._apply_canvas_colors(cid, fill="#ff5a5f", outline="#ff5a5f")
                touched.add(cid)
            for cid in parts.get("label", []):
                self._apply_canvas_colors(cid, fill="#c2410c")
                touched.add(cid)
        elif t == "SpawnPoint":
            for cid in parts.get("body", []):
                self._apply_canvas_colors(cid, fill="#fff08a", outline="#d9480f")
                touched.add(cid)
            for cid in parts.get("arrow", []):
                self._apply_canvas_colors(cid, fill="#d9480f", outline="#d9480f")
                touched.add(cid)
        elif t == "CeilingLight":
            for cid in parts.get("frame", []):
                self._apply_canvas_colors(cid, outline="#ff8c00")
                touched.add(cid)
            for cid in parts.get("gridline", []):
                self._apply_canvas_colors(cid, fill="#ff8c00")
                touched.add(cid)
            for cid in parts.get("lamp", []):
                self._apply_canvas_colors(cid, fill="#fff3a1", outline="#ffb347")
                touched.add(cid)
        elif t == "RoomTone":
            for cid in parts.get("icon", []):
                self._apply_canvas_colors(cid, fill="#ff9090", outline="#ff3b3b")
                touched.add(cid)
            for cid in parts.get("wave", []):
                self._apply_canvas_colors(cid, fill="#ff9090", outline="#ff9090")
                touched.add(cid)
            for cid in parts.get("radius", []):
                restore_style(cid)
                touched.add(cid)
        elif t == "Elevator":
            for cid in parts.get("line", []):
                self._apply_canvas_colors(cid, fill="#d896ff", outline="#d896ff")
                touched.add(cid)
            for cid in parts.get("cab", []):
                self._apply_canvas_colors(cid, fill="#f5eef8", outline="#d896ff")
                touched.add(cid)
        else:
            for cid in obj["canvas_ids"]:
                if cid in anchors:
                    continue
                self._apply_canvas_colors(cid, outline="#ff5a5f")
                touched.add(cid)

        for cid in obj["canvas_ids"]:
            if cid in anchors or cid in touched:
                continue
            restore_style(cid)

    def can_rotate(self, item):
        return "Yaw" in item

    def rotate_selection(self, delta_deg=90):
        if not self.selected_objects:
            return
        
        self.save_state()  # Save state for undo
        
        # Rotate all selected objects
        any_rotated = False
        for obj in self.selected_objects:
            item = obj["data"]
            if self.can_rotate(item):
                self.apply_rotation(item, delta_deg)
                if self.snap_to_grid.get():
                    self.snap_object(item)
                any_rotated = True
        
        if any_rotated:
            self.rebuild_canvas(preserve_selection=True)
        elif hasattr(self.root, "bell"):
            self.root.bell()

    def apply_rotation(self, item, delta_deg):
        yaw_old = self.normalize_yaw(item.get("Yaw", 0.0))
        yaw_new = self.normalize_yaw(yaw_old + delta_deg)
        item["Yaw"] = yaw_new

        if item.get("Type") == "Cubicle" and "Start" in item:
            start = item["Start"]
            # Use display dimensions for rotation adjustment
            display_width = float(self.cubicle_display_width.get())
            display_depth = float(self.cubicle_display_depth.get())
            dim = {"X": display_width, "Y": display_depth}
            width_old, height_old = self.get_axis_size(dim, yaw_old)
            start_x = float(start.get("X", 0.0))
            start_y = float(start.get("Y", 0.0))
            center_x = start_x + width_old / 2
            center_y = start_y + height_old / 2

            width_new, height_new = self.get_axis_size(dim, yaw_new)
            new_start_x = center_x - width_new / 2
            new_start_y = center_y - height_new / 2
            start["X"] = new_start_x
            start["Y"] = new_start_y

    # ---------- Mouse handlers ----------

    def find_object_at(self, sx, sy):
        items = self.canvas.find_overlapping(sx, sy, sx, sy)
        for cid in reversed(items):
            obj = self.id_to_obj.get(cid)
            if obj is not None:
                t = obj["data"].get("Type")
                if t in ("Wall", "Door", "Window", "Elevator", "Cubicle", "Floor", "Ceiling", "SpawnPoint", "CeilingLight", "RoomTone"):
                    return obj
        return None

    def find_anchor_at(self, sx, sy):
        items = self.canvas.find_overlapping(sx, sy, sx, sy)
        for cid in reversed(items):
            if "anchor" not in self.canvas.gettags(cid):
                continue
            meta = self.anchor_meta.get(cid)
            if meta is None:
                continue
            obj = self.id_to_obj.get(cid)
            if obj is None:
                continue
            return obj, meta
        return None, None

    def redraw_item(self, item):
        old_obj = None
        old_index = None
        for idx, obj in enumerate(self.objects):
            if obj["data"] is item:
                old_obj = obj
                old_index = idx
                break
        if old_obj is None or old_index is None:
            return

        selected_indices = [i for i, sobj in enumerate(self.selected_objects) if sobj is old_obj]
        was_primary = self.selected_obj is old_obj

        self.objects.pop(old_index)
        for cid in old_obj["canvas_ids"]:
            self.canvas.delete(cid)
            self.id_to_obj.pop(cid, None)
            self.anchor_meta.pop(cid, None)

        new_obj = self.draw_item(item)
        if new_obj is None:
            return

        # Maintain original ordering
        self.objects.pop()
        self.objects.insert(old_index, new_obj)

        if was_primary:
            self.selected_obj = new_obj
        for idx in selected_indices:
            self.selected_objects[idx] = new_obj

        if was_primary or selected_indices:
            self.style_object(new_obj, selected=True)

    def _get_rectangle_corners(self, item):
        s = item.get("Start", {})
        e = item.get("End", {})
        x0 = float(s.get("X", 0.0))
        y0 = float(s.get("Y", 0.0))
        x1 = float(e.get("X", 0.0))
        y1 = float(e.get("Y", 0.0))
        return [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]

    def _get_cubicle_corners(self, item):
        start = item.get("Start", {})
        yaw = self.normalize_yaw(float(item.get("Yaw", 0.0)))
        display_width = float(self.cubicle_display_width.get())
        display_depth = float(self.cubicle_display_depth.get())
        dim = {"X": display_width, "Y": display_depth}
        vis_yaw = self.normalize_yaw(yaw + 90.0)
        w_world, h_world = self.get_axis_size(dim, vis_yaw)
        x0 = float(start.get("X", 0.0))
        y0 = float(start.get("Y", 0.0))
        x1 = x0 + w_world
        y1 = y0 + h_world
        return [(x0, y0), (x1, y0), (x0, y1), (x1, y1)]

    def _rect_signature(self, item):
        start = item.get("Start", {}) if isinstance(item, dict) else {}
        end = item.get("End", {}) if isinstance(item, dict) else {}
        x0 = float(start.get("X", 0.0))
        y0 = float(start.get("Y", 0.0))
        x1 = float(end.get("X", 0.0))
        y1 = float(end.get("Y", 0.0))
        min_x, max_x = sorted((x0, x1))
        min_y, max_y = sorted((y0, y1))
        return (
            round(min_x, 4),
            round(min_y, 4),
            round(max_x, 4),
            round(max_y, 4),
        )

    def find_floor_ceiling_partner_data(self, item):
        if not isinstance(item, dict):
            return None
        partner = self.floor_ceiling_links.get(id(item))
        if partner is not None and partner in self.data:
            return partner
        item_type = item.get("Type")
        if item_type not in ("Floor", "Ceiling"):
            return None
        target_type = "Ceiling" if item_type == "Floor" else "Floor"
        signature = self._rect_signature(item)
        for candidate in self.data:
            if candidate is item:
                continue
            if candidate.get("Type") != target_type:
                continue
            if self._rect_signature(candidate) == signature:
                self.floor_ceiling_links[id(item)] = candidate
                self.floor_ceiling_links[id(candidate)] = item
                return candidate
        return None

    def find_floor_ceiling_partner_object(self, obj):
        if obj is None:
            return None
        partner_data = self.find_floor_ceiling_partner_data(obj.get("data"))
        if partner_data is None:
            return None
        for candidate in self.objects:
            if candidate["data"] is partner_data:
                return candidate
        return None

    def _copy_floor_ceiling_shape(self, source_item, target_item):
        if not isinstance(source_item, dict) or not isinstance(target_item, dict):
            return
        for key in ("Start", "End"):
            if key not in source_item:
                continue
            src = source_item.get(key, {})
            dst = target_item.setdefault(key, {})
            dst["X"] = float(src.get("X", 0.0))
            dst["Y"] = float(src.get("Y", 0.0))

    def sync_floor_ceiling_partner(self, item):
        partner = self.find_floor_ceiling_partner_data(item)
        if partner is None:
            return None
        self._copy_floor_ceiling_shape(item, partner)
        self.floor_ceiling_links[id(item)] = partner
        self.floor_ceiling_links[id(partner)] = item
        return partner

    def refresh_floor_ceiling_links(self):
        self.floor_ceiling_links.clear()
        signature_map = {}

        for item in self.data:
            if not isinstance(item, dict):
                continue
            item_type = item.get("Type")
            if item_type not in ("Floor", "Ceiling"):
                continue
            signature = self._rect_signature(item)
            bucket = signature_map.setdefault(signature, {"Floor": [], "Ceiling": []})
            bucket[item_type].append(item)

        for bucket in signature_map.values():
            floors = bucket.get("Floor", [])
            ceilings = bucket.get("Ceiling", [])
            for floor_item, ceiling_item in zip(floors, ceilings):
                self.floor_ceiling_links[id(floor_item)] = ceiling_item
                self.floor_ceiling_links[id(ceiling_item)] = floor_item

    def ensure_floor_ceiling_pairs(self):
        created_items = []
        for item in list(self.data):
            item_type = item.get("Type")
            if item_type not in ("Floor", "Ceiling"):
                continue

            if self.find_floor_ceiling_partner_data(item) is not None:
                continue

            start = item.get("Start")
            end = item.get("End")
            if not isinstance(start, dict) or not isinstance(end, dict):
                continue

            partner_type = "Ceiling" if item_type == "Floor" else "Floor"
            partner_start = {
                "X": float(start.get("X", 0.0)),
                "Y": float(start.get("Y", 0.0)),
            }
            partner_end = {
                "X": float(end.get("X", 0.0)),
                "Y": float(end.get("Y", 0.0)),
            }

            created_items.append(
                {
                    "Type": partner_type,
                    "Start": partner_start,
                    "End": partner_end,
                }
            )

        if created_items:
            self.save_state()
            self.data.extend(created_items)
            for item in created_items:
                existing = self.find_floor_ceiling_partner_data(item)
                if existing is None:
                    continue
                self.floor_ceiling_links[id(item)] = existing
                self.floor_ceiling_links[id(existing)] = item
            self.refresh_floor_ceiling_links()
        return created_items

    def _get_linked_selection_group(self, obj):
        if obj is None:
            return []
        group = [obj]
        if not self.lock_floor_ceiling.get():
            return group
        item_type = obj["data"].get("Type")
        if item_type not in ("Floor", "Ceiling"):
            return group
        partner_obj = self.find_floor_ceiling_partner_object(obj)
        if partner_obj and partner_obj not in group:
            group.append(partner_obj)
        return group

    def _get_floor_ceiling_primary(self):
        if len(self.selected_objects) != 2:
            return None
        floor_objs = [obj for obj in self.selected_objects if obj["data"].get("Type") == "Floor"]
        ceiling_objs = [obj for obj in self.selected_objects if obj["data"].get("Type") == "Ceiling"]
        if not floor_objs or not ceiling_objs:
            return None
        floor_obj = floor_objs[0]
        partner = self.find_floor_ceiling_partner_object(floor_obj)
        if partner is not ceiling_objs[0]:
            return None
        if self.selected_obj in (floor_obj, partner):
            return self.selected_obj
        return floor_obj

    def handle_anchor_drag(self, event):
        if not self.dragging_anchor:
            return

        item = self.dragging_anchor.get("item")
        meta = self.dragging_anchor.get("meta", {})
        if item is None:
            return

        wx, wy = self.screen_to_world(event.x, event.y)
        wx, wy = self.snap_point(wx, wy)

        item_type = item.get("Type")
        kind = meta.get("kind")
        changed = False

        if kind == "endpoint":
            endpoint = meta.get("endpoint")
            target = item.get("Start") if endpoint == "start" else item.get("End")
            if target is not None:
                old_x = float(target.get("X", 0.0))
                old_y = float(target.get("Y", 0.0))
                if not math.isclose(old_x, wx, abs_tol=1e-6) or not math.isclose(old_y, wy, abs_tol=1e-6):
                    target["X"] = wx
                    target["Y"] = wy
                    changed = True

        elif kind == "corner":
            corner_index = meta.get("corner")
            if corner_index is None:
                return

            if item_type in ("Floor", "Ceiling", "CeilingLight"):
                start = item.setdefault("Start", {})
                end = item.setdefault("End", {})

                target_x = start if corner_index in (0, 2) else end
                target_y = start if corner_index in (0, 1) else end

                old_x = float(target_x.get("X", 0.0))
                old_y = float(target_y.get("Y", 0.0))

                if not math.isclose(old_x, wx, abs_tol=1e-6):
                    target_x["X"] = wx
                    changed = True
                if not math.isclose(old_y, wy, abs_tol=1e-6):
                    target_y["Y"] = wy
                    changed = True

            elif item_type == "Cubicle":
                corners = self._get_cubicle_corners(item)
                if 0 <= corner_index < len(corners):
                    old_corner = corners[corner_index]
                    dx = wx - old_corner[0]
                    dy = wy - old_corner[1]
                    if abs(dx) > 1e-6 or abs(dy) > 1e-6:
                        start = item.setdefault("Start", {})
                        start["X"] = float(start.get("X", 0.0)) + dx
                        start["Y"] = float(start.get("Y", 0.0)) + dy
                        changed = True

        elif kind == "point":
            start = item.setdefault("Start", {})
            old_x = float(start.get("X", 0.0))
            old_y = float(start.get("Y", 0.0))
            if not math.isclose(old_x, wx, abs_tol=1e-6) or not math.isclose(old_y, wy, abs_tol=1e-6):
                start["X"] = wx
                start["Y"] = wy
                changed = True

        elif kind == "center":
            if item_type in ("Wall", "Door", "Window"):
                start = item.setdefault("Start", {})
                end = item.setdefault("End", {})
                sx = float(start.get("X", 0.0))
                sy = float(start.get("Y", 0.0))
                ex = float(end.get("X", 0.0))
                ey = float(end.get("Y", 0.0))
                cx = (sx + ex) / 2.0
                cy = (sy + ey) / 2.0
                dx = wx - cx
                dy = wy - cy
                if abs(dx) > 1e-6 or abs(dy) > 1e-6:
                    start["X"] = sx + dx
                    start["Y"] = sy + dy
                    end["X"] = ex + dx
                    end["Y"] = ey + dy
                    changed = True

            elif item_type in ("Floor", "Ceiling", "CeilingLight"):
                start = item.setdefault("Start", {})
                end = item.setdefault("End", {})
                sx = float(start.get("X", 0.0))
                sy = float(start.get("Y", 0.0))
                ex = float(end.get("X", 0.0))
                ey = float(end.get("Y", 0.0))
                cx = (sx + ex) / 2.0
                cy = (sy + ey) / 2.0
                dx = wx - cx
                dy = wy - cy
                if abs(dx) > 1e-6 or abs(dy) > 1e-6:
                    start["X"] = sx + dx
                    start["Y"] = sy + dy
                    end["X"] = ex + dx
                    end["Y"] = ey + dy
                    changed = True

            elif item_type == "Cubicle":
                start = item.setdefault("Start", {})
                display_width = float(self.cubicle_display_width.get())
                display_depth = float(self.cubicle_display_depth.get())
                yaw = self.normalize_yaw(float(item.get("Yaw", 0.0)))
                dim = {"X": display_width, "Y": display_depth}
                vis_yaw = self.normalize_yaw(yaw + 90.0)
                width_world, height_world = self.get_axis_size(dim, vis_yaw)
                sx = float(start.get("X", 0.0))
                sy = float(start.get("Y", 0.0))
                cx = sx + width_world / 2.0
                cy = sy + height_world / 2.0
                dx = wx - cx
                dy = wy - cy
                if abs(dx) > 1e-6 or abs(dy) > 1e-6:
                    start["X"] = sx + dx
                    start["Y"] = sy + dy
                    changed = True

        if changed:
            partner_data = None
            if self.lock_floor_ceiling.get() and item_type in ("Floor", "Ceiling"):
                partner_data = self.sync_floor_ceiling_partner(item)

            self.redraw_item(item)
            if partner_data is not None:
                self.redraw_item(partner_data)

            self.update_properties_panel()

    def on_left_click(self, event):
        mode = self.mode.get()
        wx, wy = self.screen_to_world(event.x, event.y)

        if mode not in ("add_wall", "add_door", "add_window", "add_elevator", "add_floor_ceiling"):
            self.clear_pending_line()

        if mode == "select":
            ctrl_pressed = (event.state & 0x4) != 0  # Check if Ctrl is pressed

            anchor_obj, anchor_meta = self.find_anchor_at(event.x, event.y)
            if anchor_obj is not None and anchor_meta is not None:
                # If the anchor's object is already selected, keep selection and drag
                if anchor_obj not in self.selected_objects:
                    self.set_selected(anchor_obj, multi=False)
                self.dragging_anchor = {"item": anchor_obj["data"], "meta": anchor_meta}
                self.drag_saved_state = False
                self.dragging = True
                self.drag_start_sx = event.x
                self.drag_start_sy = event.y
                return

            obj = self.find_object_at(event.x, event.y)
            
            if obj is not None:
                # Clicked on an object
                if ctrl_pressed:
                    # Ctrl+click toggles selection
                    self.set_selected(obj, multi=True)
                elif obj in self.selected_objects:
                    # Clicked on already-selected object: keep selection, start drag
                    pass
                else:
                    # Clicked on unselected object: select it (deselect others)
                    self.set_selected(obj, multi=False)
                
                self.dragging = True
                self.drag_start_sx = event.x
                self.drag_start_sy = event.y
                self.drag_saved_state = False
            else:
                # Clicked on empty space: start box selection
                if not ctrl_pressed:
                    # Clear selection unless Ctrl is held
                    self.set_selected(None)
                self.box_selecting = True
                self.box_select_start = (event.x, event.y)
                self.box_select_rect_id = self.canvas.create_rectangle(
                    event.x, event.y, event.x, event.y,
                    outline="#0078d7", dash=(3, 3), width=1
                )
        elif mode == "add_cubicle":
            self.add_cubicle_at(wx, wy)
        elif mode == "add_spawn":
            self.add_spawn_at(wx, wy)
        elif mode == "add_roomtone":
            self.add_roomtone_at(wx, wy)
        elif mode == "add_floor_ceiling":
            self.handle_floor_ceiling_press(event)
        elif mode in ("add_wall", "add_door", "add_window", "add_elevator"):
            self.handle_line_press(mode, event)

    def on_drag(self, event):
        # Handle box selection drag
        if self.box_selecting and self.box_select_start is not None:
            sx, sy = self.box_select_start
            if self.box_select_rect_id is not None:
                self.canvas.coords(self.box_select_rect_id, sx, sy, event.x, event.y)
            return

        if self.pending_line and self.pending_line.get("preview_id") is not None:
            self.update_pending_line(event)
            return

        if self.dragging_anchor is not None:
            if not self.drag_saved_state:
                self.save_state()
                self.drag_saved_state = True
            self.handle_anchor_drag(event)
            return

        if not self.dragging or not self.selected_objects:
            return
        
        # Save state on first drag movement (not just click)
        if not self.drag_saved_state:
            self.save_state()
            self.drag_saved_state = True
        
        dx_pix = event.x - self.drag_start_sx
        dy_pix = event.y - self.drag_start_sy

        wx0, wy0, wx1, wy1 = self.world_bbox
        width, height = self.current_canvas_size()
        dx_world = dx_pix / width * (wx1 - wx0)
        dy_world = -dy_pix / height * (wy1 - wy0)

        self.drag_start_sx = event.x
        self.drag_start_sy = event.y

        handled_items = set()

        def move_object(target_obj):
            item = target_obj["data"]
            data_id = id(item)
            if data_id in handled_items:
                return
            handled_items.add(data_id)

            if "Start" in item:
                s = item["Start"]
                s["X"] = float(s.get("X", 0.0)) + dx_world
                s["Y"] = float(s.get("Y", 0.0)) + dy_world

            if "End" in item:
                e = item["End"]
                e["X"] = float(e.get("X", 0.0)) + dx_world
                e["Y"] = float(e.get("Y", 0.0)) + dy_world

            for cid in target_obj["canvas_ids"]:
                self.canvas.move(cid, dx_pix, dy_pix)

        for obj in list(self.selected_objects):
            move_object(obj)

            if (
                self.lock_floor_ceiling.get()
                and obj["data"].get("Type") in ("Floor", "Ceiling")
            ):
                partner_obj = self.find_floor_ceiling_partner_object(obj)
                if partner_obj is not None:
                    move_object(partner_obj)

    def on_release(self, event):
        if self.pending_line:
            if self.pending_line.get("dragged"):
                wx, wy = self.screen_to_world(event.x, event.y)
                wx, wy = self.snap_point(wx, wy)
                self.finalize_pending_line((wx, wy))
            self.dragging = False
            return

        # Handle box selection release
        if self.box_selecting:
            self.finish_box_selection(event)
            return

        if self.dragging_anchor is not None:
            self.dragging_anchor = None
            self.dragging = False
            self.drag_saved_state = False
            self.update_properties_panel()
            return

        if self.dragging and self.selected_objects:
            any_snapped = False
            for obj in self.selected_objects:
                if self.snap_object(obj["data"]):
                    any_snapped = True
            if any_snapped:
                self.rebuild_canvas(preserve_selection=True)
        self.dragging = False

    def finish_box_selection(self, event):
        """Finalize box selection and select all objects within the box."""
        if self.box_select_rect_id is not None:
            self.canvas.delete(self.box_select_rect_id)
            self.box_select_rect_id = None

        if self.box_select_start is None:
            self.box_selecting = False
            return

        sx, sy = self.box_select_start
        ex, ey = event.x, event.y

        # Normalize coordinates (ensure min < max)
        x0, x1 = min(sx, ex), max(sx, ex)
        y0, y1 = min(sy, ey), max(sy, ey)

        self.box_selecting = False
        self.box_select_start = None

        # Find all objects that intersect with the box
        for obj in self.objects:
            if obj in self.selected_objects:
                continue  # Already selected

            # Check if any of the object's canvas items overlap with the box
            for cid in obj["canvas_ids"]:
                if cid in obj.get("anchors", []):
                    continue  # Skip anchor markers

                try:
                    bbox = self.canvas.bbox(cid)
                    if bbox is None:
                        continue

                    item_x0, item_y0, item_x1, item_y1 = bbox

                    # Check if bounding boxes overlap
                    if not (item_x1 < x0 or item_x0 > x1 or item_y1 < y0 or item_y0 > y1):
                        # Overlaps - select this object
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

        self.update_properties_panel()

    # ---------- Add / Delete ----------

    def on_delete(self, event=None):
        if not self.selected_objects:
            return
        
        # Save state for undo
        self.save_state()

        self.dragging_anchor = None
        
        # Delete all selected objects
        for obj in self.selected_objects:
            item = obj["data"]
            partner = self.floor_ceiling_links.pop(id(item), None)
            if partner is not None:
                self.floor_ceiling_links.pop(id(partner), None)
            for cid in obj["canvas_ids"]:
                self.canvas.delete(cid)
                self.id_to_obj.pop(cid, None)
                self.anchor_meta.pop(cid, None)
            try:
                self.data.remove(item)
            except ValueError:
                pass
            try:
                self.objects.remove(obj)
            except ValueError:
                pass
        
        self.selected_obj = None
        self.selected_objects.clear()

    def add_cubicle_at(self, wx, wy):
        self.save_state()  # Save state for undo
        wx, wy = self.snap_point(wx, wy)
        item = {
            "Type": "Cubicle",
            "Start": {"X": wx, "Y": wy},
            "Yaw": 0.0
        }
        self.data.append(item)
        obj = self.draw_item(item)
        if obj is not None:
            self.set_selected(obj)
    
    def add_spawn_at(self, wx, wy):
        self.save_state()  # Save state for undo
        
        # Remove any existing spawn points (only one allowed)
        existing_spawns = [item for item in self.data if item.get("Type") == "SpawnPoint"]
        for spawn in existing_spawns:
            self.data.remove(spawn)
        
        wx, wy = self.snap_point(wx, wy)
        item = {
            "Type": "SpawnPoint",
            "Start": {"X": wx, "Y": wy},
            "HeightOffset": 100.0,
            "Yaw": 0.0
        }
        self.data.append(item)
        
        # Rebuild canvas to remove old spawn visualization
        self.rebuild_canvas(preserve_selection=False)
        
        # Find and select the new spawn
        for obj in self.objects:
            if obj["data"] is item:
                self.set_selected(obj)
                break
    
    def add_roomtone_at(self, wx, wy):
        self.save_state()  # Save state for undo
        wx, wy = self.snap_point(wx, wy)
        item = {
            "Type": "RoomTone",
            "Start": {"X": wx, "Y": wy},
            "HeightOffset": 150.0,
            "AudioId": "NewRoomTone",
            "bOmnidirectional": True,
            "AttenuationRadius": 1000.0,
            "VolumeMultiplier": 1.0
        }
        self.data.append(item)
        obj = self.draw_item(item)
        if obj is not None:
            self.set_selected(obj)

    def line_mode_color(self, mode):
        return {
            "add_wall": "#222222",
            "add_door": "#2b8a45",
            "add_window": "#1d6bd6",
            "add_elevator": "#9b59b6",
            "add_floor_ceiling": "#9966cc",
        }.get(mode, "#555555")
    
    def handle_floor_ceiling_press(self, event):
        """Handle placement of Floor+Ceiling pair using rectangle drawing"""
        wx, wy = self.screen_to_world(event.x, event.y)
        wx, wy = self.snap_point(wx, wy)
        
        pending = self.pending_line
        mode = "add_floor_ceiling"
        
        if pending and pending.get("mode") != mode:
            self.clear_pending_line()
            pending = None
        
        if pending is None:
            self.begin_pending_line(mode, (wx, wy))
            return
        
        if pending.get("dragged"):
            # user finished previous drag; start a fresh segment from current point
            self.clear_pending_line()
            self.begin_pending_line(mode, (wx, wy))
            return
        
        start_world = pending.get("start_world")
        if start_world and math.isclose(start_world[0], wx, abs_tol=1e-6) and math.isclose(start_world[1], wy, abs_tol=1e-6):
            return
        
        self.create_floor_ceiling_pair(start_world, (wx, wy))
        self.clear_pending_line()
    
    def create_floor_ceiling_pair(self, start_world, end_world):
        """Create paired Floor and Ceiling objects"""
        if start_world == end_world:
            return
        
        self.save_state()  # Save state for undo
        
        start_x, start_y = self.snap_point(*start_world)
        end_x, end_y = self.snap_point(*end_world)
        
        # Create Floor
        floor_item = {
            "Type": "Floor",
            "Start": {"X": float(start_x), "Y": float(start_y)},
            "End": {"X": float(end_x), "Y": float(end_y)}
        }
        self.data.append(floor_item)
        
        # Create Ceiling
        ceiling_item = {
            "Type": "Ceiling",
            "Start": {"X": float(start_x), "Y": float(start_y)},
            "End": {"X": float(end_x), "Y": float(end_y)}
        }
        self.data.append(ceiling_item)

        self.floor_ceiling_links[id(floor_item)] = ceiling_item
        self.floor_ceiling_links[id(ceiling_item)] = floor_item
        
        # Rebuild and select the floor
        self.rebuild_canvas(preserve_selection=False)
        for obj in self.objects:
            if obj["data"] is floor_item:
                self.set_selected(obj)
                break

    def handle_line_press(self, mode, event):
        wx, wy = self.screen_to_world(event.x, event.y)
        wx, wy = self.snap_point(wx, wy)

        pending = self.pending_line
        if pending and pending.get("mode") != mode:
            self.clear_pending_line()
            pending = None

        if pending is None:
            self.begin_pending_line(mode, (wx, wy))
            return

        if pending.get("dragged"):
            # user finished previous drag; start a fresh segment from current point
            self.clear_pending_line()
            self.begin_pending_line(mode, (wx, wy))
            return

        start_world = pending.get("start_world")
        if start_world and math.isclose(start_world[0], wx, abs_tol=1e-6) and math.isclose(start_world[1], wy, abs_tol=1e-6):
            return

        self.create_line_item(mode, start_world, (wx, wy))
        self.clear_pending_line()

    def begin_pending_line(self, mode, start_world):
        color = self.line_mode_color(mode)
        sx, sy = self.world_to_screen(*start_world)
        
        # For floor_ceiling mode, use rectangle instead of line
        if mode == "add_floor_ceiling":
            preview_id = self.canvas.create_rectangle(sx, sy, sx, sy,
                                                     outline=color, dash=(8, 4), width=2,
                                                     fill="", tags=("preview",))
        else:
            preview_id = self.canvas.create_line(sx, sy, sx, sy,
                                                 fill=color, dash=(8, 4), width=2,
                                                 tags=("preview",))
        
        marker_id = self.create_anchor_marker(start_world[0], start_world[1],
                                              color=color, size=6,
                                              state="normal", tags=("preview",))

        self.pending_line = {
            "mode": mode,
            "start_world": start_world,
            "preview_id": preview_id,
            "start_marker_id": marker_id,
            "dragged": False,
            "current_end_world": start_world,
        }
        self.pending_preview_id = preview_id
        self.canvas.tag_raise("preview")

    def update_pending_line(self, event):
        if not self.pending_line:
            return
        wx, wy = self.screen_to_world(event.x, event.y)
        wx, wy = self.snap_point(wx, wy)
        start_wx, start_wy = self.pending_line["start_world"]
        sx0, sy0 = self.world_to_screen(start_wx, start_wy)
        sx1, sy1 = self.world_to_screen(wx, wy)
        self.canvas.coords(self.pending_line["preview_id"], sx0, sy0, sx1, sy1)
        self.pending_line["current_end_world"] = (wx, wy)
        if (wx, wy) != (start_wx, start_wy):
            self.pending_line["dragged"] = True

    def finalize_pending_line(self, end_world=None):
        pending = self.pending_line
        if not pending:
            return
        start_world = pending.get("start_world")
        if end_world is None:
            end_world = pending.get("current_end_world", start_world)

        if start_world == end_world:
            self.clear_pending_line()
            return

        mode = pending.get("mode")
        if mode == "add_floor_ceiling":
            self.create_floor_ceiling_pair(start_world, end_world)
        else:
            self.create_line_item(mode, start_world, end_world)
        self.clear_pending_line()

    def create_line_item(self, mode, start_world, end_world):
        if start_world == end_world:
            return

        self.save_state()  # Save state for undo
        
        start_x, start_y = self.snap_point(*start_world)
        end_x, end_y = self.snap_point(*end_world)

        if mode == "add_wall":
            t = "Wall"
            thickness = 25.0
        elif mode == "add_door":
            t = "Door"
            thickness = 1.0
        elif mode == "add_elevator":
            t = "Elevator"
            thickness = None  # Elevator doesn't use thickness
        else:
            t = "Window"
            thickness = 1.0

        item = {
            "Type": t,
            "Start": {"X": float(start_x), "Y": float(start_y)},
            "End": {"X": float(end_x), "Y": float(end_y)},
        }
        if thickness is not None:
            item["Thickness"] = float(thickness)
        self.data.append(item)
        obj = self.draw_item(item)
        if obj is not None:
            self.set_selected(obj)

    def snap_object(self, item):
        if not self.snap_to_grid.get():
            return False

        changed = False

        if "Start" in item:
            s = item["Start"]
            x = float(s.get("X", 0.0))
            y = float(s.get("Y", 0.0))
            sx, sy = self.snap_point(x, y)
            if not math.isclose(sx, x, abs_tol=1e-6) or not math.isclose(sy, y, abs_tol=1e-6):
                s["X"] = sx
                s["Y"] = sy
                changed = True

        if "End" in item:
            e = item["End"]
            x = float(e.get("X", 0.0))
            y = float(e.get("Y", 0.0))
            sx, sy = self.snap_point(x, y)
            if not math.isclose(sx, x, abs_tol=1e-6) or not math.isclose(sy, y, abs_tol=1e-6):
                e["X"] = sx
                e["Y"] = sy
                changed = True

        if item.get("Type") == "Cubicle" and "Start" in item:
            s = item["Start"]
            x = float(s.get("X", 0.0))
            y = float(s.get("Y", 0.0))
            sx, sy = self.snap_point(x, y)
            if not math.isclose(sx, x, abs_tol=1e-6) or not math.isclose(sy, y, abs_tol=1e-6):
                s["X"] = sx
                s["Y"] = sy
                changed = True

        return changed
    
    # ---------- Copy/Paste ----------
    
    def on_copy(self, event=None):
        """Copy selected objects to clipboard"""
        if not self.selected_objects:
            return
        self.clipboard.clear()
        for obj in self.selected_objects:
            # Deep copy the data
            item_copy = json.loads(json.dumps(obj["data"]))
            self.clipboard.append(item_copy)
        self.status_label.config(text=f"Copied {len(self.clipboard)} object(s)")
        self.root.after(2000, lambda: self.status_label.config(
            text="Pan: Right/Middle drag | Zoom: Wheel | Undo: Ctrl+Z | Redo: Ctrl+Y | Copy/Paste: Ctrl+C/V"))
        return "break"
    
    def on_paste(self, event=None):
        """Paste objects from clipboard with offset"""
        if not self.clipboard:
            return "break"
        
        self.save_state()  # Save state for undo
        
        # Calculate offset for paste (slightly to the right and down)
        offset_x = self.get_grid_size() if self.snap_to_grid.get() else 50.0
        offset_y = self.get_grid_size() if self.snap_to_grid.get() else 50.0
        
        # Clear current selection
        self.set_selected(None)
        new_objects = []
        
        for item_data in self.clipboard:
            # Deep copy and offset
            item = json.loads(json.dumps(item_data))
            
            # Apply offset to position
            if "Start" in item:
                s = item["Start"]
                s["X"] = float(s.get("X", 0.0)) + offset_x
                s["Y"] = float(s.get("Y", 0.0)) + offset_y
            if "End" in item:
                e = item["End"]
                e["X"] = float(e.get("X", 0.0)) + offset_x
                e["Y"] = float(e.get("Y", 0.0)) + offset_y
            
            self.data.append(item)
            obj = self.draw_item(item)
            if obj is not None:
                new_objects.append(obj)
        
        # Select the pasted objects
        if new_objects:
            self.selected_objects = new_objects
            self.selected_obj = new_objects[0] if len(new_objects) == 1 else None
            for obj in new_objects:
                self.style_object(obj, selected=True)

        if any(obj["data"].get("Type") in ("Floor", "Ceiling") for obj in new_objects):
            self.refresh_floor_ceiling_links()
        
        self.status_label.config(text=f"Pasted {len(new_objects)} object(s)")
        self.root.after(2000, lambda: self.status_label.config(
            text="Pan: Right/Middle drag | Zoom: Wheel | Undo: Ctrl+Z | Redo: Ctrl+Y | Copy/Paste: Ctrl+C/V"))
        return "break"
    
    def on_select_all(self, event=None):
        """Select all objects"""
        self.selected_objects.clear()
        self.selected_obj = None
        
        for obj in self.objects:
            item_type = obj["data"].get("Type")
            if item_type not in ("Wall", "Door", "Window", "Elevator", "Cubicle", "SpawnPoint", "CeilingLight", "RoomTone", "Floor", "Ceiling"):
                continue

            group = self._get_linked_selection_group(obj) if self.lock_floor_ceiling.get() and item_type in ("Floor", "Ceiling") else [obj]
            for member in group:
                if member not in self.selected_objects:
                    self.selected_objects.append(member)
                    self.style_object(member, selected=True)
        
        if self.selected_objects:
            self.selected_obj = self.selected_objects[0]
        else:
            self.selected_obj = None

        self.update_properties_panel()
        
        return "break"
    
    # ---------- Undo/Redo ----------
    
    def save_state(self):
        """Save current state to undo stack"""
        # Deep copy current data
        state = json.loads(json.dumps(self.data))
        self.undo_stack.append(state)
        if len(self.undo_stack) > self.max_undo:
            self.undo_stack.pop(0)
        # Clear redo stack on new action
        self.redo_stack.clear()
    
    def on_undo(self, event=None):
        """Undo last action"""
        if not self.undo_stack:
            return "break"
        
        # Save current state to redo stack
        current_state = json.loads(json.dumps(self.data))
        self.redo_stack.append(current_state)
        
        # Restore previous state
        self.data = self.undo_stack.pop()
        self.rebuild_canvas(preserve_selection=False)
        
        self.status_label.config(text="Undo")
        self.root.after(2000, lambda: self.status_label.config(
            text="Pan: Right/Middle drag | Zoom: Wheel | Undo: Ctrl+Z | Redo: Ctrl+Y | Copy/Paste: Ctrl+C/V"))
        return "break"
    
    def on_redo(self, event=None):
        """Redo last undone action"""
        if not self.redo_stack:
            return "break"
        
        # Save current state to undo stack
        current_state = json.loads(json.dumps(self.data))
        self.undo_stack.append(current_state)
        
        # Restore next state
        self.data = self.redo_stack.pop()
        self.rebuild_canvas(preserve_selection=False)
        
        self.status_label.config(text="Redo")
        self.root.after(2000, lambda: self.status_label.config(
            text="Pan: Right/Middle drag | Zoom: Wheel | Undo: Ctrl+Z | Redo: Ctrl+Y | Copy/Paste: Ctrl+C/V"))
        return "break"


def main():
    root = tk.Tk()
    app = FloorplanEditor(root)
    root.mainloop()


if __name__ == "__main__":
    main()
