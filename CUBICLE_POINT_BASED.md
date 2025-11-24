# Cubicle Point-Based Format

## Overview

Cubicles have been simplified to point-based objects in the JSON format. They no longer store `Dimensions` - instead, they are purely positional markers with rotation. Display dimensions are configured in the UI and are not saved to files.

## JSON Format

### New Format (Current)
```json
{
  "Type": "Cubicle",
  "Start": { "X": 600.0, "Y": 250.0 },
  "Yaw": 0.0
}
```

### Old Format (Removed)
```json
{
  "Type": "Cubicle",
  "Start": { "X": 600.0, "Y": 250.0 },
  "Dimensions": { "X": 300.0, "Y": 250.0 },  // ❌ No longer used
  "Yaw": 0.0
}
```

## UI-Only Display Dimensions

### Configuration Controls

Located in the **Display** section of the toolbar:
- **Cubicle W:** Width spinbox (50-1000 units, default 300.0)
- **Cubicle D:** Depth spinbox (50-1000 units, default 250.0)

### Behavior

1. **Not Saved**: Display dimensions are NEVER written to JSON files
2. **Not Loaded**: When loading JSON, any `Dimensions` in Cubicle objects are ignored
3. **Global Setting**: All Cubicles use the same display dimensions
4. **Live Update**: Changing display dimensions immediately updates all Cubicles on canvas

### Implementation Details

```python
# Configuration variables (UI state only)
self.cubicle_display_width = tk.DoubleVar(value=300.0)
self.cubicle_display_depth = tk.DoubleVar(value=250.0)

# When drawing Cubicles, use display dimensions
display_width = float(self.cubicle_display_width.get())
display_depth = float(self.cubicle_display_depth.get())
dim = {"X": display_width, "Y": display_depth}
w_world, h_world = self.get_axis_size(dim, yaw)
```

## Rationale

### Why Point-Based?

1. **Logical Representation**: In the game engine, Cubicles represent specific points/locations where furniture spawns
2. **Actual Dimensions**: The game engine determines actual cubicle dimensions during spawning
3. **Cleaner Data**: JSON files contain only essential positioning data
4. **Consistent Workflow**: Similar to SpawnPoint and RoomTone (also point-based)

### Why UI-Only Display?

1. **Visualization Need**: Users need to see approximate cubicle footprint when placing them
2. **Flexibility**: Different users may prefer different display sizes for clarity
3. **No Data Pollution**: Display preferences don't clutter the data files
4. **User Control**: Adjustable without affecting saved data or other users

## Visual Representation

```
Display Width = 300.0, Depth = 250.0:

    ┌──────────────┐  ← 300.0 units wide
    │              │
    │      ●───→   │  ← Center with Yaw arrow
    │              │
    └──────────────┘
         250.0 units deep
         
Yaw = 0°: Arrow points right (default orientation)
Yaw = 90°: Arrow points down
Yaw = 180°: Arrow points left
Yaw = 270°: Arrow points up
```

## Rotation Behavior

When rotating a Cubicle:
1. Calculate current center using display dimensions
2. Apply rotation (Yaw changes by ±90°)
3. Recalculate Start position to keep center fixed
4. Display dimensions affect visual footprint during rotation

```python
# Rotation preserves center point
center_x = start_x + width_old / 2
center_y = start_y + height_old / 2
# ... rotate yaw ...
new_start_x = center_x - width_new / 2
new_start_y = center_y - height_new / 2
```

## Properties Panel

Cubicles in the properties panel show:
- **Start Position**: X and Y coordinates (editable)
- **Yaw**: Rotation angle (editable, 0-360°)
- **No Dimensions**: Removed from properties (use toolbar controls)

## Migration

### Loading Old Files

Files with old format (containing `Dimensions`) will:
- Load successfully
- `Dimensions` property ignored
- Display using current UI display dimensions
- Save in new format (without `Dimensions`)

### Example Migration

**Before (loaded):**
```json
{
  "Type": "Cubicle",
  "Start": { "X": 600.0, "Y": 250.0 },
  "Dimensions": { "X": 300.0, "Y": 250.0 },
  "Yaw": 0.0
}
```

**After (saved):**
```json
{
  "Type": "Cubicle",
  "Start": { "X": 600.0, "Y": 250.0 },
  "Yaw": 0.0
}
```

## Best Practices

1. **Adjust Display Dimensions**: Set comfortable display size for your viewport scale
2. **Consistent Placement**: Use grid snapping for precise cubicle positioning
3. **Visual Clarity**: Increase display dimensions if Cubicles are too small to see
4. **Performance**: Display dimensions don't affect file size or performance

## Technical Implementation

### Key Changes

1. **Creation**: `add_cubicle_at()` no longer adds `Dimensions`
2. **Drawing**: `draw_item()` uses `self.cubicle_display_width/depth` instead of `item["Dimensions"]`
3. **Rotation**: Uses display dimensions for pivot calculation
4. **Bounding Box**: Calculates using display dimensions for viewport fitting
5. **Properties**: Removed `Dimensions` section from properties panel

### Code Locations

- **Config Variables**: Line 46-48 in `__init__()`
- **UI Controls**: Line 146-151 in toolbar setup
- **Drawing Logic**: Line 761+ in `draw_item()`
- **Rotation Logic**: Line 1473+ in `rotate_item()`
- **Properties Panel**: Line 1123+ (Dimensions section removed)

## Examples

### Example 1: Office Cubicles
```json
{
  "Elements": [
    {"Type": "Cubicle", "Start": {"X": 0.0, "Y": 0.0}, "Yaw": 0.0},
    {"Type": "Cubicle", "Start": {"X": 400.0, "Y": 0.0}, "Yaw": 0.0},
    {"Type": "Cubicle", "Start": {"X": 0.0, "Y": 400.0}, "Yaw": 180.0},
    {"Type": "Cubicle", "Start": {"X": 400.0, "Y": 400.0}, "Yaw": 180.0}
  ]
}
```

### Example 2: Mixed Orientations
```json
{
  "Elements": [
    {"Type": "Cubicle", "Start": {"X": 600.0, "Y": 250.0}, "Yaw": 0.0},
    {"Type": "Cubicle", "Start": {"X": 0.0, "Y": -300.0}, "Yaw": 90.0},
    {"Type": "Cubicle", "Start": {"X": -500.0, "Y": 100.0}, "Yaw": 270.0}
  ]
}
```

## Summary

**Key Points:**
- ✅ Cubicles are point-based (Start + Yaw only)
- ✅ Display dimensions configurable in UI
- ✅ Display dimensions NOT saved to JSON
- ✅ All Cubicles use same display dimensions
- ✅ Cleaner data format
- ✅ Flexible visualization
- ✅ Backward compatible (ignores old Dimensions)
