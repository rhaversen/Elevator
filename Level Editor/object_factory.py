"""
Object Factory for the Floorplan Editor.

Provides functions for creating layout elements.
"""

from typing import Dict, List, Tuple, Optional, Callable


def create_wall(x0: float, y0: float, x1: float, y1: float) -> Dict:
    """Create a wall element."""
    return {
        "Type": "Wall",
        "Start": {"X": x0, "Y": y0},
        "End": {"X": x1, "Y": y1}
    }


def create_door(x0: float, y0: float, x1: float, y1: float) -> Dict:
    """Create a door element."""
    return {
        "Type": "Door",
        "Start": {"X": x0, "Y": y0},
        "End": {"X": x1, "Y": y1}
    }


def create_window(x0: float, y0: float, x1: float, y1: float) -> Dict:
    """Create a window element."""
    return {
        "Type": "Window",
        "Start": {"X": x0, "Y": y0},
        "End": {"X": x1, "Y": y1}
    }


def create_elevator(x0: float, y0: float, x1: float, y1: float) -> Dict:
    """Create an elevator element."""
    return {
        "Type": "Elevator",
        "Start": {"X": x0, "Y": y0},
        "End": {"X": x1, "Y": y1}
    }


def create_floor_ceiling_pair(
    x0: float, y0: float, x1: float, y1: float
) -> Tuple[Dict, Dict]:
    """
    Create a floor and ceiling pair.
    
    Returns:
        Tuple of (floor_dict, ceiling_dict)
    """
    floor = {
        "Type": "Floor",
        "Start": {"X": min(x0, x1), "Y": min(y0, y1)},
        "End": {"X": max(x0, x1), "Y": max(y0, y1)}
    }
    ceiling = {
        "Type": "Ceiling",
        "Start": {"X": min(x0, x1), "Y": min(y0, y1)},
        "End": {"X": max(x0, x1), "Y": max(y0, y1)}
    }
    return floor, ceiling


def create_ceiling_light(x0: float, y0: float, x1: float, y1: float) -> Dict:
    """Create a ceiling light element."""
    return {
        "Type": "CeilingLight",
        "Start": {"X": min(x0, x1), "Y": min(y0, y1)},
        "End": {"X": max(x0, x1), "Y": max(y0, y1)}
    }


def create_cubicle(x: float, y: float, yaw: float = 0, seed: int = 0) -> Dict:
    """Create a cubicle element."""
    return {
        "Type": "Cubicle",
        "Start": {"X": x, "Y": y},
        "Yaw": yaw,
        "Seed": seed
    }


def create_spawn_point(x: float, y: float, yaw: float = 0, player: bool = False) -> Dict:
    """Create a spawn point element."""
    return {
        "Type": "SpawnPoint",
        "Start": {"X": x, "Y": y},
        "Yaw": yaw,
        "Player": player
    }


def create_room_tone(x: float, y: float) -> Dict:
    """Create a room tone element."""
    return {
        "Type": "RoomTone",
        "Start": {"X": x, "Y": y}
    }


# Mode to factory function mapping
LINE_FACTORIES = {
    "add_wall": create_wall,
    "add_door": create_door,
    "add_window": create_window,
    "add_elevator": create_elevator,
}

RECT_FACTORIES = {
    "add_ceiling_light": create_ceiling_light,
    # Note: add_floor_ceiling handled specially due to pair creation
}

POINT_FACTORIES = {
    "add_cubicle": lambda x, y: create_cubicle(x, y),
    "add_spawn": lambda x, y: create_spawn_point(x, y),
    "add_roomtone": lambda x, y: create_room_tone(x, y),
}


def create_line_object(mode: str, x0: float, y0: float, x1: float, y1: float) -> Optional[Dict]:
    """Create a line-based object from mode."""
    factory = LINE_FACTORIES.get(mode)
    if factory:
        return factory(x0, y0, x1, y1)
    return None


def create_rect_object(mode: str, x0: float, y0: float, x1: float, y1: float) -> Optional[Dict]:
    """Create a rect-based object from mode."""
    factory = RECT_FACTORIES.get(mode)
    if factory:
        return factory(x0, y0, x1, y1)
    return None


def create_point_object(mode: str, x: float, y: float) -> Optional[Dict]:
    """Create a point-based object from mode."""
    factory = POINT_FACTORIES.get(mode)
    if factory:
        return factory(x, y)
    return None
