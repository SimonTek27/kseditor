#!/usr/bin/env python3
with open('E:\\Users\\Simon\\source\\repos\\kseditor/src/core/mesh/MeshOperations.h', 'r') as f:
    content = f.read()

idx1 = content.find('static NURBSSurface filletSurface(const NURBSSurface& surfaceA,')
# Find the second occurrence after the first
idx2 = content.find('static NURBSSurface filletSurface(const NURBSSurface& surfaceA,', idx1 + 1)

first = content[idx1:idx1+350]
second = content[idx2:idx2+350]

print("=== FIRST ===")
print(first)
print("=== SECOND ===")
print(second)

# Remove the default argument from the second occurrence
# The second has "int segments = 8" - we need to remove "int segments = " but keep the semicolon
# Actually, we need to replace the second declaration to not have the default

# Let's just remove the default from the second one
# Find "int segments = 8);" in the second occurrence and replace
import re
# Pattern: int segments = 8);
new_content = re.sub(
    r'int segments = 8\)', 
    'int segments', 
    content,
    count=1  # only first replacement... wait, we want to remove from second
)

# Actually, let's find both and remove default from second
# Let's just print what we need to change
print("\n\nNeed to modify second occurrence")