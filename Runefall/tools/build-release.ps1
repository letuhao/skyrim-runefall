# Builds a v1 candidate release archive (Data + fomod) from build/dist.
param(
    [string]$Project = (Join-Path $PSScriptRoot ".."),
    [string]$Version = ""
)
$ErrorActionPreference = "Stop"
$Project = (Resolve-Path $Project).Path
if (-not $Version) {
    $current = Get-Content (Join-Path $Project "CURRENT.txt") -Raw
    $Version = ($current -replace "Runefall\s+", "").Trim()
}
$dist = Join-Path $Project "build\dist"
$releaseRoot = Join-Path $Project "build\release"
$stage = Join-Path $releaseRoot "Runefall-$Version"
$zip = Join-Path $releaseRoot "Runefall-$Version.zip"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
if (Test-Path $zip) { Remove-Item $zip -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
New-Item -ItemType Directory -Force (Join-Path $stage "Data") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $stage "fomod") | Out-Null

robocopy (Join-Path $dist "Data") (Join-Path $stage "Data") /E /NFL /NDL /NP /R:1 /W:1 | Out-Null
if ($LASTEXITCODE -ge 8) { throw "data copy failed: $LASTEXITCODE" }
robocopy (Join-Path $Project "fomod") (Join-Path $stage "fomod") /E /NFL /NDL /NP /R:1 /W:1 | Out-Null
if ($LASTEXITCODE -ge 8) { throw "fomod copy failed: $LASTEXITCODE" }

$data = Join-Path $stage "Data"
$plugins = Join-Path $data "SKSE\Plugins"
New-Item -ItemType Directory -Force $plugins | Out-Null
Copy-Item (Join-Path $Project "src\plugin\build\msvc\Runefall.dll") (Join-Path $plugins "Runefall.dll") -Force
$scripts = Join-Path $data "Scripts"
if (-not (Test-Path $scripts)) { New-Item -ItemType Directory -Force $scripts | Out-Null }

Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -Force
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash
$zip | Set-Content (Join-Path $releaseRoot "LAST-ARCHIVE.txt")
Write-Output "archive=$zip"
Write-Output "sha256=$hash"
