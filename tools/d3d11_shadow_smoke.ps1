param(
    [string]$Preset = "ci",
    [string]$Configuration = "Debug",
    [int]$SmokeFrames = 3,
    [switch]$Build
)

$ErrorActionPreference = "Stop"

function Resolve-GameExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BinaryDir,
        [Parameter(Mandatory = $true)]
        [string]$Configuration
    )

    $candidates = @()
    if (![string]::IsNullOrWhiteSpace($Configuration)) {
        $candidates += Join-Path (Join-Path $BinaryDir $Configuration) "blokowa_satyra.exe"
    }
    $candidates += Join-Path $BinaryDir "blokowa_satyra.exe"

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $candidateText = ($candidates | ForEach-Object { "  $_" }) -join [Environment]::NewLine
    throw "Game executable not found. Checked:$([Environment]::NewLine)$candidateText. Run with -Build or: cmake --build --preset $Preset --target blokowa_satyra"
}

$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$DataRoot = (Resolve-Path (Join-Path $SourceRoot "data")).Path
$PresetFile = Get-Content (Join-Path $SourceRoot "CMakePresets.json") -Raw | ConvertFrom-Json
$ConfigurePreset = $PresetFile.configurePresets | Where-Object { $_.name -eq $Preset } | Select-Object -First 1
if ($null -eq $ConfigurePreset) {
    throw "Unknown configure preset: $Preset"
}
if ($ConfigurePreset.cacheVariables.BS3D_BUILD_GAME -eq "OFF") {
    throw "Preset '$Preset' does not build blokowa_satyra"
}

$BinaryDir = $ConfigurePreset.binaryDir.Replace('${sourceDir}', $SourceRoot)
$BinaryDir = [System.IO.Path]::GetFullPath($BinaryDir)

if ($Build) {
    Write-Host "Building blokowa_satyra (preset $Preset)..."
    cmake --build --preset $Preset --target blokowa_satyra
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }
    Write-Host "Build completed"
}

$ExePath = Resolve-GameExecutable -BinaryDir $BinaryDir -Configuration $Configuration

Write-Host "== Blok 13 D3D11 Shadow Sidecar Smoke =="
Write-Host "Root: $SourceRoot"
Write-Host "Preset: $Preset"
Write-Host "Configuration: $Configuration"
Write-Host "Binary dir: $BinaryDir"
Write-Host "Data root: $DataRoot"
Write-Host "Executable: $ExePath"
Write-Host "Smoke frames: $SmokeFrames"

$baseArgs = @(
    "--data-root", $DataRoot,
    "--no-audio",
    "--no-save",
    "--no-load-save",
    "--smoke-frames", $SmokeFrames
)

# Test 1: --renderframe-shadow with diagnostics (no sidecar window)
Write-Host ""
Write-Host "--- PASS 1: shadow diagnostics only (no sidecar window) ---"
& $ExePath @baseArgs --renderframe-shadow --renderframe-shadow-interval 1 --d3d11-shadow-diagnostics
if ($LASTEXITCODE -eq 0) {
    Write-Host "PASS 1 OK" -ForegroundColor Green
} else {
    Write-Host "PASS 1 FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

# Test 2: full sidecar window with diagnostics (Windows only)
Write-Host ""
Write-Host "--- PASS 2: full sidecar window with diagnostics ---"
if ($IsWindows -or $env:OS -match "Windows") {
    & $ExePath @baseArgs --renderframe-shadow --renderframe-shadow-interval 1 --d3d11-shadow-window --d3d11-shadow-diagnostics
    if ($LASTEXITCODE -eq 0) {
        Write-Host "PASS 2 OK" -ForegroundColor Green
    } else {
        Write-Host "PASS 2 FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "PASS 2 SKIPPED (non-Windows)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "All passes OK" -ForegroundColor Green
