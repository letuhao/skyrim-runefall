# Builds the Runefall Papyrus scripts into build/dist/Data/Scripts.
# Requires the Steam Skyrim SE source scripts + Papyrus Compiler (paths configurable).
param(
    [string]$Compiler = "E:\SteamLibrary\steamapps\common\Skyrim Special Edition\Papyrus Compiler\PapyrusCompiler.exe",
    [string]$SourceDir = "E:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data\Source\Scripts",
    [string]$Project = (Join-Path $PSScriptRoot "..\..")
)
$ErrorActionPreference = "Stop"
$src = Join-Path $Project "src\papyrus"
$out = Join-Path $Project "build\dist\Data\Scripts"
New-Item -ItemType Directory -Force $out | Out-Null
$flags = Join-Path $SourceDir "TESV_Papyrus_Flags.flg"
foreach ($script in Get-ChildItem $src -Filter "*.psc") {
    & $Compiler $script.FullName "-f=$flags" "-i=$src;$SourceDir" "-o=$out"
    if ($LASTEXITCODE -ne 0) { throw "Papyrus compile failed: $($script.Name)" }
}
Write-Output "Papyrus scripts compiled to $out"
