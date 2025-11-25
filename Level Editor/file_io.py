"""
File I/O operations for the Floorplan Editor.
"""

import json
import os
from typing import Dict, List, Any, Optional, Tuple


def load_layout_file(path: str) -> Tuple[Optional[Dict], List[Dict], Optional[str]]:
    """
    Load a layout JSON file.
    
    Returns:
        (root_json, elements, error_message)
        - root_json: The root JSON object if it's a dict with "Elements", else None
        - elements: The list of layout elements
        - error_message: Error string if loading failed, else None
    """
    try:
        with open(path, "r") as f:
            root = json.load(f)
        
        if isinstance(root, dict) and "Elements" in root:
            elements = root["Elements"]
            if not isinstance(elements, list):
                return None, [], '"Elements" must be a list'
            return root, elements, None
        elif isinstance(root, list):
            return None, root, None
        else:
            return None, [], "Expected a JSON array or an object with 'Elements'"
    except Exception as e:
        return None, [], str(e)


def save_layout_file(
    path: str,
    elements: List[Dict],
    root_json: Optional[Dict] = None,
) -> Optional[str]:
    """
    Save a layout to a JSON file.
    
    Args:
        path: File path to save to
        elements: The list of layout elements
        root_json: Original root JSON object (to preserve other fields)
    
    Returns:
        Error message if saving failed, else None
    """
    try:
        if root_json is not None:
            out = dict(root_json)
            out["Elements"] = elements
        else:
            out = elements
        
        with open(path, "w") as f:
            json.dump(out, f, indent=2)
        
        return None
    except Exception as e:
        return str(e)


def get_display_name(path: str) -> str:
    """Get the display name (filename) from a path."""
    return os.path.basename(path)
