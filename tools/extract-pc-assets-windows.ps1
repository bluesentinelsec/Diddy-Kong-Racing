param(
    [Parameter(Position = 0)]
    [string]$Rom
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ExpectedSha1 = "6D96743D46F8C0CD0EDB0EC5600B003C89B93755"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Baserom = Join-Path $RepoRoot "baseroms\baserom.us.v80.z64"
$Venv = Join-Path $RepoRoot ".venv"
$VenvPython = Join-Path $Venv "Scripts\python.exe"

Push-Location $RepoRoot
try {
    if ($Rom) {
        $Candidates = @((Resolve-Path -LiteralPath $Rom).Path)
    } else {
        $Candidates = @(
            Get-ChildItem -LiteralPath $RepoRoot, (Join-Path $RepoRoot "baseroms") -File |
                Where-Object { $_.Extension -in ".z64", ".v64", ".n64" } |
                Select-Object -ExpandProperty FullName -Unique
        )
    }

    $MatchingRom = $null
    foreach ($Candidate in $Candidates) {
        if ((Get-FileHash -LiteralPath $Candidate -Algorithm SHA1).Hash -eq $ExpectedSha1) {
            $MatchingRom = $Candidate
            break
        }
    }

    if ($null -eq $MatchingRom) {
        throw "US 1.1 ROM not found (expected SHA-1 $ExpectedSha1). Put it in the repository root or baseroms/."
    }

    if ($MatchingRom -ne $Baserom) {
        Copy-Item -LiteralPath $MatchingRom -Destination $Baserom -Force
    }
    Write-Host "ROM: $MatchingRom"

    if (-not (Test-Path -LiteralPath $VenvPython)) {
        $SystemPython = (Get-Command python.exe -ErrorAction Stop).Source
        & $SystemPython -m venv $Venv
        if ($LASTEXITCODE -ne 0) {
            throw "Could not create the Python virtual environment."
        }
    }

    & $VenvPython -m pip install --disable-pip-version-check --quiet -r (Join-Path $RepoRoot "requirements.txt")
    if ($LASTEXITCODE -ne 0) {
        throw "Could not install the asset extraction dependencies."
    }

    # Python otherwise uses the active Windows code page. Some decompilation
    # sources contain UTF-8 text which Splat scans while locating functions.
    $PreviousPythonUtf8 = $env:PYTHONUTF8
    # Splat regenerates this tracked file even though PC extraction does not
    # need it. Preserve its exact bytes (including the checkout's line endings)
    # so `make assets` does not dirty the source tree.
    $MacroInclude = Join-Path $RepoRoot "include\macro.inc"
    $MacroIncludeBytes = [System.IO.File]::ReadAllBytes($MacroInclude)
    try {
        $env:PYTHONUTF8 = "1"
        & $VenvPython -m splat split (Join-Path $RepoRoot "ver\splat\dkr.us.v80.yaml")
        if ($LASTEXITCODE -ne 0) {
            throw "Splat failed to extract the ROM."
        }
    } finally {
        $env:PYTHONUTF8 = $PreviousPythonUtf8
        [System.IO.File]::WriteAllBytes($MacroInclude, $MacroIncludeBytes)
    }

    $AssetsBin = Join-Path $RepoRoot "assets\assets.bin"
    $AssetsLut = Join-Path $RepoRoot "assets\assets.lut.bin"
    if (-not (Test-Path -LiteralPath $AssetsBin) -or -not (Test-Path -LiteralPath $AssetsLut)) {
        throw "Extraction completed without producing the two PC runtime asset files."
    }
    if ((Get-Item -LiteralPath $AssetsBin).Length -ne 11140640 -or
        (Get-Item -LiteralPath $AssetsLut).Length -ne 208) {
        throw "The extracted US 1.1 runtime asset files have unexpected sizes."
    }

    Write-Host "Assets ready:"
    Get-Item -LiteralPath $AssetsBin, $AssetsLut | Format-Table Name, Length
} finally {
    Pop-Location
}
