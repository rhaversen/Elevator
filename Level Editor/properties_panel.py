"""
Properties Panel for the Floorplan Editor.

Provides a reusable properties panel component for editing object properties.
"""

import tkinter as tk
from typing import Dict, List, Any, Optional, Callable

try:
    from .object_types import TYPE_MAP
except ImportError:
    from object_types import TYPE_MAP


class PropertiesPanel:
    """Manages the properties panel UI for editing selected objects."""
    
    def __init__(
        self,
        parent: tk.Frame,
        on_property_changed: Callable[[], None],
        get_lock_floor_ceiling: Callable[[], bool],
        sync_floor_ceiling_partner: Callable[[Dict], None],
    ):
        """
        Initialize the properties panel.
        
        Args:
            parent: Parent frame to build the panel in
            on_property_changed: Callback when a property is modified
            get_lock_floor_ceiling: Callback to check if floor/ceiling linking is enabled
            sync_floor_ceiling_partner: Callback to sync floor/ceiling partners
        """
        self.parent = parent
        self.on_property_changed = on_property_changed
        self.get_lock_floor_ceiling = get_lock_floor_ceiling
        self.sync_floor_ceiling_partner = sync_floor_ceiling_partner
        
        self._build_ui()
    
    def _build_ui(self):
        """Build the properties panel UI structure."""
        self.frame = tk.Frame(self.parent, width=280, relief=tk.FLAT, bd=1, bg="#fafafa")
        self.frame.pack(side=tk.RIGHT, fill=tk.Y, padx=0, pady=0)
        self.frame.pack_propagate(False)

        # Header
        header = tk.Frame(self.frame, bg="#0078d7", height=35)
        header.pack(fill=tk.X)
        tk.Label(header, text="Properties", font=("Segoe UI", 11, "bold"),
                 bg="#0078d7", fg="white").pack(pady=8)

        # Scrollable content area
        self.canvas = tk.Canvas(self.frame, highlightthickness=0, bg="#fafafa")
        scrollbar = tk.Scrollbar(self.frame, orient="vertical", command=self.canvas.yview)
        self.inner = tk.Frame(self.canvas, bg="#fafafa")

        self.canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.canvas_window = self.canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.inner.bind("<Configure>",
            lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
    
    def clear(self):
        """Clear all widgets from the properties panel."""
        for w in self.inner.winfo_children():
            w.destroy()
    
    def show_no_selection(self):
        """Show 'no selection' message."""
        self.clear()
        tk.Label(self.inner, text="No selection", bg="#fafafa", fg="#999999").pack(pady=10)
    
    def show_single_selection(self, obj: Dict):
        """Show properties for a single selected object."""
        self.clear()
        
        if not obj:
            self.show_no_selection()
            return
        
        item = obj.get("data", obj)
        item_type = item.get("Type", "Unknown")

        # Type header
        tk.Label(self.inner, text=f"Type: {item_type}", bg="#e3f2fd", fg="#333333",
                 font=("Segoe UI", 11, "bold")).pack(anchor="w", fill="x", padx=5, pady=(10, 5))

        type_def = TYPE_MAP.get(item_type)
        if type_def:
            self._create_property_entries(item, type_def)
    
    def show_multi_selection(
        self,
        selected_objects: List[Dict],
        on_clear_selection: Callable[[], None],
        on_remove_from_selection: Callable[[Dict], None],
    ):
        """Show panel for multiple selected objects."""
        self.clear()
        
        # Header with count and clear button
        header = tk.Frame(self.inner, bg="#e3f2fd")
        header.pack(fill="x", padx=5, pady=(10, 5))
        tk.Label(header, text=f"{len(selected_objects)} objects selected",
                 bg="#e3f2fd", fg="#333333", font=("Segoe UI", 11, "bold")).pack(side="left")
        
        tk.Button(header, text="Clear All", command=on_clear_selection,
                  bg="#ff6666", fg="white", relief=tk.FLAT, padx=8).pack(side="right")

        # Group by type
        by_type: Dict[str, List[Dict]] = {}
        for obj in selected_objects:
            t = obj.get("data", obj).get("Type", "Unknown")
            if t not in by_type:
                by_type[t] = []
            by_type[t].append(obj)

        # Type groups with remove buttons
        for type_name, objs in sorted(by_type.items()):
            self._create_type_group(type_name, objs, on_remove_from_selection)
    
    def _create_type_group(
        self,
        type_name: str,
        objs: List[Dict],
        on_remove_from_selection: Callable[[Dict], None],
    ):
        """Create a collapsible group for objects of the same type."""
        type_frame = tk.Frame(self.inner, bg="#f0f0f0", relief=tk.FLAT, bd=1)
        type_frame.pack(fill="x", padx=5, pady=(5, 0))
        
        # Type header with remove-all button
        type_header = tk.Frame(type_frame, bg="#e0e0e0")
        type_header.pack(fill="x")
        tk.Label(type_header, text=f"{type_name} ({len(objs)})", bg="#e0e0e0", 
                 fg="#333333", font=("Segoe UI", 9, "bold")).pack(side="left", padx=5, pady=2)
        
        def remove_type():
            for o in list(objs):
                on_remove_from_selection(o)
        
        tk.Button(type_header, text="×", command=remove_type,
                  bg="#cc4444", fg="white", relief=tk.FLAT, width=2,
                  font=("Segoe UI", 9, "bold")).pack(side="right", padx=2, pady=2)

        # Individual items
        for obj in objs:
            item = obj.get("data", obj)
            item_frame = tk.Frame(type_frame, bg="#f0f0f0")
            item_frame.pack(fill="x", padx=5, pady=1)
            
            # Display Id or position
            display_text = item.get("Id", "")
            if not display_text:
                start = item.get("Start", {})
                display_text = f"({start.get('X', 0):.0f}, {start.get('Y', 0):.0f})"
            
            tk.Label(item_frame, text=display_text, bg="#f0f0f0", fg="#555555",
                     anchor="w", width=20).pack(side="left")
            
            def remove_item(o=obj):
                on_remove_from_selection(o)
            
            tk.Button(item_frame, text="×", command=remove_item,
                      bg="#999999", fg="white", relief=tk.FLAT, width=2,
                      font=("Segoe UI", 8)).pack(side="right")
    
    def _create_property_entries(self, item: Dict, type_def):
        """Create property entries for item based on type definition."""
        item_type = item.get("Type", "")
        
        # Id field only for Elevator and Cubicle types
        if item_type in ("Elevator", "Cubicle"):
            self._add_id_field(item)
        
        # Use get_properties() from type definition if available
        if hasattr(type_def, 'get_properties'):
            props = type_def.get_properties()
            for prop in props:
                if prop.prop_type == "section":
                    self._add_point_fields(prop.label, item.get(prop.key, {}), item, prop.key)
                elif prop.prop_type == "yaw":
                    self._add_yaw_field(item, prop.key)
                elif prop.prop_type == "float":
                    self._add_float_field(prop.label, item, prop.key, 
                                          getattr(prop, 'step', 10.0),
                                          getattr(prop, 'precision', 2))
                elif prop.prop_type == "int":
                    self._add_int_field(prop.label, item, prop.key,
                                        int(getattr(prop, 'minimum', 0)))
                elif prop.prop_type == "string":
                    self._add_string_field(prop.label, item, prop.key, "")
                elif prop.prop_type == "bool":
                    self._add_boolean_field(prop.label, item, prop.key)

    def _add_point_fields(self, label: str, point: Dict, item: Dict, key: str):
        """Add X/Y fields for a point with spinbox controls."""
        frame = tk.Frame(self.inner, bg="#fafafa")
        frame.pack(fill="x", padx=5, pady=2)
        tk.Label(frame, text=f"{label}:", bg="#fafafa", fg="#333333", width=12, anchor="w").pack(side="left")

        x_var = tk.DoubleVar(value=float(point.get("X", 0)))
        y_var = tk.DoubleVar(value=float(point.get("Y", 0)))

        def on_change(*_):
            try:
                item[key] = {"X": float(x_var.get()), "Y": float(y_var.get())}
                if self.get_lock_floor_ceiling() and item.get("Type") in ("Floor", "Ceiling"):
                    self.sync_floor_ceiling_partner(item)
                self.on_property_changed()
            except (ValueError, tk.TclError):
                pass

        tk.Label(frame, text="X:", bg="#fafafa", fg="#666666").pack(side="left", padx=(5, 0))
        x_spin = tk.Spinbox(frame, textvariable=x_var, from_=-100000, to=100000, increment=10,
                            width=7, bg="#ffffff", fg="#333333", buttonbackground="#e0e0e0",
                            command=on_change)
        x_spin.pack(side="left", padx=2)
        x_spin.bind("<Return>", on_change)
        x_spin.bind("<FocusOut>", on_change)

        tk.Label(frame, text="Y:", bg="#fafafa", fg="#666666").pack(side="left", padx=(5, 0))
        y_spin = tk.Spinbox(frame, textvariable=y_var, from_=-100000, to=100000, increment=10,
                            width=7, bg="#ffffff", fg="#333333", buttonbackground="#e0e0e0",
                            command=on_change)
        y_spin.pack(side="left", padx=2)
        y_spin.bind("<Return>", on_change)
        y_spin.bind("<FocusOut>", on_change)

    def _add_yaw_field(self, item: Dict, prop_name: str):
        """Add yaw rotation field with spinbox."""
        frame = tk.Frame(self.inner, bg="#fafafa")
        frame.pack(fill="x", padx=5, pady=2)
        tk.Label(frame, text="Yaw:", bg="#fafafa", fg="#333333", width=12, anchor="w").pack(side="left")

        var = tk.DoubleVar(value=float(item.get(prop_name, 0)))

        def on_change(*_):
            try:
                item[prop_name] = float(var.get())
                self.on_property_changed()
            except (ValueError, tk.TclError):
                pass

        spin = tk.Spinbox(frame, textvariable=var, from_=-360, to=360, increment=15,
                          width=8, bg="#ffffff", fg="#333333", buttonbackground="#e0e0e0",
                          command=on_change)
        spin.pack(side="left", padx=2)
        spin.bind("<Return>", on_change)
        spin.bind("<FocusOut>", on_change)

    def _add_id_field(self, item: Dict):
        """Add Id field."""
        frame = tk.Frame(self.inner, bg="#fafafa")
        frame.pack(fill="x", padx=5, pady=2)
        tk.Label(frame, text="Id:", bg="#fafafa", fg="#333333", width=12, anchor="w").pack(side="left")

        var = tk.StringVar(value=str(item.get("Id", "")))

        def on_change(*_):
            val = var.get().strip()
            if val:
                item["Id"] = val
            elif "Id" in item:
                del item["Id"]
            self.on_property_changed()

        ent = tk.Entry(frame, textvariable=var, width=16, bg="#ffffff", fg="#333333",
                       insertbackground="#333333")
        ent.pack(side="left", padx=2, fill="x", expand=True)
        ent.bind("<Return>", on_change)
        ent.bind("<FocusOut>", on_change)

    def _add_float_field(self, label: str, item: Dict, prop: str, step: float = 10.0, precision: int = 2):
        """Add a float field with spinbox."""
        frame = tk.Frame(self.inner, bg="#fafafa")
        frame.pack(fill="x", padx=5, pady=2)
        tk.Label(frame, text=f"{label}:", bg="#fafafa", fg="#333333", width=12, anchor="w").pack(side="left")

        var = tk.DoubleVar(value=round(float(item.get(prop, 0.0)), precision))

        def on_change(*_):
            try:
                item[prop] = float(var.get())
                self.on_property_changed()
            except (ValueError, tk.TclError):
                pass

        spin = tk.Spinbox(frame, textvariable=var, from_=-100000, to=100000, increment=step,
                          width=10, bg="#ffffff", fg="#333333", buttonbackground="#e0e0e0",
                          command=on_change)
        spin.pack(side="left", padx=2)
        spin.bind("<Return>", on_change)
        spin.bind("<FocusOut>", on_change)

    def _add_int_field(self, label: str, item: Dict, prop: str, default: int = 0):
        """Add an integer field with spinbox."""
        frame = tk.Frame(self.inner, bg="#fafafa")
        frame.pack(fill="x", padx=5, pady=2)
        tk.Label(frame, text=f"{label}:", bg="#fafafa", fg="#333333", width=12, anchor="w").pack(side="left")

        var = tk.IntVar(value=int(item.get(prop, default)))

        def on_change(*_):
            try:
                item[prop] = int(var.get())
                self.on_property_changed()
            except (ValueError, tk.TclError):
                pass

        spin = tk.Spinbox(frame, textvariable=var, from_=-100000, to=100000, increment=1,
                          width=8, bg="#ffffff", fg="#333333", buttonbackground="#e0e0e0",
                          command=on_change)
        spin.pack(side="left", padx=2)
        spin.bind("<Return>", on_change)
        spin.bind("<FocusOut>", on_change)

    def _add_string_field(self, label: str, item: Dict, prop: str, default: str = ""):
        """Add a string field."""
        frame = tk.Frame(self.inner, bg="#fafafa")
        frame.pack(fill="x", padx=5, pady=2)
        tk.Label(frame, text=f"{label}:", bg="#fafafa", fg="#333333", width=12, anchor="w").pack(side="left")

        var = tk.StringVar(value=str(item.get(prop, default)))

        def on_change(*_):
            val = var.get().strip()
            if val:
                item[prop] = val
            elif prop in item:
                del item[prop]
            self.on_property_changed()

        ent = tk.Entry(frame, textvariable=var, width=16, bg="#ffffff", fg="#333333",
                       insertbackground="#333333")
        ent.pack(side="left", padx=2, fill="x", expand=True)
        ent.bind("<Return>", on_change)
        ent.bind("<FocusOut>", on_change)

    def _add_boolean_field(self, label: str, item: Dict, prop: str):
        """Add a boolean checkbox field."""
        frame = tk.Frame(self.inner, bg="#fafafa")
        frame.pack(fill="x", padx=5, pady=2)

        var = tk.BooleanVar(value=bool(item.get(prop, False)))

        def on_change(*_):
            item[prop] = var.get()
            self.on_property_changed()

        cb = tk.Checkbutton(frame, text=label, variable=var, bg="#fafafa", fg="#333333",
                            selectcolor="#ffffff", activebackground="#fafafa",
                            activeforeground="#333333", command=on_change)
        cb.pack(side="left", padx=5)
