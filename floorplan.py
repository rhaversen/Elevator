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
        self.grid_size.trace_add("write", self.on_grid_setting_changed)
        self.show_grid.trace_add("write", self.on_grid_setting_changed)
        self.canvas_grid_ids = []

        self.pending_line = None  # dict with start/end data while drawing
        self.pending_preview_id = None

        self.selected_obj = None
        self.dragging = False
        self.drag_start_sx = 0
        self.drag_start_sy = 0
        self.drag_saved_state = False  # Track if state was saved for this drag

        self.mode = tk.StringVar(value="select")
        self.mode.trace_add("write", self.on_mode_changed)

        self._build_ui()

    def _build_ui(self):
          # Main toolbar
          toolbar = tk.Frame(self.root)
          toolbar.pack(side=tk.TOP, fill=tk.X)

          # Mode selection frame
          mode_frame = tk.LabelFrame(toolbar, text="Mode", padx=5, pady=2)
          mode_frame.pack(side=tk.LEFT, padx=5, pady=2)
          
          tk.Radiobutton(mode_frame, text="Select", variable=self.mode,
                     value="select").pack(side=tk.LEFT)
          tk.Radiobutton(mode_frame, text="Cubicle", variable=self.mode,
                     value="add_cubicle").pack(side=tk.LEFT)
          tk.Radiobutton(mode_frame, text="Wall", variable=self.mode,
                     value="add_wall").pack(side=tk.LEFT)
          tk.Radiobutton(mode_frame, text="Door", variable=self.mode,
                     value="add_door").pack(side=tk.LEFT)
          tk.Radiobutton(mode_frame, text="Window", variable=self.mode,
                     value="add_window").pack(side=tk.LEFT)
          tk.Radiobutton(mode_frame, text="Spawn", variable=self.mode,
                     value="add_spawn").pack(side=tk.LEFT)
          
          # View controls frame
          view_frame = tk.LabelFrame(toolbar, text="View", padx=5, pady=2)
          view_frame.pack(side=tk.LEFT, padx=5, pady=2)
          
          tk.Button(view_frame, text="Reset",
                command=self.reset_view).pack(side=tk.LEFT)
          tk.Button(view_frame, text="Zoom+",
                command=self.zoom_in).pack(side=tk.LEFT)
          tk.Button(view_frame, text="Zoom−",
                command=self.zoom_out).pack(side=tk.LEFT)
          
          # Rotation controls frame
          rotate_frame = tk.LabelFrame(toolbar, text="Rotate", padx=5, pady=2)
          rotate_frame.pack(side=tk.LEFT, padx=5, pady=2)
          
          tk.Button(rotate_frame, text="⟳ 90°",
                command=lambda: self.rotate_selection(90)).pack(side=tk.LEFT)
          tk.Button(rotate_frame, text="⟲ 90°",
                command=lambda: self.rotate_selection(-90)).pack(side=tk.LEFT)

          # Grid controls frame
          grid_frame = tk.LabelFrame(toolbar, text="Grid", padx=5, pady=2)
          grid_frame.pack(side=tk.LEFT, padx=5, pady=2)
          
          tk.Checkbutton(grid_frame, text="Snap", variable=self.snap_to_grid).pack(side=tk.LEFT)
          tk.Label(grid_frame, text="Size:").pack(side=tk.LEFT, padx=(5, 0))
          tk.Spinbox(grid_frame, from_=10, to=2000, increment=10,
                 width=5, textvariable=self.grid_size,
                 command=self.on_grid_setting_changed).pack(side=tk.LEFT)
          tk.Checkbutton(grid_frame, text="Show",
                     variable=self.show_grid).pack(side=tk.LEFT, padx=(5, 0))
          
          # Display options frame
          display_frame = tk.LabelFrame(toolbar, text="Display", padx=5, pady=2)
          display_frame.pack(side=tk.LEFT, padx=5, pady=2)
          
          tk.Checkbutton(display_frame, text="Lamps",
                     variable=self.show_lamps,
                     command=lambda: self.rebuild_canvas(preserve_selection=True)).pack(side=tk.LEFT)
          
          # Properties panel on the right
          self.props_frame = tk.Frame(self.root, width=250, relief=tk.SUNKEN, bd=1)
          self.props_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=2, pady=2)
          self.props_frame.pack_propagate(False)
          
          props_title = tk.Label(self.props_frame, text="Properties", font=("Arial", 10, "bold"))
          props_title.pack(pady=5)
          
          # Scrollable properties area
          self.props_canvas = tk.Canvas(self.props_frame, highlightthickness=0)
          props_scrollbar = tk.Scrollbar(self.props_frame, orient="vertical", command=self.props_canvas.yview)
          self.props_inner = tk.Frame(self.props_canvas)
          
          self.props_canvas.configure(yscrollcommand=props_scrollbar.set)
          props_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
          self.props_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
          
          self.props_canvas_window = self.props_canvas.create_window((0, 0), window=self.props_inner, anchor="nw")
          self.props_inner.bind("<Configure>", lambda e: self.props_canvas.configure(scrollregion=self.props_canvas.bbox("all")))
          
          # Status bar with help text
          status_frame = tk.Frame(self.root, relief=tk.SUNKEN, bd=1)
          status_frame.pack(side=tk.BOTTOM, fill=tk.X)
          self.status_label = tk.Label(status_frame, 
                                       text="Pan: Right/Middle drag | Zoom: Wheel | Undo: Ctrl+Z | Redo: Ctrl+Y | Copy/Paste: Ctrl+C/V",
                                       anchor=tk.W)
          self.status_label.pack(side=tk.LEFT, padx=5)

          self.canvas = tk.Canvas(self.root, width=self.canvas_width,
                          height=self.canvas_height, bg="white")
          self.canvas.pack(fill=tk.BOTH, expand=True)

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
          self.root.bind("<Control-y>", self.on_redo)

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
                dim = item.get("Dimensions", {})
                yaw = float(item.get("Yaw", 0.0))
                w = float(dim.get("X", 0.0))
                h = float(dim.get("Y", 0.0))
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
            if pending_mode != mode or mode not in ("add_wall", "add_door", "add_window"):
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

    def create_anchor_marker(self, wx, wy, color="#ff8844", size=5, state="hidden", tags=("anchor",)):
        sx, sy = self.world_to_screen(wx, wy)
        cid = self.canvas.create_rectangle(sx - size, sy - size,
                                           sx + size, sy + size,
                                           outline="", fill=color,
                                           tags=tags)
        if state != "normal":
            self.canvas.itemconfigure(cid, state=state)
        return cid

    def cancel_transient_actions(self):
        self.clear_pending_line()
        if self.pan_active:
            self.pan_active = False
            if hasattr(self, "canvas"):
                self.canvas.configure(cursor="")
        self.dragging = False

    def rebuild_canvas(self, preserve_selection=False):
        self.clear_pending_line()
        selected_data_list = [obj["data"] for obj in self.selected_objects] if preserve_selection and self.selected_objects else []
        if self.selected_obj is not None:
            self.style_object(self.selected_obj, selected=False)
        for obj in self.selected_objects:
            self.style_object(obj, selected=False)
        self.selected_obj = None
        self.selected_objects.clear()

        self.canvas.delete("all")
        self.canvas_grid_ids.clear()
        self.objects.clear()
        self.id_to_obj.clear()

        self.draw_background_grid()

        to_select = []
        for item in self.data:
            obj = self.draw_item(item)
            if obj is not None and item in selected_data_list:
                to_select.append(obj)

        if to_select:
            self.selected_objects = to_select
            for obj in to_select:
                self.style_object(obj, selected=True)
            if len(to_select) == 1:
                self.selected_obj = to_select[0]

    def draw_item(self, item):
        t = item.get("Type")
        canvas_ids = []
        anchors = []
        obj = None

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
            for wx, wy in ((x0_world, y0_world), (x1_world, y0_world),
                           (x0_world, y1_world), (x1_world, y1_world)):
                anchor_id = self.create_anchor_marker(wx, wy, color="#999999", size=4)
                anchors.append(anchor_id)
                canvas_ids.append(anchor_id)

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
                    tick_length = min(door_length * 0.4, 35)
                    tick_x = mx - uy * tick_length
                    tick_y = my + ux * tick_length
                    tick_line = self.canvas.create_line(mx, my, tick_x, tick_y,
                                                        fill=color, width=max(1, width - 1),
                                                        capstyle=tk.ROUND)
                    canvas_ids.append(tick_line)

            anchor_start = self.create_anchor_marker(sx_world, sy_world, color="#ff8c00")
            anchor_end = self.create_anchor_marker(ex_world, ey_world, color="#1f78d1")
            anchors.extend([anchor_start, anchor_end])
            canvas_ids.extend([anchor_start, anchor_end])

        elif t == "Cubicle":
            start = item.get("Start", {})
            dim = item.get("Dimensions", {})
            yaw = self.normalize_yaw(float(item.get("Yaw", 0.0)))
            item["Yaw"] = yaw
            w_world, h_world = self.get_axis_size(dim, yaw)
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
            
            # Add rotation indicator (small triangle in corner) - using screen coordinates
            yaw_normalized = self.normalize_yaw(yaw)
            indicator_size = min(abs(x1 - x0), abs(y1 - y0)) * 0.15
            # Draw indicator in top-left corner pointing in rotation direction
            if yaw_normalized == 0.0:
                # Pointing right
                tri_screen = [x0, y0, x0 + indicator_size, y0, x0, y0 + indicator_size]
            elif yaw_normalized == 90.0:
                # Pointing down
                tri_screen = [x0, y0, x0 + indicator_size, y0, x0 + indicator_size, y0 + indicator_size]
            elif yaw_normalized == 180.0:
                # Pointing left
                tri_screen = [x0 + indicator_size, y0, x0 + indicator_size, y0 + indicator_size, x0, y0 + indicator_size]
            else:  # 270.0
                # Pointing up
                tri_screen = [x0, y0 + indicator_size, x0 + indicator_size, y0 + indicator_size, x0, y0]
            
            tri_id = self.canvas.create_polygon(tri_screen, fill="#888888", outline="black")
            canvas_ids.append(tri_id)

            corners = [
                (x0_world, y0_world),
                (x1_world, y0_world),
                (x0_world, y1_world),
                (x1_world, y1_world)
            ]
            for idx, (wx, wy) in enumerate(corners):
                color = "#ff8c00" if idx == 0 else ("#1f78d1" if idx == 3 else "#666666")
                anchor_id = self.create_anchor_marker(wx, wy, color=color)
                anchors.append(anchor_id)
                canvas_ids.append(anchor_id)
            
            # Add dimension text label (always visible)
            center_x = (x0_world + x1_world) / 2
            center_y = (y0_world + y1_world) / 2
            cx, cy = self.world_to_screen(center_x, center_y)
            dim_x = float(dim.get("X", 0.0))
            dim_y = float(dim.get("Y", 0.0))
            dim_text = f"{dim_x:.0f}×{dim_y:.0f}"
            text_id = self.canvas.create_text(cx, cy, text=dim_text,
                                             fill="#555555", font=("Arial", 9))
            canvas_ids.append(text_id)

        elif t in ("Ceiling", "CeilingLight"):
            # Visualize CeilingLight with optional lamp markers
            if t == "CeilingLight":
                s = item.get("Start", {})
                e = item.get("End", {})
                spacing = item.get("Spacing", {})
                padding = item.get("Padding", {})
                
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
                
                # Draw lamp positions if enabled
                if self.show_lamps.get():
                    space_x = float(spacing.get("X", 400.0))
                    space_y = float(spacing.get("Y", 400.0))
                    pad_x = float(padding.get("X", 0.0))
                    pad_y = float(padding.get("Y", 0.0))
                    
                    # Calculate lamp positions with padding and spacing
                    min_x = min(x0_world, x1_world) + pad_x
                    max_x = max(x0_world, x1_world) - pad_x
                    min_y = min(y0_world, y1_world) + pad_y
                    max_y = max(y0_world, y1_world) - pad_y
                    
                    if space_x > 0 and space_y > 0:
                        lamp_x = min_x
                        while lamp_x <= max_x:
                            lamp_y = min_y
                            while lamp_y <= max_y:
                                sx, sy = self.world_to_screen(lamp_x, lamp_y)
                                lamp_id = self.canvas.create_oval(sx - 3, sy - 3, sx + 3, sy + 3,
                                                                  fill="#ffff00", outline="#ffa500")
                                canvas_ids.append(lamp_id)
                                lamp_y += space_y
                            lamp_x += space_x
                
                # Add corner anchors
                anchor_start = self.create_anchor_marker(x0_world, y0_world, color="#ffa500")
                anchor_end = self.create_anchor_marker(x1_world, y1_world, color="#ffa500")
                anchors.extend([anchor_start, anchor_end])
                canvas_ids.extend([anchor_start, anchor_end])
        
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
            
            # Draw direction arrow
            angle_rad = math.radians(yaw)
            arrow_len = 15
            end_x = sx + arrow_len * math.cos(angle_rad)
            end_y = sy - arrow_len * math.sin(angle_rad)
            arrow_line = self.canvas.create_line(sx, sy, end_x, end_y,
                                                 fill="#008800", width=2,
                                                 arrow=tk.LAST, arrowshape=(8, 10, 4))
            canvas_ids.append(arrow_line)
            
            # Add anchor
            anchor_id = self.create_anchor_marker(x_world, y_world, color="#00ff00")
            anchors.append(anchor_id)
            canvas_ids.append(anchor_id)

        if canvas_ids:
            obj = {"data": item, "canvas_ids": canvas_ids, "anchors": anchors}
            self.objects.append(obj)
            for cid in canvas_ids:
                self.id_to_obj[cid] = obj

        return obj

    # ---------- Selection & styling ----------

    def set_selected(self, obj, multi=False):
        if multi:
            # Multi-select mode (Ctrl+Click)
            if obj is None:
                return
            if obj in self.selected_objects:
                # Deselect if already selected
                self.selected_objects.remove(obj)
                self.style_object(obj, selected=False)
                if self.selected_obj is obj:
                    self.selected_obj = self.selected_objects[0] if self.selected_objects else None
            else:
                # Add to selection
                self.selected_objects.append(obj)
                self.style_object(obj, selected=True)
                self.selected_obj = obj
        else:
            # Single select mode
            if self.selected_obj is obj and not self.selected_objects:
                return
            # Clear previous selection
            if self.selected_obj is not None:
                self.style_object(self.selected_obj, selected=False)
            for sobj in self.selected_objects:
                self.style_object(sobj, selected=False)
            self.selected_objects.clear()
            
            self.selected_obj = obj
            if obj is not None:
                self.selected_objects = [obj]
                self.style_object(obj, selected=True)
        
        # Update properties panel
        self.update_properties_panel()
    
    def update_properties_panel(self):
        """Update properties panel based on selection"""
        # Clear existing properties
        for widget in self.props_inner.winfo_children():
            widget.destroy()
        
        if not self.selected_obj:
            tk.Label(self.props_inner, text="No selection", fg="gray").pack(pady=20)
            return
        
        if len(self.selected_objects) > 1:
            tk.Label(self.props_inner, text=f"{len(self.selected_objects)} objects selected", 
                    font=("Arial", 9, "bold")).pack(pady=5)
            return
        
        item = self.selected_obj["data"]
        item_type = item.get("Type", "Unknown")
        
        tk.Label(self.props_inner, text=f"Type: {item_type}", 
                font=("Arial", 9, "bold")).pack(pady=5, anchor="w", padx=5)
        
        # Common properties
        if "Start" in item:
            self._add_property_section("Start Position", item["Start"], ["X", "Y"])
        
        if "End" in item:
            self._add_property_section("End Position", item["End"], ["X", "Y"])
        
        # Type-specific properties
        if item_type == "Cubicle":
            if "Dimensions" in item:
                self._add_property_section("Dimensions", item["Dimensions"], ["X", "Y"])
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
                self._add_property_section("Spacing", item["Spacing"], ["X", "Y"])
            if "Padding" in item:
                self._add_property_section("Padding", item["Padding"], ["X", "Y"])
    
    def _add_property_section(self, title, data_dict, keys):
        """Add a property section with multiple fields"""
        frame = tk.LabelFrame(self.props_inner, text=title, padx=5, pady=5)
        frame.pack(fill=tk.X, padx=5, pady=2)
        
        for key in keys:
            row = tk.Frame(frame)
            row.pack(fill=tk.X, pady=2)
            tk.Label(row, text=f"{key}:", width=12, anchor="w").pack(side=tk.LEFT)
            
            var = tk.DoubleVar(value=float(data_dict.get(key, 0.0)))
            entry = tk.Entry(row, textvariable=var, width=10)
            entry.pack(side=tk.LEFT)
            
            def make_callback(d, k, v):
                def callback(*args):
                    try:
                        self.save_state()
                        d[k] = float(v.get())
                        self.rebuild_canvas(preserve_selection=True)
                    except (ValueError, tk.TclError):
                        pass
                return callback
            
            var.trace_add("write", make_callback(data_dict, key, var))
    
    def _add_float_property(self, label, item, key):
        """Add a single float property"""
        frame = tk.Frame(self.props_inner)
        frame.pack(fill=tk.X, padx=5, pady=2)
        tk.Label(frame, text=f"{label}:", width=12, anchor="w").pack(side=tk.LEFT)
        
        var = tk.DoubleVar(value=float(item.get(key, 0.0)))
        entry = tk.Entry(frame, textvariable=var, width=10)
        entry.pack(side=tk.LEFT)
        
        def callback(*args):
            try:
                self.save_state()
                item[key] = float(var.get())
                self.rebuild_canvas(preserve_selection=True)
            except (ValueError, tk.TclError):
                pass
        
        var.trace_add("write", callback)
    
    def _add_int_property(self, label, item, key):
        """Add a single integer property"""
        frame = tk.Frame(self.props_inner)
        frame.pack(fill=tk.X, padx=5, pady=2)
        tk.Label(frame, text=f"{label}:", width=12, anchor="w").pack(side=tk.LEFT)
        
        var = tk.IntVar(value=int(item.get(key, 1)))
        entry = tk.Entry(frame, textvariable=var, width=10)
        entry.pack(side=tk.LEFT)
        
        def callback(*args):
            try:
                self.save_state()
                item[key] = int(var.get())
                self.rebuild_canvas(preserve_selection=True)
            except (ValueError, tk.TclError):
                pass
        
        var.trace_add("write", callback)
    
    def _add_yaw_property(self, item):
        """Add yaw property with rotation buttons"""
        frame = tk.LabelFrame(self.props_inner, text="Rotation (Yaw)", padx=5, pady=5)
        frame.pack(fill=tk.X, padx=5, pady=2)
        
        row = tk.Frame(frame)
        row.pack(fill=tk.X, pady=2)
        tk.Label(row, text="Degrees:", width=12, anchor="w").pack(side=tk.LEFT)
        
        var = tk.DoubleVar(value=float(item.get("Yaw", 0.0)))
        entry = tk.Entry(row, textvariable=var, width=10)
        entry.pack(side=tk.LEFT)
        
        def callback(*args):
            try:
                self.save_state()
                item["Yaw"] = float(var.get())
                self.rebuild_canvas(preserve_selection=True)
            except (ValueError, tk.TclError):
                pass
        
        var.trace_add("write", callback)
        
        # Rotation buttons
        btn_row = tk.Frame(frame)
        btn_row.pack(fill=tk.X, pady=2)
        tk.Button(btn_row, text="⟳ 90°", command=lambda: self.rotate_selection(90)).pack(side=tk.LEFT, padx=2)
        tk.Button(btn_row, text="⟲ 90°", command=lambda: self.rotate_selection(-90)).pack(side=tk.LEFT, padx=2)

    def style_object(self, obj, selected=False):
        item = obj["data"]
        t = item.get("Type")
        anchors = set(obj.get("anchors", []))
        for cid in obj["canvas_ids"]:
            if cid in anchors:
                state = "normal" if selected else "hidden"
                self.canvas.itemconfigure(cid, state=state)
                continue
            if t in ("Wall", "Door", "Window"):
                if t == "Wall":
                    base_color = "#222222"
                elif t == "Door":
                    base_color = "#2b8a45"
                else:
                    base_color = "#1d6bd6"
                color = "red" if selected else base_color
                self.canvas.itemconfig(cid, fill=color, outline=color)
            elif t == "Cubicle":
                outline = "red" if selected else "black"
                fill = "#ffeecc" if selected else "#dddddd"
                self.canvas.itemconfig(cid, outline=outline, fill=fill)
            elif t == "Floor":
                outline = "#ffaaaa" if selected else "#cccccc"
                self.canvas.itemconfig(cid, outline=outline)
            elif t == "SpawnPoint":
                outline = "red" if selected else "#008800"
                fill = "#ffff00" if selected else "#00ff00"
                try:
                    self.canvas.itemconfig(cid, outline=outline, fill=fill)
                except tk.TclError:
                    pass  # Some canvas items might not support these config options
            elif t == "CeilingLight":
                outline = "red" if selected else "#ffa500"
                try:
                    self.canvas.itemconfig(cid, outline=outline)
                except tk.TclError:
                    pass

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

        if item.get("Type") == "Cubicle" and "Start" in item and "Dimensions" in item:
            start = item["Start"]
            dim = item["Dimensions"]
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
                if t in ("Wall", "Door", "Window", "Cubicle", "Floor", "SpawnPoint", "CeilingLight"):
                    return obj
        return None

    def on_left_click(self, event):
        mode = self.mode.get()
        wx, wy = self.screen_to_world(event.x, event.y)

        if mode not in ("add_wall", "add_door", "add_window"):
            self.clear_pending_line()

        if mode == "select":
            obj = self.find_object_at(event.x, event.y)
            ctrl_pressed = (event.state & 0x4) != 0  # Check if Ctrl is pressed
            self.set_selected(obj, multi=ctrl_pressed)
            if obj is not None:
                self.dragging = True
                self.drag_start_sx = event.x
                self.drag_start_sy = event.y
                self.drag_saved_state = False  # Haven't saved state yet
        elif mode == "add_cubicle":
            self.add_cubicle_at(wx, wy)
        elif mode == "add_spawn":
            self.add_spawn_at(wx, wy)
        elif mode in ("add_wall", "add_door", "add_window"):
            self.handle_line_press(mode, event)

    def on_drag(self, event):
        if self.pending_line and self.pending_line.get("preview_id") is not None:
            self.update_pending_line(event)
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

        # Move all selected objects
        for obj in self.selected_objects:
            item = obj["data"]

            if "Start" in item:
                s = item["Start"]
                s["X"] = float(s.get("X", 0.0)) + dx_world
                s["Y"] = float(s.get("Y", 0.0)) + dy_world

            if "End" in item:
                e = item["End"]
                e["X"] = float(e.get("X", 0.0)) + dx_world
                e["Y"] = float(e.get("Y", 0.0)) + dy_world

            for cid in obj["canvas_ids"]:
                self.canvas.move(cid, dx_pix, dy_pix)

    def on_release(self, event):
        if self.pending_line:
            if self.pending_line.get("dragged"):
                wx, wy = self.screen_to_world(event.x, event.y)
                wx, wy = self.snap_point(wx, wy)
                self.finalize_pending_line((wx, wy))
            self.dragging = False
            return

        if self.dragging and self.selected_objects:
            any_snapped = False
            for obj in self.selected_objects:
                if self.snap_object(obj["data"]):
                    any_snapped = True
            if any_snapped:
                self.rebuild_canvas(preserve_selection=True)
        self.dragging = False

    # ---------- Add / Delete ----------

    def on_delete(self, event=None):
        if not self.selected_objects:
            return
        
        # Save state for undo
        self.save_state()
        
        # Delete all selected objects
        for obj in self.selected_objects:
            for cid in obj["canvas_ids"]:
                self.canvas.delete(cid)
                self.id_to_obj.pop(cid, None)
            try:
                self.data.remove(obj["data"])
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
            "Dimensions": {"X": 300.0, "Y": 250.0},
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

    def line_mode_color(self, mode):
        return {
            "add_wall": "#222222",
            "add_door": "#2b8a45",
            "add_window": "#1d6bd6",
        }.get(mode, "#555555")

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

        self.create_line_item(pending.get("mode"), start_world, end_world)
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
        else:
            t = "Window"
            thickness = 1.0

        item = {
            "Type": t,
            "Start": {"X": float(start_x), "Y": float(start_y)},
            "End": {"X": float(end_x), "Y": float(end_y)},
            "Thickness": float(thickness)
        }
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
            # Only select editable objects
            if item_type in ("Wall", "Door", "Window", "Cubicle", "SpawnPoint", "CeilingLight"):
                self.selected_objects.append(obj)
                self.style_object(obj, selected=True)
        
        if len(self.selected_objects) == 1:
            self.selected_obj = self.selected_objects[0]
        
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
