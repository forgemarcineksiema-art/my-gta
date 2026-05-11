param(
    [string]$Preset = "ci",
    [string]$Configuration = "Debug",
    [int]$SmokeFrames = 3
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
    throw "Game executable not found. Checked:$([Environment]::NewLine)$candidateText. Run cmake --build --preset $Preset first."
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
$ExePath = Resolve-GameExecutable -BinaryDir $BinaryDir -Configuration $Configuration

Write-Host "== Blok 13 CI Smoke =="
Write-Host "Root: $SourceRoot"
Write-Host "Preset: $Preset"
Write-Host "Configuration: $Configuration"
Write-Host "Binary dir: $BinaryDir"
Write-Host "Data root: $DataRoot"
Write-Host "Executable: $ExePath"
Write-Host "Smoke frames: $SmokeFrames"

& $ExePath `
    --data-root $DataRoot `
    --no-audio `
    --no-save `
    --no-load-save `
    --smoke-frames $SmokeFrames
