param(
    [string]$Preset = "ci",
    [string]$Configuration = "Debug",
    [int]$SmokeFrames = 3,
    [string]$Output = "artifacts\shadow_frame.txt",
    [switch]$Build
)

$ErrorActionPreference = "Stop"

function Resolve-Executable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BinaryDir,
        [Parameter(Mandatory = $true)]
        [string]$Configuration,
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $candidates = @()
    if (![string]::IsNullOrWhiteSpace($Configuration)) {
        $candidates += Join-Path (Join-Path $BinaryDir $Configuration) $Name
    }
    $candidates += Join-Path $BinaryDir $Name

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $candidateText = ($candidates | ForEach-Object { "  $_" }) -join [Environment]::NewLine
    throw "$Name not found. Checked:$([Environment]::NewLine)$candidateText. Run with -Build or: cmake --build --preset $Preset"
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
        throw "Build of blokowa_satyra failed with exit code $LASTEXITCODE"
    }
    Write-Host "Building bs3d_d3d11_renderer_smoke (preset $Preset)..."
    cmake --build --preset $Preset --target bs3d_d3d11_renderer_smoke
    if ($LASTEXITCODE -ne 0) {
        throw "Build of bs3d_d3d11_renderer_smoke failed with exit code $LASTEXITCODE"
    }
    Write-Host "Build completed"
}

$GameExe = Resolve-Executable -BinaryDir $BinaryDir -Configuration $Configuration -Name "blokowa_satyra.exe"
$SmokeExe = Resolve-Executable -BinaryDir $BinaryDir -Configuration $Configuration -Name "bs3d_d3d11_renderer_smoke.exe"

$OutputFull = Join-Path $SourceRoot $Output

Write-Host "== RenderFrame Capture/Replay Smoke =="
Write-Host "Root: $SourceRoot"
Write-Host "Preset: $Preset"
Write-Host "Configuration: $Configuration"
Write-Host "Binary dir: $BinaryDir"
Write-Host "Data root: $DataRoot"
Write-Host "Game exe: $GameExe"
Write-Host "Smoke exe: $SmokeExe"
Write-Host "Smoke frames: $SmokeFrames"
Write-Host "Output: $OutputFull"

# --- Capture ---
Write-Host ""
Write-Host "--- CAPTURE: run GameApp shadow RenderFrame dump ---"
& $GameExe `
    --data-root $DataRoot `
    --no-audio `
    --no-save `
    --no-load-save `
    --smoke-frames $SmokeFrames `
    --renderframe-shadow `
    --renderframe-shadow-interval 1 `
    --renderframe-shadow-dump $OutputFull

if ($LASTEXITCODE -ne 0) {
    Write-Host "CAPTURE FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $OutputFull)) {
    Write-Host "CAPTURE FAILED: output file not found: $OutputFull" -ForegroundColor Red
    exit 1
}

$fileSize = (Get-Item $OutputFull).Length
if ($fileSize -eq 0) {
    Write-Host "CAPTURE FAILED: output file is empty: $OutputFull" -ForegroundColor Red
    exit 1
}

$headerLine = (Get-Content $OutputFull -First 1).Trim()
if ($headerLine -ne "RenderFrameDump v1") {
    Write-Host "CAPTURE FAILED: unexpected dump header '$headerLine', expected 'RenderFrameDump v1'" -ForegroundColor Red
    exit 1
}

Write-Host "CAPTURE OK ($fileSize bytes, v1 header)" -ForegroundColor Green

# --- Replay (direct) ---
Write-Host ""
Write-Host "--- REPLAY: D3D11 smoke (direct) ---"
& $SmokeExe --frames 3 --load-frame $OutputFull
if ($LASTEXITCODE -eq 0) {
    Write-Host "REPLAY (direct) OK" -ForegroundColor Green
} else {
    Write-Host "REPLAY (direct) FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

# --- Replay (factory) ---
Write-Host ""
Write-Host "--- REPLAY: D3D11 smoke (factory) ---"
& $SmokeExe --frames 3 --factory --load-frame $OutputFull
if ($LASTEXITCODE -eq 0) {
    Write-Host "REPLAY (factory) OK" -ForegroundColor Green
} else {
    Write-Host "REPLAY (factory) FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

# --- Replay (game shell) ---
Write-Host ""
Write-Host "--- REPLAY: D3D11 game shell ---"
$ShellExe = Resolve-Executable -BinaryDir $BinaryDir -Configuration $Configuration -Name "bs3d_d3d11_game_shell.exe"
& $ShellExe --frames 3 --load-frame $OutputFull
if ($LASTEXITCODE -eq 0) {
    Write-Host "REPLAY (game shell) OK" -ForegroundColor Green
} else {
    Write-Host "REPLAY (game shell) FAILED (exit code $LASTEXITCODE)" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "All passes OK" -ForegroundColor Green
