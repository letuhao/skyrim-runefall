<#
.SYNOPSIS
  Compatibility smoke test for one external mod in the disposable J: test environment.
.DESCRIPTION
  Copies a target Data tree into the test environment, appends its plugins, launches Skyrim,
  loads CleanSave, exercises the Runefall container/actor path and records crash evidence. The
  environment is restored (files removed, Plugins.txt restored, game stopped) in a finally block.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Name,
    [Parameter(Mandatory = $true)][string]$SourceData,
    [string[]]$Plugins = @(),
    [switch]$CopySpidKidSkyPatcher,
    [string]$GameRoot = 'J:\SkyrimTest\1.6.1170',
    [string]$SaveName = '',
    [string]$Node = 'C:\nvm4w\nodejs\node.exe',
    [string]$McpClient = 'D:\Works\source\skyrim-moddings\.agents\skills\skyrim-mcp-control\scripts\mcp-client.js',
    [int]$WaitSeconds = 90
)
$ErrorActionPreference = 'Stop'
$dataDir = Join-Path $GameRoot 'Data'
$pluginsTxt = Join-Path $env:LOCALAPPDATA 'Skyrim Special Edition\Plugins.txt'
$skseDir = Join-Path $env:USERPROFILE 'Documents\My Games\Skyrim Special Edition\SKSE'
$startTime = Get-Date
$evidenceDir = 'D:\Works\source\skyrim-moddings\skyrim-runefall\Runefall\build\evidence'
New-Item -ItemType Directory -Force $evidenceDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$report = Join-Path $evidenceDir ("compat-$Name-$stamp.md")
$copied = New-Object System.Collections.Generic.List[string]
$pluginsBackup = Get-Content $pluginsTxt -Raw
$failures = New-Object System.Collections.Generic.List[string]
$checks = New-Object System.Collections.Generic.List[string]

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
function Copy-Tree([string]$src, [string]$dstRoot) {
    if (-not (Test-Path $src)) { throw "source missing: $src" }
    Get-ChildItem -LiteralPath $src -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring((Resolve-Path $src).Path.Length).TrimStart('\')
        $dst = Join-Path $dstRoot $rel
        New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
        Copy-Item $_.FullName $dst -Force
        $copied.Add($dst)
    }
}
function Tap-Key([int]$code) { return Invoke-Mcp 'call_papyrus_function' @{ className = 'Input'; functionName = 'TapKey'; args = "[$code]" } }
function LogText { $p = Join-Path $skseDir 'Runefall.log'; if (Test-Path $p) { return (Get-Content $p -Raw) } return '' }
function Wait-Log([string]$pattern, [int]$seconds = 20) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) { if ((LogText) -match $pattern) { return $true }; Start-Sleep -Milliseconds 500 }
    return ((LogText) -match $pattern)
}

