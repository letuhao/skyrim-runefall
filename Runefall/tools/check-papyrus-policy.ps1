<#
.SYNOPSIS
  Enforces the Runefall Papyrus policy: Papyrus is allowed only for native declarations, the
  debug/probe harness, and explicitly reviewed compatibility shims. Gameplay logic and polling
  loops are DLL-only.
#>
[CmdletBinding()]
param([string]$Project = (Join-Path $PSScriptRoot ".."))
$ErrorActionPreference = 'Stop'
$Project = (Resolve-Path $Project).Path
$papyrus = Join-Path $Project 'src\papyrus'
$allowed = @(
    'RunefallNative.psc',
    'RunefallProbe.psc',
    'RunefallQuestHost.psc',
    'RunefallMenuBridge.psc'
)
$forbidden = @(
    'RegisterForUpdate',
    'RegisterForSingleUpdate',
    'OnUpdate',
    'Utility.Wait',
    'Utility.WaitGameTime',
    'While '
)
if (-not (Test-Path $papyrus)) { throw "Papyrus source directory missing: $papyrus" }
$scripts = Get-ChildItem $papyrus -Filter '*.psc'
$failures = New-Object System.Collections.Generic.List[string]
foreach ($script in $scripts) {
    if ($script.Name -notin $allowed) {
        $failures.Add("unlisted Papyrus script: $($script.Name) (DLL-first policy)")
        continue
    }
    $text = Get-Content $script.FullName -Raw
    if ($script.Name -eq 'RunefallNative.psc') {
        if ($text -match 'EndFunction') { $failures.Add('RunefallNative.psc must contain native declarations only') }
        $decls = [regex]::Matches($text, '(?m)^\s*(?:[A-Za-z0-9_]+)\s+Function\s+[A-Za-z0-9_]+\s*\([^)]*\)\s+Global\s+Native\s*$')
        $fnCount = [regex]::Matches($text, '(?m)^\s*(?:[A-Za-z0-9_]+)\s+Function\s+').Count
        if ($decls.Count -ne $fnCount) { $failures.Add("RunefallNative.psc has non-native function declarations ($($fnCount - $decls.Count))") }
        continue
    }
    foreach ($pattern in $forbidden) {
        if ($text -match [regex]::Escape($pattern)) {
            $failures.Add("$($script.Name) contains forbidden Papyrus pattern '$pattern'")
        }
    }
}
if ($scripts.Count -eq 0) { $failures.Add('no Papyrus scripts found; expected the native declaration stub') }
if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Host "[FAIL] $_" }
    exit 1
}
Write-Host ("[PASS] Papyrus policy: {0} allowlisted script(s), no polling/gameplay logic" -f $scripts.Count)
exit 0
