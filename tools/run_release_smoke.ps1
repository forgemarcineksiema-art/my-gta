param(
    [string]$Preset = "release",
    [string]$Configuration = "Release",
    [int]$SmokeFrames = 3,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$GameArgs
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
    throw "Game executable not found. Checked:$([Environment]::NewLine)$candidateText"
}

$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$DataRoot = (Resolve-Path (Join-Path $SourceRoot "data")).Path
$PresetPath = Join-Path $SourceRoot "CMakePresets.json"
$PresetFile = Get-Content $PresetPath -Raw | ConvertFrom-Json
$ConfigurePreset = $PresetFile.configurePresets | Where-Object { $_.name -eq $Preset } | Select-Object -First 1
if ($null -eq $ConfigurePreset) {
    throw "Unknown configure preset: $Preset"
}
if ($ConfigurePreset.cacheVariables.BS3D_BUILD_GAME -eq "OFF") {
    throw "Preset '$Preset' does not build blokowa_satyra"
}

$BinaryDir = $ConfigurePreset.binaryDir.Replace('${sourceDir}', $SourceRoot)
$BinaryDir = [System.IO.Path]::GetFullPath($BinaryDir)

Write-Host "== Blok 13 Release Smoke =="
Write-Host "Root: $SourceRoot"
Write-Host "Preset: $Preset"
Write-Host "Configuration: $Configuration"
Write-Host "Binary dir: $BinaryDir"
Write-Host "Data root: $DataRoot"
Write-Host "Smoke frames: $SmokeFrames"

cmake --preset $Preset
cmake --build --preset $Preset

$ExePath = Resolve-GameExecutable -BinaryDir $BinaryDir -Configuration $Configuration
Write-Host "Executable: $ExePath"
Write-Host "Args: --data-root `"$DataRoot`" --no-audio --no-save --no-load-save --smoke-frames $SmokeFrames $GameArgs"

& $ExePath --data-root $DataRoot --no-audio --no-save --no-load-save --smoke-frames $SmokeFrames @GameArgs
