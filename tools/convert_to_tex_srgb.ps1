# Convert textures to BC1_UNORM_SRGB .tex format
# This script finds all image files in examples/*/Assets/Textures and converts them to compressed .tex format

param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# Get the script's directory (tools folder)
$toolsDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$texconvPath = Join-Path $toolsDir "texconv.exe"
$rootDir = Split-Path -Parent $toolsDir

# Verify texconv exists
if (-not (Test-Path $texconvPath)) {
    Write-Host "Error: texconv.exe not found at $texconvPath" -ForegroundColor Red
    exit 1
}

# Supported image extensions
$imageExtensions = @("*.png", "*.jpg", "*.jpeg", "*.tga")

# Find all texture directories in examples
$examplesDir = Join-Path $rootDir "examples"
if (-not (Test-Path $examplesDir)) {
    Write-Host "Error: Examples directory not found at $examplesDir" -ForegroundColor Red
    exit 1
}

Write-Host "Scanning for example projects..." -ForegroundColor Cyan

# Build list of projects with texture directories
$projects = @()
Get-ChildItem -Path $examplesDir -Directory | ForEach-Object {
    $textureDir = Join-Path $_.FullName "Assets\Textures"
    if (Test-Path $textureDir) {
        # Count images in this project
        $imageCount = 0
        foreach ($ext in $imageExtensions) {
            $imageCount += (Get-ChildItem -Path $textureDir -Filter $ext -File).Count
        }

        $projects += @{
            Name = $_.Name
            Path = $_.FullName
            TextureDir = $textureDir
            ImageCount = $imageCount
        }
    }
}

if ($projects.Count -eq 0) {
    Write-Host "No example projects with Assets/Textures directories found." -ForegroundColor Yellow
    exit 0
}

# Display project selection menu
Write-Host "`n=====================================" -ForegroundColor Cyan
Write-Host "Available Example Projects" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan

for ($i = 0; $i -lt $projects.Count; $i++) {
    $project = $projects[$i]
    Write-Host "$($i + 1). $($project.Name) " -NoNewline -ForegroundColor White
    Write-Host "($($project.ImageCount) image(s))" -ForegroundColor Gray
}
Write-Host "$($projects.Count + 1). All projects" -ForegroundColor Yellow

# Get user selection
Write-Host ""
do {
    $selection = Read-Host "Select a project (1-$($projects.Count + 1))"
    $selectionNum = 0
    $validSelection = [int]::TryParse($selection, [ref]$selectionNum) -and $selectionNum -ge 1 -and $selectionNum -le ($projects.Count + 1)

    if (-not $validSelection) {
        Write-Host "Invalid selection. Please enter a number between 1 and $($projects.Count + 1)." -ForegroundColor Red
    }
} while (-not $validSelection)

# Determine which projects to process
$selectedProjects = @()
if ($selectionNum -eq ($projects.Count + 1)) {
    # All projects
    $selectedProjects = $projects
    Write-Host "`nSelected: All projects" -ForegroundColor Green
} else {
    # Single project
    $selectedProjects = @($projects[$selectionNum - 1])
    Write-Host "`nSelected: $($selectedProjects[0].Name)" -ForegroundColor Green
}

# Collect all image files from selected projects
$allImages = @()
foreach ($project in $selectedProjects) {
    $textureDir = $project.TextureDir
    foreach ($ext in $imageExtensions) {
        $files = Get-ChildItem -Path $textureDir -Filter $ext -File
        foreach ($file in $files) {
            $allImages += @{
                Path = $file.FullName
                Directory = $textureDir
                Name = $file.Name
                Project = $project.Name
            }
        }
    }
}

if ($allImages.Count -eq 0) {
    Write-Host "No image files found to convert in selected project(s)." -ForegroundColor Yellow
    exit 0
}

# Display found files
Write-Host "`nFound $($allImages.Count) image file(s) to convert:" -ForegroundColor Green
foreach ($img in $allImages) {
    $relativePath = $img.Path.Replace($rootDir + "\", "")
    if ($selectedProjects.Count -gt 1) {
        Write-Host "  [$($img.Project)] " -NoNewline -ForegroundColor Cyan
    }
    Write-Host "$relativePath" -ForegroundColor Gray
}

# Prompt for confirmation unless -Force is used
if (-not $Force) {
    Write-Host "`nConversion settings:" -ForegroundColor Cyan
    Write-Host "  Format: BC1_UNORM_SRGB (compressed, sRGB color space)"
    Write-Host "  Output: .tex files (DDS renamed)"
    Write-Host "  Mipmaps: Auto-generated"
    Write-Host ""
    $response = Read-Host "Do you want to proceed with the conversion? (y/n)"

    if ($response -ne "y" -and $response -ne "Y") {
        Write-Host "Conversion cancelled." -ForegroundColor Yellow
        exit 0
    }
}

# Process each image
$successCount = 0
$failCount = 0

Write-Host "`nConverting textures..." -ForegroundColor Cyan

foreach ($img in $allImages) {
    $relativePath = $img.Path.Replace($rootDir + "\", "")
    Write-Host "`nProcessing: $relativePath" -ForegroundColor White

    try {
        # Run texconv
        $outputDir = $img.Directory
        $args = @(
            "-f", "BC1_UNORM_SRGB",
            "-o", $outputDir,
            $img.Path
        )

        $process = Start-Process -FilePath $texconvPath -ArgumentList $args -NoNewWindow -Wait -PassThru

        if ($process.ExitCode -ne 0) {
            throw "texconv.exe failed with exit code $($process.ExitCode)"
        }

        # Rename .dds to .tex
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($img.Name)
        $ddsPath = Join-Path $outputDir "$baseName.dds"
        $texPath = Join-Path $outputDir "$baseName.tex"

        if (Test-Path $ddsPath) {
            # Remove existing .tex if it exists
            if (Test-Path $texPath) {
                Remove-Item $texPath -Force
            }

            Rename-Item -Path $ddsPath -NewName "$baseName.tex" -Force
            Write-Host "  Success: Created $baseName.tex" -ForegroundColor Green
            $successCount++
        } else {
            throw "Expected output file not found: $ddsPath"
        }
    }
    catch {
        Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Red
        $failCount++
    }
}

# Summary
Write-Host "`n=====================================" -ForegroundColor Cyan
Write-Host "Conversion Summary" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "Successful: $successCount" -ForegroundColor Green
Write-Host "Failed: $failCount" -ForegroundColor $(if ($failCount -gt 0) { "Red" } else { "Gray" })
Write-Host "Total: $($allImages.Count)" -ForegroundColor White

if ($successCount -gt 0) {
    Write-Host "`nConverted textures are ready to use as .tex files!" -ForegroundColor Green
}
