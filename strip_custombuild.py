#!/usr/bin/env python3
import sys
import os
import re

base_dir = sys.argv[1] if len(sys.argv) > 1 else r'E:\Users\Simon\source\repos\kseditor'

# Find all .vcxproj files
vcxproj_files = []
for root, dirs, files in os.walk(base_dir):
    for f in files:
        if f.endswith('.vcxproj'):
            vcxproj_files.append(os.path.join(root, f))

modified = 0
for vcxproj_path in vcxproj_files:
    try:
        with open(vcxproj_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original = content
        # Remove all CustomBuild blocks
        content = re.sub(r'<CustomBuild Include=".*?">.*?</CustomBuild>', '', content, flags=re.DOTALL)
        
        if content != original:
            with open(vcxproj_path, 'w', encoding='utf-8') as f:
                f.write(content)
            modified += 1
            print(f"Modified: {os.path.relpath(vcxproj_path, base_dir)}")
    except Exception as e:
        print(f"Error: {vcxproj_path}: {e}")

print(f"\nTotal modified: {modified}")