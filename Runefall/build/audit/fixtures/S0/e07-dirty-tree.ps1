# S0 E07 dirty-tree fixture: append to a tracked file, run gate, restore.
param([string]$Repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")).Path)
$ErrorActionPreference = "Stop"
$target = Join-Path $Repo "Runefall\STATE.md"
$original = Get-Content $target -Raw
try {
  Add-Content $target "<!-- dirty-tree-fixture -->"
  $gate = Join-Path $Repo "Runefall\tools\audit-gate.ps1"
  $out = & powershell -NoProfile -ExecutionPolicy Bypass -File $gate -StatePath "Runefall/EXECUTION-STATE.json" -Mode truthfulness -Bootstrap 2>&1
  $code = $LASTEXITCODE
  if ($code -ne 1 -or ($out -join " ") -notmatch "GATE-E07") { throw "expected GATE-E07, exit=$code" }
  Write-Output "PASS dirty-tree fixture: GATE-E07"
} finally {
  Set-Content -Path $target -Value $original -NoNewline
}
