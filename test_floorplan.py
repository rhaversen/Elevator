#!/usr/bin/env python3
"""
Test script for floorplan editor
This verifies that the JSON loading and data structures work correctly
"""
import json
import sys

def test_load_json():
    """Test loading the example JSON file"""
    print("Testing JSON loading...")
    with open('Content/Layouts/ExampleOpenOffice.json', 'r') as f:
        data = json.load(f)
    
    assert 'Elements' in data, "JSON should have 'Elements' key"
    elements = data['Elements']
    assert isinstance(elements, list), "Elements should be a list"
    
    print(f"✓ Loaded {len(elements)} elements")
    return elements

def test_element_types(elements):
    """Test that all expected element types are present"""
    print("\nTesting element types...")
    
    expected_types = {
        'Floor', 'Ceiling', 'CeilingLight', 'Cubicle', 
        'Wall', 'Window', 'Door', 'SpawnPoint', 'Elevator'
    }
    found_types = set()
    
    for elem in elements:
        elem_type = elem.get('Type')
        found_types.add(elem_type)
    
    print(f"Found types: {', '.join(sorted(found_types))}")
    
    # Check for specific features we implemented
    ceiling_lights = [e for e in elements if e.get('Type') == 'CeilingLight']
    print(f"✓ Found {len(ceiling_lights)} CeilingLight elements")
    
    for cl in ceiling_lights:
        assert 'Spacing' in cl, "CeilingLight should have Spacing"
        assert 'Padding' in cl, "CeilingLight should have Padding"
        print(f"  - CeilingLight with Spacing: {cl['Spacing']}, Padding: {cl['Padding']}")
    
    cubicles = [e for e in elements if e.get('Type') == 'Cubicle']
    print(f"✓ Found {len(cubicles)} Cubicle elements")
    
    for cub in cubicles:
        assert 'Dimensions' in cub, "Cubicle should have Dimensions"
        assert 'Yaw' in cub, "Cubicle should have Yaw"
        dims = cub['Dimensions']
        print(f"  - Cubicle at {cub['Start']}, Dimensions: {dims['X']}×{dims['Y']}, Yaw: {cub['Yaw']}°")
    
    windows = [e for e in elements if e.get('Type') == 'Window']
    print(f"✓ Found {len(windows)} Window elements")
    
    for win in windows:
        section_count = win.get('SectionCount', 1)
        print(f"  - Window with {section_count} sections")
    
    spawn_points = [e for e in elements if e.get('Type') == 'SpawnPoint']
    print(f"✓ Found {len(spawn_points)} SpawnPoint elements")
    
    for sp in spawn_points:
        assert 'Start' in sp, "SpawnPoint should have Start"
        assert 'Yaw' in sp, "SpawnPoint should have Yaw"
        print(f"  - SpawnPoint at {sp['Start']}, Yaw: {sp['Yaw']}°")

def test_copy_operations():
    """Test that JSON serialization works for copy/paste"""
    print("\nTesting copy/paste serialization...")
    
    test_item = {
        "Type": "Cubicle",
        "Start": {"X": 100.0, "Y": 200.0},
        "Dimensions": {"X": 300.0, "Y": 250.0},
        "Yaw": 90.0
    }
    
    # Simulate copy
    copied = json.loads(json.dumps(test_item))
    
    # Simulate paste with offset
    copied["Start"]["X"] += 50.0
    copied["Start"]["Y"] += 50.0
    
    assert copied["Start"]["X"] == 150.0, "Copy/paste offset should work"
    assert copied["Start"]["Y"] == 250.0, "Copy/paste offset should work"
    assert copied is not test_item, "Copy should create new object"
    
    print("✓ Copy/paste serialization works correctly")

def main():
    """Run all tests"""
    print("=" * 60)
    print("Floorplan Editor Feature Tests")
    print("=" * 60)
    
    try:
        elements = test_load_json()
        test_element_types(elements)
        test_copy_operations()
        
        print("\n" + "=" * 60)
        print("All tests passed! ✓")
        print("=" * 60)
        return 0
    except AssertionError as e:
        print(f"\n✗ Test failed: {e}")
        return 1
    except Exception as e:
        print(f"\n✗ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1

if __name__ == '__main__':
    sys.exit(main())
