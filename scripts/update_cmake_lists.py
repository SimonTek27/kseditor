#!/usr/bin/env python3
"""
CMakeLists.txt Module Reorganization Updater
Automatically updates SOURCES and HEADERS paths after module reorganization
"""

import re
import sys
from pathlib import Path

def update_cmake_lists(cmake_file: Path, dry_run: bool = True) -> int:
	"""
	Update CMakeLists.txt with new module paths

	Args:
		cmake_file: Path to CMakeLists.txt
		dry_run: If True, only show what would be changed

	Returns:
		Number of replacements made
	"""

	# Define path replacements
	replacements = [
		# 3DModeling → 3DEngine/Modeling
		(r'src/modules/3DModeling/', 'src/modules/3DEngine/Modeling/'),

		# UI consolidation
		(r'src/modules/displayeditor/', 'src/modules/UI/displayeditor/'),
		(r'src/modules/fonteditor/', 'src/modules/UI/fonteditor/'),
		(r'src/modules/SetupEditor/', 'src/modules/UI/SetupEditor/'),
		(r'src/modules/ppfilterseditor/', 'src/modules/UI/ppfilterseditor/'),

		# AssettoCorsaComponents
		(r'src/modules/CspConfigEditor/', 'src/modules/AssettoCorsaComponents/CSPConfig/'),
	]

	if not cmake_file.exists():
		print(f"❌ File not found: {cmake_file}")
		return 0

	# Read original content
	original_content = cmake_file.read_text()
	updated_content = original_content
	replacements_count = 0

	# Apply replacements
	for old_path, new_path in replacements:
		# Count occurrences
		pattern = re.compile(re.escape(old_path))
		matches = pattern.findall(updated_content)
		count = len(matches)

		if count > 0:
			print(f"📝 {old_path:45} → {new_path:45} ({count:3} occurrences)")
			if not dry_run:
				updated_content = updated_content.replace(old_path, new_path)
			replacements_count += count

	# Write updated content if not dry run
	if not dry_run and replacements_count > 0:
		cmake_file.write_text(updated_content)
		print(f"\n✅ Updated {cmake_file} with {replacements_count} replacements")
		return replacements_count
	elif dry_run and replacements_count > 0:
		print(f"\n📊 DRY RUN: Would make {replacements_count} replacements")
		return replacements_count
	else:
		print(f"\n⚠️  No replacements needed")
		return 0

def main():
	"""Main entry point"""

	cmake_path = Path("CMakeLists.txt")

	# Try to find CMakeLists.txt
	if not cmake_path.exists():
		# Try parent directory
		if (Path("..") / "CMakeLists.txt").exists():
			cmake_path = Path("..") / "CMakeLists.txt"
		else:
			print("❌ CMakeLists.txt not found in current directory or parent")
			print(f"   Current dir: {Path.cwd()}")
			sys.exit(1)

	print("=" * 100)
	print("CMakeLists.txt Module Reorganization Updater")
	print("=" * 100)
	print(f"\n📍 Processing: {cmake_path.absolute()}")
	print(f"📊 File size: {cmake_path.stat().st_size} bytes")
	print("\n🔍 Checking for paths to update:\n")

	# First, dry run to show what would be changed
	replacements = update_cmake_lists(cmake_path, dry_run=True)

	if replacements > 0:
		print("\n" + "=" * 100)
		print("Apply changes? (yes/no)")
		print("=" * 100)
		response = input("> ").strip().lower()

		if response in ("yes", "y", "1"):
			print("\n⏳ Applying changes...\n")
			update_cmake_lists(cmake_path, dry_run=False)
			print("\n✅ CMakeLists.txt successfully updated!")
		else:
			print("\n⏭️  Skipped. No changes made.")
			sys.exit(0)
	else:
		print("\n✅ No changes needed")
		sys.exit(0)

if __name__ == "__main__":
	main()
