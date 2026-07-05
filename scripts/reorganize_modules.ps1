#!/usr/bin/env powershell
# MODULE REORGANIZATION - FILE MIGRATION SCRIPT
# Author: ksEditor Refactoring
# Date: 2026-05-25
# Purpose: Move files to reorganized module structure

$ErrorActionPreference = "Stop"

Write-Host "🔄 ksEditor Module Reorganization Script" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

$baseDir = "E:\Users\Simon\source\repos\kseditor"

# Function to move directory content
function Move-DirectoryContent {
	param (
		[string]$Source,
		[string]$Destination,
		[string]$Label
	)

	if (!(Test-Path $Source)) {
		Write-Host "⚠️  Source not found: $Source" -ForegroundColor Yellow
		return $false
	}

	if (!(Test-Path $Destination)) {
		Write-Host "⚠️  Destination not found: $Destination" -ForegroundColor Yellow
		return $false
	}

	try {
		$fileCount = @(Get-ChildItem -Path $Source -File -Recurse).Count
		Write-Host "📦 $Label: Moving $fileCount files..." -ForegroundColor White

		Move-Item -Path "$Source\*" -Destination $Destination -Force -Confirm:$false

		Write-Host "✅ $Label: Complete" -ForegroundColor Green
		return $true
	} catch {
		Write-Host "❌ $Label: Failed - $_" -ForegroundColor Red
		return $false
	}
}

Write-Host "PRE-MIGRATION CHECKS" -ForegroundColor Cyan
Write-Host "--------------------" -ForegroundColor Cyan

# Check if VS is running
$vsRunning = Get-Process -Name "devenv" -ErrorAction SilentlyContinue
if ($vsRunning) {
	Write-Host "⚠️  Visual Studio is running. Close it first!" -ForegroundColor Yellow
	Write-Host "   Command: Stop-Process -Name 'devenv' -Force" -ForegroundColor Gray
	exit 1
}
Write-Host "✅ Visual Studio is not running" -ForegroundColor Green

# Check directories
$dirs = @(
	@{ Path = "$baseDir\src\modules\UI\displayeditor"; Type = "dest" },
	@{ Path = "$baseDir\src\modules\UI\fonteditor"; Type = "dest" },
	@{ Path = "$baseDir\src\modules\UI\SetupEditor"; Type = "dest" },
	@{ Path = "$baseDir\src\modules\UI\ppfilterseditor"; Type = "dest" },
	@{ Path = "$baseDir\src\modules\3DEngine\Modeling"; Type = "dest" },
	@{ Path = "$baseDir\src\modules\AssettoCorsaComponents\CSPConfig"; Type = "dest" },
	@{ Path = "$baseDir\src\modules\AssettoCorsaComponents\Components"; Type = "dest" },
	@{ Path = "$baseDir\src\modules\displayeditor"; Type = "source" },
	@{ Path = "$baseDir\src\modules\fonteditor"; Type = "source" },
	@{ Path = "$baseDir\src\modules\SetupEditor"; Type = "source" },
	@{ Path = "$baseDir\src\modules\ppfilterseditor"; Type = "source" },
	@{ Path = "$baseDir\src\modules\3DModeling"; Type = "source" }
)

foreach ($dir in $dirs) {
	if (Test-Path $dir.Path) {
		$fileCount = @(Get-ChildItem -Path $dir.Path -File -Recurse -ErrorAction SilentlyContinue).Count
		$status = if ($dir.Type -eq "dest") { "✅ Ready" } else { "📁 Content" }
		Write-Host "$status : $($dir.Path) ($fileCount files)" -ForegroundColor Green
	} else {
		Write-Host "❌ Missing: $($dir.Path)" -ForegroundColor Red
	}
}

Write-Host ""
Write-Host "STARTING MIGRATION" -ForegroundColor Cyan
Write-Host "==================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Move UI modules
$success = $true
$success = $success -and (Move-DirectoryContent `
	"$baseDir\src\modules\displayeditor" `
	"$baseDir\src\modules\UI\displayeditor" `
	"displayeditor")

$success = $success -and (Move-DirectoryContent `
	"$baseDir\src\modules\fonteditor" `
	"$baseDir\src\modules\UI\fonteditor" `
	"fonteditor")

$success = $success -and (Move-DirectoryContent `
	"$baseDir\src\modules\SetupEditor" `
	"$baseDir\src\modules\UI\SetupEditor" `
	"SetupEditor")

$success = $success -and (Move-DirectoryContent `
	"$baseDir\src\modules\ppfilterseditor" `
	"$baseDir\src\modules\UI\ppfilterseditor" `
	"ppfilterseditor")

# Step 2: Move 3DModeling
$success = $success -and (Move-DirectoryContent `
	"$baseDir\src\modules\3DModeling" `
	"$baseDir\src\modules\3DEngine\Modeling" `
	"3DModeling → 3DEngine/Modeling")

# Step 3: Move CspConfigEditor (if exists)
if (Test-Path "$baseDir\src\modules\CspConfigEditor") {
	$success = $success -and (Move-DirectoryContent `
		"$baseDir\src\modules\CspConfigEditor" `
		"$baseDir\src\modules\AssettoCorsaComponents\CSPConfig" `
		"CspConfigEditor")
}

Write-Host ""
Write-Host "POST-MIGRATION CLEANUP" -ForegroundColor Cyan
Write-Host "=======================" -ForegroundColor Cyan
Write-Host ""

# Remove empty directories
$emptyDirs = @(
	"$baseDir\src\modules\displayeditor",
	"$baseDir\src\modules\fonteditor",
	"$baseDir\src\modules\SetupEditor",
	"$baseDir\src\modules\ppfilterseditor",
	"$baseDir\src\modules\3DModeling",
	"$baseDir\src\modules\CspConfigEditor"
)

foreach ($dir in $emptyDirs) {
	if ((Test-Path $dir) -and @(Get-ChildItem -Path $dir).Count -eq 0) {
		try {
			Remove-Item -Path $dir -Force
			Write-Host "✅ Removed empty directory: $dir" -ForegroundColor Green
		} catch {
			Write-Host "⚠️  Could not remove: $dir" -ForegroundColor Yellow
		}
	}
}

Write-Host ""
if ($success) {
	Write-Host "✅ MIGRATION COMPLETE!" -ForegroundColor Green
	Write-Host ""
	Write-Host "Next steps:" -ForegroundColor Cyan
	Write-Host "1. Verify file contents are correct"
	Write-Host "2. Update CMakeLists.txt SOURCES and HEADERS lists"
	Write-Host "3. Run: cmake .. && cmake --build . --config Release"
	Write-Host "4. Test compilation and linking"
} else {
	Write-Host "❌ MIGRATION ENCOUNTERED ERRORS" -ForegroundColor Red
	Write-Host "Check the messages above for details" -ForegroundColor Yellow
}

Write-Host ""
