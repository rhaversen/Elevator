"""
Editor state management for the Floorplan Editor.

Handles:
- Selection state
- Undo/redo history
- Clipboard operations
- Floor/Ceiling linking
"""

import copy
import json
from typing import Dict, List, Any, Optional, Tuple, Set


class EditorState:
    """Manages editor state including selection, history, and clipboard."""
    
    def __init__(self, max_undo: int = 50):
        self.max_undo = max_undo
        
        # Data
        self.data: List[Dict] = []
        self.root_json: Optional[Dict] = None
        self.current_file_path: Optional[str] = None
        
        # Selection
        self.selected_objects: List[Dict] = []  # List of canvas object dicts
        self.primary_selected: Optional[Dict] = None  # Primary selection for properties
        
        # History
        self.undo_stack: List[str] = []  # JSON snapshots
        self.redo_stack: List[str] = []
        
        # Clipboard
        self.clipboard: List[Dict] = []
        
        # Floor/Ceiling linking
        self.floor_ceiling_links: Dict[int, Dict] = {}  # id(item) -> partner item
        self.lock_floor_ceiling: bool = True
    
    def clear(self):
        """Clear all state."""
        self.data = []
        self.root_json = None
        self.selected_objects = []
        self.primary_selected = None
        self.undo_stack = []
        self.redo_stack = []
        self.floor_ceiling_links = {}
    
    def set_data(self, data: List[Dict], root_json: Optional[Dict] = None):
        """Set the layout data."""
        self.data = data
        self.root_json = root_json
        self.clear_selection()
        self.undo_stack = []
        self.redo_stack = []
        self.refresh_floor_ceiling_links()
    
    # =========================================================================
    # Selection
    # =========================================================================
    
    def clear_selection(self):
        """Clear all selection."""
        self.selected_objects = []
        self.primary_selected = None
    
    def is_selected(self, obj: Dict) -> bool:
        """Check if an object is selected."""
        return obj in self.selected_objects
    
    def select(self, obj: Dict, multi: bool = False):
        """Select an object, optionally adding to multi-selection."""
        if multi:
            if obj in self.selected_objects:
                # Toggle off
                self.selected_objects.remove(obj)
                if self.primary_selected is obj:
                    self.primary_selected = self.selected_objects[0] if self.selected_objects else None
            else:
                self.selected_objects.append(obj)
                if self.primary_selected is None:
                    self.primary_selected = obj
        else:
            self.selected_objects = [obj]
            self.primary_selected = obj
    
    def select_multiple(self, objects: List[Dict]):
        """Select multiple objects."""
        self.selected_objects = list(objects)
        self.primary_selected = objects[0] if objects else None
    
    def deselect(self, obj: Dict):
        """Remove an object from selection."""
        if obj in self.selected_objects:
            self.selected_objects.remove(obj)
            if self.primary_selected is obj:
                self.primary_selected = self.selected_objects[0] if self.selected_objects else None
    
    # =========================================================================
    # Undo/Redo
    # =========================================================================
    
    def save_state(self):
        """Save current state for undo."""
        import json
        snapshot = json.dumps(self.data)
        self.undo_stack.append(snapshot)
        if len(self.undo_stack) > self.max_undo:
            self.undo_stack.pop(0)
        self.redo_stack.clear()
    
    def undo(self) -> bool:
        """Undo last action. Returns True if undo was performed."""
        if not self.undo_stack:
            return False
        
        import json
        # Save current state to redo
        current = json.dumps(self.data)
        self.redo_stack.append(current)
        
        # Restore previous state
        snapshot = self.undo_stack.pop()
        self.data.clear()
        self.data.extend(json.loads(snapshot))
        
        self.clear_selection()
        self.refresh_floor_ceiling_links()
        return True
    
    def redo(self) -> bool:
        """Redo last undone action. Returns True if redo was performed."""
        if not self.redo_stack:
            return False
        
        import json
        # Save current state to undo
        current = json.dumps(self.data)
        self.undo_stack.append(current)
        
        # Restore redo state
        snapshot = self.redo_stack.pop()
        self.data.clear()
        self.data.extend(json.loads(snapshot))
        
        self.clear_selection()
        self.refresh_floor_ceiling_links()
        return True
    
    # =========================================================================
    # Clipboard
    # =========================================================================
    
    def copy_selected(self) -> int:
        """Copy selected objects to clipboard. Returns count of copied items."""
        if not self.selected_objects:
            return 0
        
        self.clipboard = []
        for obj in self.selected_objects:
            item = obj.get("data", obj)
            self.clipboard.append(copy.deepcopy(item))
        
        return len(self.clipboard)
    
    def paste(self, offset: Tuple[float, float] = (50.0, 50.0)) -> List[Dict]:
        """Paste clipboard contents with offset. Returns list of new items."""
        if not self.clipboard:
            return []
        
        self.save_state()
        new_items = []
        
        for item in self.clipboard:
            new_item = copy.deepcopy(item)
            
            # Offset position
            if "Start" in new_item:
                new_item["Start"]["X"] = new_item["Start"].get("X", 0.0) + offset[0]
                new_item["Start"]["Y"] = new_item["Start"].get("Y", 0.0) + offset[1]
            if "End" in new_item:
                new_item["End"]["X"] = new_item["End"].get("X", 0.0) + offset[0]
                new_item["End"]["Y"] = new_item["End"].get("Y", 0.0) + offset[1]
            
            # Remove Id to avoid duplicates
            if "Id" in new_item:
                del new_item["Id"]
            
            self.data.append(new_item)
            new_items.append(new_item)
        
        return new_items
    
    # =========================================================================
    # Floor/Ceiling Linking
    # =========================================================================
    
    def refresh_floor_ceiling_links(self):
        """Rebuild floor/ceiling link mappings based on matching coordinates."""
        self.floor_ceiling_links.clear()
        
        floors = [item for item in self.data if item.get("Type") == "Floor"]
        ceilings = [item for item in self.data if item.get("Type") == "Ceiling"]
        
        for floor in floors:
            fs = floor.get("Start", {})
            fe = floor.get("End", {})
            fx0, fy0 = float(fs.get("X", 0)), float(fs.get("Y", 0))
            fx1, fy1 = float(fe.get("X", 0)), float(fe.get("Y", 0))
            
            for ceiling in ceilings:
                if id(ceiling) in self.floor_ceiling_links:
                    continue  # Already linked
                
                cs = ceiling.get("Start", {})
                ce = ceiling.get("End", {})
                cx0, cy0 = float(cs.get("X", 0)), float(cs.get("Y", 0))
                cx1, cy1 = float(ce.get("X", 0)), float(ce.get("Y", 0))
                
                if (abs(fx0 - cx0) < 1e-6 and abs(fy0 - cy0) < 1e-6 and
                    abs(fx1 - cx1) < 1e-6 and abs(fy1 - cy1) < 1e-6):
                    self.floor_ceiling_links[id(floor)] = ceiling
                    self.floor_ceiling_links[id(ceiling)] = floor
                    break
    
    def get_floor_ceiling_partner(self, item: Dict) -> Optional[Dict]:
        """Get the linked Floor/Ceiling partner for an item."""
        return self.floor_ceiling_links.get(id(item))
    
    def link_floor_ceiling(self, floor: Dict, ceiling: Dict):
        """Create a Floor/Ceiling link."""
        self.floor_ceiling_links[id(floor)] = ceiling
        self.floor_ceiling_links[id(ceiling)] = floor
    
    def unlink_floor_ceiling(self, item: Dict):
        """Remove Floor/Ceiling link for an item."""
        partner = self.floor_ceiling_links.pop(id(item), None)
        if partner is not None:
            self.floor_ceiling_links.pop(id(partner), None)
    
    def sync_floor_ceiling_partner(self, item: Dict):
        """Sync partner's coordinates to match item."""
        partner = self.get_floor_ceiling_partner(item)
        if partner is None:
            return
        
        partner["Start"] = dict(item.get("Start", {}))
        partner["End"] = dict(item.get("End", {}))
    
    def ensure_floor_ceiling_pairs(self):
        """Ensure all Floors have matching Ceilings and vice versa."""
        floors = [item for item in self.data if item.get("Type") == "Floor"]
        ceilings = [item for item in self.data if item.get("Type") == "Ceiling"]
        
        for floor in floors:
            if id(floor) in self.floor_ceiling_links:
                continue
            
            # Create matching ceiling
            ceiling = {
                "Type": "Ceiling",
                "Start": dict(floor.get("Start", {})),
                "End": dict(floor.get("End", {})),
            }
            self.data.append(ceiling)
            self.link_floor_ceiling(floor, ceiling)
        
        for ceiling in ceilings:
            if id(ceiling) in self.floor_ceiling_links:
                continue
            
            # Create matching floor
            floor = {
                "Type": "Floor",
                "Start": dict(ceiling.get("Start", {})),
                "End": dict(ceiling.get("End", {})),
            }
            self.data.append(floor)
            self.link_floor_ceiling(floor, ceiling)
    
    # =========================================================================
    # Delete
    # =========================================================================
    
    def delete_item(self, item: Dict) -> bool:
        """Delete an item from the data. Returns True if deleted."""
        if item in self.data:
            self.data.remove(item)
            self.unlink_floor_ceiling(item)
            return True
        return False
    
    def delete_selected(self) -> int:
        """Delete all selected items. Returns count of deleted items."""
        if not self.selected_objects:
            return 0
        
        self.save_state()
        count = 0
        
        for obj in list(self.selected_objects):
            item = obj.get("data", obj)
            if self.delete_item(item):
                count += 1
        
        self.clear_selection()
        return count
