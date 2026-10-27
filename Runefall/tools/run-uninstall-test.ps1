<#
.SYNOPSIS
  Uninstall / pack-disable matrix for the disposable J: test environment.
.DESCRIPTION
  Loads CleanSave with a reduced Runefall install, waits through gameplay activation and records
  crash evidence, then restores the environment. Modes:
    pack-disable  - base ESP + DLL remain, optional Unique/Hunt packs disabled.
    dll-remove    - ESP/data remain, Runefall.dll renamed away.
    all-remove    - every Runefall plugin disabled and the DLL renamed away.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][ValidateSet('pack-disable','dll-remove','all-remove')][string]$Mode,
    [string]$GameRoot = 'J:\SkyrimTest\1.6.1170',
    [string]$SaveName = '',
    [string]$Node = 'C:\nvm4w\nodejs\node.exe',
    [string]$McpClient = 'D:\Works\source\skyrim-moddings\.agents\skills\skyrim-mcp-control\scripts\mcp-client.js'
)
$ErrorActionPreference = 'Stop'
$pluginsTxt = Join-Path $env:LOCALAPPDATA 'Skyrim Special Edition\Plugins.txt'
$dll = Join-Path $GameRoot 'Data\SKSE\Plugins\Runefall.dll'
$dllOff = "$dll.disabled"
$skseDir = Join-Path $env:USERPROFILE 'Documents\My Games\Skyrim Special Edition\SKSE'
$evidenceDir = 'D:\Works\source\skyrim-moddings\skyrim-runefall\Runefall\build\evidence'
New-Item -ItemType Directory -Force $evidenceDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$report = Join-Path $evidenceDir ("uninstall-$Mode-$stamp.md")
$startTime = Get-Date
$checks = New-Object System.Collections.Generic.List[string]
$failures = New-Object System.Collections.Generic.List[string]

if (-not $SaveName) {
    $manifest = 'D:\Works\source\skyrim-moddings\_context\clean-save\CURRENT-CLEAN-SAVE.txt'
    $line = Get-Content $manifest | Where-Object { $_ -like 'saveName=*' } | Select-Object -First 1
    $SaveName = $line -replace '^saveName=', ''
}
function Add-Check([string]$name, [bool]$ok, [string]$detail = '') {
    $mark = if ($ok) { 'PASS' } else { 'FAIL' }
    $checks.Add("| $name | $mark | $detail |")
    if (-not $ok) { $failures.Add("$name $detail") }
    Write-Host "[$mark] $name $detail"
}
function Invoke-Mcp([string]$tool, [hashtable]$arguments = @{}) {
    $raw = & $Node $McpClient --call $tool --args ($arguments | ConvertTo-Json -Compress -Depth 5) 2>&1 | Out-String
    return ($raw -replace '\\"', '"')
}

$pluginsBackup = Get-Content $pluginsTxt -Raw
$dllMoved = $false
try {
    $running = Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.ExecutablePath -like "$GameRoot*" }
    if ($running) { throw 'Skyrim is already running; close it before an uninstall test.' }

    $lines = Get-Content $pluginsTxt
    if ($Mode -in @('pack-disable','all-remove')) {
        $lines = $lines | Where-Object { $_ -notmatch 'Runefall - (Unique|Hunt)\.esp' }
    }
    if ($Mode -eq 'all-remove') {
        $lines = $lines | Where-Object { $_ -notmatch 'Runefall( - (Unique|Hunt))?\.esp' }
    }
    Set-Content -Path $pluginsTxt -Value $lines
    if ($Mode -in @('dll-remove','all-remove')) {
        if (Test-Path $dll) { Move-Item $dll $dllOff -Force; $dllMoved = $true }
        $pdb = [System.IO.Path]::ChangeExtension($dll, '.pdb')
        if (Test-Path $pdb) { Move-Item $pdb "$pdb.disabled" -Force }
    }

    Start-Process -FilePath (Join-Path $GameRoot 'skse64_loader.exe') -WorkingDirectory $GameRoot
    $deadline = (Get-Date).AddSeconds(60)
    $connected = $false
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 2
        if ((Invoke-Mcp 'check_connection') -match '"connected":true') { $connected = $true; break }
    }
    Add-Check 'MCP connected' $connected ''
    $load = Invoke-Mcp 'load_save' @{ saveName = $SaveName }
    Add-Check 'Clean save load requested' ($load -match '"loading":true') ''
    $deadline = (Get-Date).AddSeconds(70)
    $player = ''
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 2
        $player = Invoke-Mcp 'get_player_info'
        if ($player -match '"name":"([^"]+)"') { break }
    }
    Add-Check 'Player loaded' ($player -match '"name":"CleanSave"') ''
    Start-Sleep -Seconds 20
    $logText = if (Test-Path (Join-Path $skseDir 'Runefall.log')) { Get-Content (Join-Path $skseDir 'Runefall.log') -Raw } else { '' }
    if ($Mode -eq 'pack-disable') {
        Add-Check 'Base DLL loaded' ((Invoke-Mcp 'get_loaded_skse_plugins') -match 'Runefall\.dll') ''
        Add-Check 'Base plugin loaded' ((Invoke-Mcp 'get_load_order') -match 'Runefall\.esp') ''
        Add-Check 'Pack absence tolerated' (($logText -match 'Runefall v0') -and ($logText -notmatch '\[error\]')) ''
    } else {
        $dllLoaded = (Invoke-Mcp 'get_loaded_skse_plugins') -match 'Runefall\.dll'
        Add-Check 'Runefall DLL absent' (-not $dllLoaded) ''
    }
    $latestCrash = Get-ChildItem $skseDir -Filter 'crash-*.log' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    $crashed = $latestCrash -and $latestCrash.LastWriteTime -gt $startTime
    Add-Check 'No new crash log' (-not $crashed) ($(if ($latestCrash) { $latestCrash.Name } else { 'none' }))
    Add-Check 'Game still running' ([bool](Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.ExecutablePath -like "$GameRoot*" })) ''
}
finally {
    Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.ExecutablePath -like "$GameRoot*" } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
    Start-Sleep -Seconds 2
    Set-Content -Path $pluginsTxt -Value $pluginsBackup -NoNewline
    if ($dllMoved -and (Test-Path $dllOff)) { Move-Item $dllOff $dll -Force }
    $pdbOff = [System.IO.Path]::ChangeExtension($dll, '.pdb') + '.disabled'
    $pdb = [System.IO.Path]::ChangeExtension($dll, '.pdb')
    if (Test-Path $pdbOff) { Move-Item $pdbOff $pdb -Force }
    $result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
    $lines = @("# Runefall uninstall test - $Mode - $stamp", '', "- Result: $result", '', '| Check | Result | Detail |', '|---|---|---|') + $checks
    Set-Content -Path $report -Value $lines -Encoding UTF8
    Write-Host "Report: $report"
}
if ($failures.Count -gt 0) { exit 1 }
exit 0
