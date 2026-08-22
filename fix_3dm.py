#!/usr/bin/env python3
import sys

filepath = r'E:\Users\Simon\source\repos\kseditor\src\modules\modellingEditor\3DModeling_io.h'
with open(filepath, 'r') as f:
    lines = f.readlines()

# Insert the 3dm import/export after line 79 (importKS3D, 0-indexed is 78)
new_lines = []
for i, line in enumerate(lines):
    new_lines.append(line)
    if i == 78:  # After line 79 (importKS3D, 0-indexed is 78)
        new_lines.append('    bool import3DM(const QString& path, geometry::Scene3D* scene);\n')
        new_lines.append('    bool export3DM(geometry::Scene3D* scene, const QString& path);\n\n')

with open(filepath, 'w') as f:
    f.writelines(new_lines)
print('Done modifying')