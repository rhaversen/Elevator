# CeilingLight Line-Based Generation - Technical Summary

## Overview

CeilingLights now generate lights in parallel lines along a rotated axis, controlled by the Yaw parameter. This provides a more realistic and flexible lighting pattern compared to the previous grid-based approach.

## Changes Made (Commit f7f33f3)

### New Behavior

**Line-Based Generation:**
- Lights are placed along parallel lines
- Lines run along a rotated axis defined by Yaw
- Yaw = 0° → lines run horizontally (along X-axis)
- Yaw = 90° → lines run vertically (along Y-axis)
- Any angle supported

**Parameters:**
1. **Spacing.X** - Distance between lights along each line
2. **Spacing.Y** - Distance between parallel lines (line width)
3. **Padding.X** - Padding along the line direction
4. **Padding.Y** - Padding perpendicular to lines
5. **Yaw** - Rotation angle of the line grid (0-360°)

### Visual Representation

The visualization now shows:
1. **Bounding box** - Dashed orange rectangle (Start → End)
2. **Guide lines** - Dashed orange lines showing the structure
3. **Lights** - Yellow circles placed along the lines
4. **Spacing visualization** - Clear gaps between lines and between lights

### Implementation Details

#### Coordinate System Transformation

```python
# Convert yaw to radians
yaw_rad = math.radians(yaw)

# Direction vectors
line_dir_x = math.cos(yaw_rad)    # Along line direction
line_dir_y = math.sin(yaw_rad)

perp_dir_x = -math.sin(yaw_rad)   # Perpendicular to lines
perp_dir_y = math.cos(yaw_rad)
```

#### Padding Application

Padding is applied in the rotated coordinate system:
- `half_w = width / 2 - pad_x` (along line direction)
- `half_h = height / 2 - pad_y` (perpendicular to lines)

This ensures padding works correctly regardless of rotation angle.

#### Line Generation

```python
# Number of lines perpendicular to yaw
num_lines = int(2 * half_h / space_between_lines) + 1

for line_idx in range(num_lines):
    # Position along perpendicular axis
    perp_offset = -half_h + line_idx * space_between_lines
    
    # Calculate line endpoints in world coordinates
    line_start = center + (-half_w * line_dir) + (perp_offset * perp_dir)
    line_end = center + (half_w * line_dir) + (perp_offset * perp_dir)
    
    # Draw guide line
    # ...
    
    # Place lamps along this line
    num_lamps = int(line_length / space_along_line) + 1
    for lamp_idx in range(num_lamps):
        along_offset = -half_w + lamp_idx * space_along_line
        lamp_pos = center + (along_offset * line_dir) + (perp_offset * perp_dir)
        # Draw lamp
```

### Properties Panel Integration

**Added Yaw Support for CeilingLight:**
```python
elif item_type == "CeilingLight":
    if "Spacing" in item:
        self._add_property_section("Spacing", item["Spacing"], ["X", "Y"])
    if "Padding" in item:
        self._add_property_section("Padding", item["Padding"], ["X", "Y"])
    if "Yaw" in item:
        self._add_yaw_property(item)
    elif item.get("Yaw") is None:
        # Add Yaw if it doesn't exist (default to 0)
        item["Yaw"] = 0.0
        self._add_yaw_property(item)
```

**Features:**
- Spinbox control for Yaw (0-360°, increment by 15°)
- Rotation buttons (⟳ 90°, ⟲ 90°)
- Keyboard shortcuts work (R for +90°, Shift+R for -90°)
- 300ms debounce for smooth editing
- Real-time preview with canvas rebuild

### Rotation Support

CeilingLights can now be rotated using:
1. **Keyboard**: R (clockwise 90°), Shift+R (counter-clockwise 90°)
2. **Properties Panel**: Rotation buttons
3. **Properties Panel**: Direct Yaw value editing
4. **Toolbar**: Rotate buttons when CeilingLight is selected

The `can_rotate(item)` function checks for "Yaw" in item, which now includes CeilingLights.

## Example JSON

```json
{
  "Type": "CeilingLight",
  "Start": { "X": -1600.0, "Y": 900.0 },
  "End": { "X": -2500.0, "Y": 500.0 },
  "Spacing": { "X": 300.0, "Y": 300.0 },
  "Padding": { "X": 100.0, "Y": 100.0 },
  "Yaw": 0.0
}
```

**Interpretation:**
- Bounding box: (-1600, 900) to (-2500, 500)
- Area: 900 units wide × 400 units tall
- Lines run at 0° (horizontal, along X-axis)
- Lights every 300 units along each line
- Lines every 300 units apart
- 100 units padding on all sides (in rotated coordinates)

**With Yaw = 45°:**
- Same bounding box
- Lines now run at 45° angle
- Lights still 300 units apart along each line
- Lines still 300 units apart
- Padding of 100 units applied along rotated axes

## Visual Comparison

### Before (Grid-Based)
```
┌────────────────────────┐
│ ●   ●   ●   ●   ●   ● │
│                        │
│ ●   ●   ●   ●   ●   ● │
│                        │
│ ●   ●   ●   ●   ●   ● │
└────────────────────────┘
```
Lights in a grid pattern, always aligned with X/Y axes.

### After (Line-Based, Yaw = 0°)
```
┌────────────────────────┐
│─●───●───●───●───●───●─│
│─●───●───●───●───●───●─│
│─●───●───●───●───●───●─│
└────────────────────────┘
```
Lights on parallel lines, with visible line guides.

### After (Line-Based, Yaw = 45°)
```
┌────────────────────────┐
│  ╱●──●──●──●──●╱       │
│ ╱●──●──●──●──●╱        │
│╱●──●──●──●──●╱         │
└────────────────────────┘
```
Lines rotated 45°, lights follow the rotation.

## Benefits

1. **More realistic** - Matches real-world fluorescent light installations
2. **Flexible** - Any rotation angle supported
3. **Clear visualization** - Guide lines show structure
4. **Easy to edit** - Yaw parameter in properties panel
5. **Consistent** - Uses same rotation system as Cubicles and SpawnPoints
6. **Performant** - Efficient calculation using trigonometry

## Testing

All tests pass:
- ✅ JSON loading with CeilingLight elements
- ✅ Spacing and Padding properties read correctly
- ✅ Visualization works with and without Yaw parameter
- ✅ Rotation controls work correctly
- ✅ Properties panel updates properly

## Future Enhancements

Possible improvements:
1. Visual handles to adjust line spacing in canvas
2. Preview of rotation during dragging
3. Snap to common angles (0°, 45°, 90°, etc.)
4. Support for curved or arc-based light patterns
5. Intensity visualization based on light density

---

**Summary**: CeilingLights now generate in a much more realistic line-based pattern with full rotation support, making it easy to create professional lighting layouts at any angle.
