"""
File I/O operations for the Floorplan Editor.
"""

import json
import hashlib
import os
from typing import Dict, List, Any, Optional, Tuple

# Directory for temp working files (gitignored)
TEMP_DIR = os.path.join(os.path.dirname(__file__), ".temp_layouts")
SESSION_FILE = os.path.join(TEMP_DIR, ".session.json")


def get_temp_path(prod_path: str) -> str:
    """
    Get the temp file path for a given prod file.
    Uses a hash of the prod path to create a unique temp filename.
    """
    # Create temp directory if it doesn't exist
    os.makedirs(TEMP_DIR, exist_ok=True)
    
    # Create a unique filename based on prod path
    basename = os.path.basename(prod_path)
    path_hash = hashlib.md5(prod_path.encode()).hexdigest()[:8]
    temp_name = f".temp_{path_hash}_{basename}"
    return os.path.join(TEMP_DIR, temp_name)


def temp_exists(prod_path: str) -> bool:
    """Check if a temp file exists for the given prod path."""
    return os.path.exists(get_temp_path(prod_path))


def delete_temp(prod_path: str) -> bool:
    """Delete the temp file for a prod path. Returns True if deleted."""
    temp_path = get_temp_path(prod_path)
    if os.path.exists(temp_path):
        os.remove(temp_path)
        return True
    return False


def compute_content_hash(elements: List[Dict], root_json: Optional[Dict] = None) -> str:
    """Compute a hash of the layout content for comparison."""
    if root_json is not None:
        out = dict(root_json)
        out["Elements"] = elements
    else:
        out = elements
    content = json.dumps(out, sort_keys=True)
    return hashlib.md5(content.encode()).hexdigest()


def save_session(prod_path: str) -> None:
    """Save the current session (which prod file is open)."""
    os.makedirs(TEMP_DIR, exist_ok=True)
    try:
        with open(SESSION_FILE, "w") as f:
            json.dump({"prod_path": prod_path}, f)
    except Exception:
        pass  # Silent fail for session save


def load_session() -> Optional[str]:
    """Load the last session's prod file path."""
    if not os.path.exists(SESSION_FILE):
        return None
    try:
        with open(SESSION_FILE, "r") as f:
            data = json.load(f)
            return data.get("prod_path")
    except Exception:
        return None


def clear_session() -> None:
    """Clear the session file."""
    if os.path.exists(SESSION_FILE):
        try:
            os.remove(SESSION_FILE)
        except Exception:
            pass


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
