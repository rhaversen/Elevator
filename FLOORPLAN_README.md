# Floorplan Editor

A visual editor for creating and editing floor plans with walls, doors, windows, cubicles, ceiling lights, and spawn points.

## Features

### Object Types

- **Floor**: Defines the base area of the floor plan
- **Wall**: Solid walls with configurable thickness
- **Door**: Doors with visual indicator (dashed line with swing direction)
- **Window**: Windows with section count visualization (perpendicular lines show sections)
- **Cubicle**: Rectangular office cubicles with:
  - Configurable dimensions (X, Y)
  - Rotation support (0°, 90°, 180°, 270°)
  - Visual rotation indicator (corner triangle)
  - Dimension label display
- **CeilingLight**: Ceiling light grids with:
  - Spacing configuration (X, Y)
  - Padding configuration (X, Y)
  - Optional lamp position visualization
- **SpawnPoint**: Player spawn locations with:
  - Position marker (green circle)
  - Direction indicator (arrow showing yaw)
  - **Note**: Only one spawn point allowed; creating a new one removes the existing one

### Editing Features

#### Selection
- **Single Select**: Click on an object to select it
- **Multi-Select**: Hold Ctrl and click to select multiple objects
- **Select All**: Press Ctrl+A to select all editable objects

#### Movement
- **Drag**: Click and drag selected objects to move them
- **Snap to Grid**: Toggle grid snapping for precise positioning
- **Grid Size**: Adjustable grid size (10-2000 units)

#### Rotation
- **Rotate Clockwise**: Press 'r' or click "⟳ 90°" button
- **Rotate Counter-clockwise**: Press Shift+R or click "⟲ 90°" button
- Applies to all selected objects with Yaw property

#### Copy/Paste
- **Copy**: Press Ctrl+C to copy selected objects
- **Paste**: Press Ctrl+V to paste objects with automatic offset

#### Delete
- **Delete**: Press Delete or Backspace to remove selected objects
- Works with multiple selected objects

#### Undo/Redo
- **Undo**: Press Ctrl+Z to undo last action
- **Redo**: Press Ctrl+Y to redo previously undone action
- Maintains history of up to 50 actions

#### Properties Panel
- **Location**: Right side of the window
- **Edit Properties**: Select an object to view and edit its properties
- **Real-time Updates**: Changes apply immediately to the canvas
- **Supported Properties**:
  - Position (Start X/Y, End X/Y)
  - Dimensions (Width/Height for cubicles)
  - Rotation (Yaw angle in degrees)
  - Thickness (for walls/doors/windows)
  - Section Count (for windows)
  - Spacing and Padding (for ceiling lights)
  - Height Offset (for spawn points)

### View Controls

- **Pan**: Right-click or middle-click and drag to pan the view
- **Zoom**: Mouse wheel to zoom in/out
- **Reset View**: Click "Reset" to reset view to fit all content
- **Grid Display**: Toggle grid visibility and configure grid size

### Display Options

- **Show Grid**: Toggle grid line visibility
- **Show Lamps**: Toggle ceiling lamp position indicators for CeilingLight objects

## Usage

### Running the Editor

```bash
python3 floorplan.py
```

### Loading a File

1. Click **File > Open...**
2. Select a JSON layout file
3. The floor plan will be displayed

### Saving Changes

1. Click **File > Save As...**
2. Choose a location and filename
3. The layout will be saved in JSON format

### Creating Objects

1. Select a mode from the toolbar:
   - **Select**: Default mode for selecting and moving objects
   - **Cubicle**: Click to place cubicles
   - **Wall**: Click and drag to draw walls
   - **Door**: Click and drag to draw doors
   - **Window**: Click and drag to draw windows
   - **Spawn**: Click to place spawn points

2. For line objects (Wall, Door, Window):
   - Click once to start the line
   - Move the mouse to see a preview
   - Click again to finish the line
   - Or click and drag to create in one motion

### Keyboard Shortcuts

- **Pan**: Right/Middle mouse drag
- **Zoom**: Mouse wheel
- **Rotate**: R (clockwise) / Shift+R (counter-clockwise)
- **Multi-select**: Ctrl+Click
- **Copy**: Ctrl+C
- **Paste**: Ctrl+V
- **Select All**: Ctrl+A
- **Undo**: Ctrl+Z
- **Redo**: Ctrl+Y
- **Delete**: Delete or Backspace
- **Cancel**: Escape (cancel pending operations)

## JSON Format

The editor supports JSON files with the following structure:

```json
{
  "Elements": [
    {
      "Type": "Cubicle",
      "Start": {"X": 0.0, "Y": 0.0},
      "Dimensions": {"X": 300.0, "Y": 250.0},
      "Yaw": 0.0
    },
    {
      "Type": "CeilingLight",
      "Start": {"X": -1500.0, "Y": -800.0},
      "End": {"X": 1500.0, "Y": 800.0},
      "Spacing": {"X": 400.0, "Y": 700.0},
      "Padding": {"X": 200.0, "Y": 400.0}
    },
    {
      "Type": "Window",
      "Start": {"X": 0.0, "Y": 0.0},
      "End": {"X": 100.0, "Y": 100.0},
      "Thickness": 1.0,
      "SectionCount": 5
    },
    {
      "Type": "SpawnPoint",
      "Start": {"X": 0.0, "Y": 0.0},
      "HeightOffset": 100.0,
      "Yaw": 90.0
    }
  ]
}
```

## Testing

Run the test suite to verify functionality:

```bash
python3 test_floorplan.py
```

This tests:
- JSON loading and parsing
- Element type recognition
- Feature-specific attributes (Spacing, Padding, Dimensions, SectionCount, etc.)
- Copy/paste serialization

## UI Organization

The toolbar is organized into logical sections:

1. **Mode**: Select editing mode (Select, Cubicle, Wall, Door, Window, Spawn)
2. **View**: View controls (Reset, Zoom+, Zoom−)
3. **Rotate**: Rotation controls (⟳ 90°, ⟲ 90°)
4. **Grid**: Grid configuration (Snap, Size, Show)
5. **Display**: Display options (Lamps checkbox)

Status bar at bottom shows helpful keyboard shortcuts and operation feedback.
