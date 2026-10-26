# Installs the compiled Runefall test build into a standalone test environment.
# Copies SKSE Menu Framework (release) first, then generated content, Papyrus PEX and the DLL.
param(
    [string]$TestEnv = "J:\SkyrimTest\1.6.1170",
    [string]$FrameworkPackage = "D:\Works\source\skyrim-moddings\rpg-mechanism\skyrim-menu-framework\SKSE Menu Framework 120352 3.18 2026-09-17T16-36Z cE8hkAlOT",
    [string]$Project = (Join-Path $PSScriptRoot ".."),
    [string]$ImGuiDll = "D:\Works\source\skyrim-moddings\rpg-mechanism\skyrim-menu-framework\SKSE-Menu-Framework-SDK-main\ImGui\release\bin\imgui.dll"
)
$ErrorActionPreference = "Stop"
if (-not (Test-Path $TestEnv)) { throw "Test environment not found: $TestEnv" }
if (-not (Test-Path $FrameworkPackage)) { throw "Menu Framework package not found: $FrameworkPackage" }

$data = Join-Path $TestEnv "Data"
New-Item -ItemType Directory -Force (Join-Path $data "SKSE\Plugins") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $data "Scripts") | Out-Null

Write-Output "Installing SKSE Menu Framework..."
robocopy (Join-Path $FrameworkPackage "SKSE") (Join-Path $data "SKSE") /E /XF "SKSEMenuFramework.pdb" /NFL /NDL /NP /R:1 /W:1 | Out-Null
if ($LASTEXITCODE -ge 8) { throw "Framework copy failed: $LASTEXITCODE" }

Write-Output "Installing generated content..."
$dist = Join-Path $Project "build\dist\Data"
if (Test-Path $dist) {
    robocopy $dist $data /E /NFL /NDL /NP /R:1 /W:1 | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "Content copy failed: $LASTEXITCODE" }
}

Write-Output "Installing native DLL..."
$dll = Join-Path $Project "src\plugin\build\msvc\Runefall.dll"
if (-not (Test-Path $dll)) { throw "Runefall.dll not built: $dll" }
Copy-Item $dll (Join-Path $data "SKSE\Plugins\Runefall.dll") -Force

Write-Output "Installed to $TestEnv"
Get-ChildItem (Join-Path $data "SKSE\Plugins") -Filter "SKSEMenuFramework.dll" | Select-Object Name,Length
Get-ChildItem (Join-Path $data "SKSE\Plugins") -Filter "Runefall.dll" | Select-Object Name,Length
Get-ChildItem (Join-Path $data "Scripts") -Filter "Runefall*.pex" | Select-Object Name,Length