try {
    $running = Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.ExecutablePath -like "$GameRoot*" }
    if ($running) { throw 'Skyrim is already running; close it before a compatibility test.' }

    Write-Host "Copying $Name data from $SourceData"
    Copy-Tree $SourceData $dataDir
    if ($CopySpidKidSkyPatcher) {
        Copy-Tree 'H:\Skyrim\MantrasOfMara\mods\Spell Perk Item Distributor (SPID)\SKSE' (Join-Path $dataDir 'SKSE')
        Copy-Tree 'H:\Skyrim\MantrasOfMara\mods\Keyword Item Distributor (KID)\SKSE' (Join-Path $dataDir 'SKSE')
        Copy-Tree 'H:\Skyrim\MantrasOfMara\mods\SkyPatcher\SKSE' (Join-Path $dataDir 'SKSE')
    }
    if ($Plugins.Count -gt 0) {
        $existing = Get-Content $pluginsTxt
        foreach ($plugin in $Plugins) {
            if (-not ($existing | Where-Object { $_ -ieq "*$plugin" })) { Add-Content $pluginsTxt "*$plugin" }
            Add-Check "Plugin file present: $plugin" (Test-Path (Join-Path $dataDir $plugin)) ''
            Add-Check "Plugin active list contains: $plugin" ([bool]((Get-Content $pluginsTxt) | Where-Object { $_ -ieq "*$plugin" })) ''
        }
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
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    $player = ''
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 2
        $player = Invoke-Mcp 'get_player_info'
        if ($player -match '"name":"([^"]+)"') { break }
    }
    Add-Check 'Player loaded' ($player -match '"name":"CleanSave"') ''
    Add-Check 'Gameplay ready' (Wait-Log 'gameplay-ready=true' 25) ''
    Add-Check 'Load3D hooks installed' ((LogText) -match 'Load3D vtable hooks installed') ''
    $loadOrder = ''
    for ($attempt = 0; $attempt -lt 4; ++$attempt) {
        $loadOrder = Invoke-Mcp 'get_load_order'
        if ($loadOrder -match '"mods"') { break }
        Start-Sleep -Seconds 2
    }
    $loadOrderDump = Join-Path $evidenceDir ("compat-$Name-$stamp-loadorder.txt")
    Set-Content -Path $loadOrderDump -Value $loadOrder -Encoding UTF8
    Write-Host "load order dump: $loadOrderDump"
    $sksePlugins = Invoke-Mcp 'get_loaded_skse_plugins'
    foreach ($plugin in $Plugins) {
        Add-Check "Target plugin active: $plugin" ($loadOrder -match [regex]::Escape($plugin)) "loadOrderBytes=$($loadOrder.Length)"
    }
    if ($Plugins.Count -eq 0) {
        Add-Check 'Target SKSE DLL loaded' ($sksePlugins -match 'DiabloInSkyrim\.dll') ''
    }
    if ($CopySpidKidSkyPatcher) {
        Add-Check 'SPID loaded' ($sksePlugins -match 'po3_SpellPerkItemDistributor\.dll') ''
        Add-Check 'KID loaded' ($sksePlugins -match 'po3_KeywordItemDistributor\.dll') ''
        Add-Check 'SkyPatcher loaded' ($sksePlugins -match 'SkyPatcher\.dll') ''
    }

    $null = Invoke-Mcp 'execute_console_command' @{ command = 'player.placeatme 0010FDE6 1' }
    Add-Check 'Container path still runs' (Wait-Log 'container: generated' 20) ''
    $null = Invoke-Mcp 'execute_console_command' @{ command = 'player.placeatme 000EBC2D 1' }
    Add-Check 'Actor path still runs' (Wait-Log 'actor-packager: package .* applied once' 20) ''
    $questOk = $false
    for ($attempt = 0; $attempt -lt 3 -and -not $questOk; ++$attempt) {
        $null = Tap-Key 67
        $questOk = Wait-Log 'quest: debug reward band 4 granted' 10
    }
    Add-Check 'Quest path still runs' $questOk ''
    $runeOk = $false
    for ($attempt = 0; $attempt -lt 3 -and -not $runeOk; ++$attempt) {
        $null = Tap-Key 66
        $runeOk = Wait-Log 'runes: aggregated 1 x R3' 10
    }
    Add-Check 'Rune path still runs' $runeOk ''

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
    foreach ($file in $copied) { if (Test-Path $file) { Remove-Item $file -Force -ErrorAction SilentlyContinue } }
    Set-Content -Path $pluginsTxt -Value $pluginsBackup -NoNewline
    $result = if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }
    $lines = @("# Runefall compatibility test - $Name - $stamp", '', "- Source: $SourceData", "- Plugins: $($Plugins -join ', ')", "- Result: $result", '', '| Check | Result | Detail |', '|---|---|---|') + $checks + @('', '## Runefall log tail', '~~~', (Get-Content (Join-Path $skseDir 'Runefall.log') -Tail 30 -ErrorAction SilentlyContinue) -join [Environment]::NewLine, '~~~')
    Set-Content -Path $report -Value $lines -Encoding UTF8
    Write-Host "Report: $report"
}
if ($failures.Count -gt 0) { exit 1 }
exit 0
