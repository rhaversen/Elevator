# RoomTone Audio Source - Implementation Summary

## Overview

RoomTone objects represent audio sources in the floorplan, such as ambient sounds, room tones, or environmental audio. They can be omnidirectional (spreading sound in all directions) or directional (projecting sound in a specific direction).

## Changes Made (Commit 76239b0)

### Visual Representation

**Omnidirectional RoomTone:**
- Red filled circle at center position (10px radius)
- Three curved arc lines radiating outward (sound waves)
- Light blue dashed circle showing attenuation radius
- Darker blue dashed circle showing source radius

**Directional RoomTone:**
- Red cone shape pointing in direction
- Same radius circles as omnidirectional
- Cone indicates directionality

**Radius Visualization:**
- **Attenuation Radius** (light blue, dashed): Maximum distance where audio can be heard with falloff
- **Source Radius** (darker blue, dashed): Core area where audio is at full volume

### Properties

| Property | Type | Description | Default |
|----------|------|-------------|---------|
| Start | Position (X, Y) | World position of audio source | Required |
| HeightOffset | Float | Vertical offset from floor level | 150.0 |
| AudioId | String | Identifier for the audio asset | "NewRoomTone" |
| bOmnidirectional | Boolean | Whether sound radiates in all directions | true |
| AttenuationRadius | Float | Maximum hearing distance with falloff | 1000.0 |
| SourceRadius | Float | Core audio source size | 100.0 |
| VolumeMultiplier | Float | Volume scaling factor | 1.0 |

### UI Integration

**Toolbar:**
- Added "RoomTone" mode button to Mode section
- Click on canvas to place new RoomTone at that location

**Properties Panel:**
- All properties editable in real-time
- String field for AudioId
- Boolean checkbox for bOmnidirectional
- Spinbox controls for numeric values
- 300ms debounce on all changes

**Selection & Manipulation:**
- Click to select (red highlight)
- Drag to move
- Copy/paste support (Ctrl+C/V)
- Undo/redo support (Ctrl+Z/Y)
- Multi-select support (Ctrl+Click)

### Code Implementation

#### Visualization Code
```python
elif t == "RoomTone":
    # Get properties
    is_omni = item.get("bOmnidirectional", True)
    attenuation_radius = float(item.get("AttenuationRadius", 1000.0))
    source_radius = float(item.get("SourceRadius", 100.0))
    
    # Draw attenuation radius circle (light blue, dashed)
    if attenuation_radius > 0:
        ar_circle = self.canvas.create_oval(...)
    
    # Draw source radius circle (darker blue)
    if source_radius > 0:
        sr_circle = self.canvas.create_oval(...)
    
    # Draw speaker icon
    if is_omni:
        # Omnidirectional: circle with sound waves
        speaker = self.canvas.create_oval(...)
        for i in range(1, 4):
            arc = self.canvas.create_arc(...)  # Sound waves
    else:
        # Directional: cone shape
        cone = self.canvas.create_polygon(...)
```

#### Properties Panel Code
```python
elif item_type == "RoomTone":
    if "HeightOffset" in item:
        self._add_float_property("HeightOffset", item, "HeightOffset")
    if "AudioId" in item:
        self._add_string_property("AudioId", item, "AudioId")
    if "bOmnidirectional" in item:
        self._add_bool_property("Omnidirectional", item, "bOmnidirectional")
    # ... etc
```

#### Adding New RoomTone
```python
def add_roomtone_at(self, wx, wy):
    self.save_state()  # For undo
    wx, wy = self.snap_point(wx, wy)
    item = {
        "Type": "RoomTone",
        "Start": {"X": wx, "Y": wy},
        "HeightOffset": 150.0,
        "AudioId": "NewRoomTone",
        "bOmnidirectional": True,
        "AttenuationRadius": 1000.0,
        "SourceRadius": 100.0,
        "VolumeMultiplier": 1.0
    }
    self.data.append(item)
    obj = self.draw_item(item)
    if obj is not None:
        self.set_selected(obj)
```

### Example JSON

```json
{
  "Type": "RoomTone",
  "Start": { "X": 0.0, "Y": 0.0 },
  "HeightOffset": 150.0,
  "AudioId": "Office",
  "bOmnidirectional": true,
  "AttenuationRadius": 2000.0,
  "SourceRadius": 500.0,
  "VolumeMultiplier": 1.0
}
```

**Omnidirectional Example:**
- Ambient office noise
- Radiates in all directions
- Large attenuation radius (2000 units)
- Moderate source radius (500 units)

```json
{
  "Type": "RoomTone",
  "Start": { "X": -2050.0, "Y": 500.0 },
  "HeightOffset": 150.0,
  "AudioId": "Annex",
  "bOmnidirectional": false,
  "AttenuationRadius": 500.0,
  "SourceRadius": 100.0,
  "VolumeMultiplier": 1.0
}
```

**Directional Example:**
- Localized annex sound
- Directional (cone shape in visualization)
- Smaller attenuation radius (500 units)
- Smaller source radius (100 units)

### Visual Legend

```
Omnidirectional:              Directional:
     ))) (sound waves)             ▶ (cone)
    (●●●) speaker                 ▶▶ direction
   ┄┄┄┄┄┄┄ source radius        ┄┄┄┄┄┄┄
  ┈┈┈┈┈┈┈┈┈ attenuation        ┈┈┈┈┈┈┈┈┈
```

### Helper Methods Added

**_add_string_property(label, item, key)**
- Text entry field with debounced updates
- Used for AudioId

**_add_bool_property(label, item, key)**
- Checkbox control with immediate updates
- Used for bOmnidirectional

Both methods:
- Follow the same pattern as existing property methods
- Include undo state saving
- Trigger canvas rebuild on change
- Handle errors gracefully

### Styling

**Normal State:**
- Speaker icon: #ff6b6b (red)
- Outline: #c92a2a (dark red)
- Attenuation radius: #87ceeb (light blue)
- Source radius: #4169e1 (darker blue)

**Selected State:**
- Speaker icon: #ffcccc (light red)
- Outline: red
- Radius circles remain same colors

### Integration Points

Updated to include RoomTone:
1. **Toolbar mode selection** - Added "RoomTone" button
2. **find_object_at()** - Includes "RoomTone" in clickable types
3. **on_select_all()** - Includes "RoomTone" in selectable types
4. **style_object()** - Added RoomTone styling rules
5. **update_properties_panel()** - Added RoomTone property section

### Testing

All existing tests pass:
- ✅ JSON loading and parsing
- ✅ Element type recognition
- ✅ Copy/paste serialization
- ✅ No security vulnerabilities (CodeQL)

### Use Cases

1. **Ambient Sound Placement**
   - Place omnidirectional RoomTone for general ambience
   - Adjust attenuation radius to control falloff
   - Set AudioId to reference audio asset

2. **Localized Audio**
   - Use directional RoomTone for speakers, vents, etc.
   - Adjust source radius for core audio area
   - Use VolumeMultiplier to balance volumes

3. **Audio Zones**
   - Multiple RoomTones can overlap
   - Different AudioIds for different areas
   - Visual radius helps plan coverage

### Future Enhancements

Possible improvements:
1. Direction arrow for directional RoomTones (like SpawnPoint)
2. Visual volume indicator (brightness or size)
3. Audio preview/testing from editor
4. Audio zone visualization (overlap areas)
5. Automatic spacing suggestions
6. Import audio asset list from project

---

**Summary**: RoomTone objects provide a complete audio source placement and visualization system, with full property editing and integration with all existing editor features.
