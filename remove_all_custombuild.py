import os
import re

base_dir = r'E:\Users\Simon\source\repos\kseditor'

# Find all .vcxproj files
vcxproj_files = []
for root, dirs, files in os.walk(base_dir):
    for f in files:
        if f.endswith('.vcxproj'):
            vcxproj_files.append(os.path.join(root, f))

print(f"Found {len(vcxproj_files)} vcxproj files")

for vcxproj_path in vcxproj_files:
    print(f"Processing: {vcxproj_path}")
    with open(vcxproj_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Remove all CustomBuild blocks
    original = content
    content = re.sub(r'<CustomBuild Include=".*?">.*?</CustomBuild>', '', content, flags=re.DOTALL)
    
    if content != original:
        with open(vcxproj_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"  - Removed CustomBuild entries")
    else:
        print(f"  - No CustomBuild entries found")