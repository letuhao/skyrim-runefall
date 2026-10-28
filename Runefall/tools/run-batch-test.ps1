<#
.SYNOPSIS
  Automated in-game batch test for Runefall using the SkyLink MCP server.
.DESCRIPTION
  Loads the clean save, queries player/cell/inventory/SKSE plugins, exercises F10/F11/F12 through
  SkyLink's Input.TapKey, then exercises the P4.2/P3.7 runtime paths: container loot generation,
  actor-rank buffs, quest-table rewards, rune-item aggregation, co-save round-trip and the
  death-drop/hunt-first-kill path. Writes an evidence report and exits non-zero on failure.
.PARAMETER ExerciseDeathDrops
  Spawn a Draugr with the console and kill it to exercise the death hook. Mutates the test save
  (adds a corpse/probe item); do not use against a real player save.
#>
[CmdletBinding()]
param(
    [string]$GameRoot = 'J:\SkyrimTest\1.6.1170',
    [string]$SaveName = '',
    [string]$McpClient = 'D:\Works\source\skyrim-moddings\.agents\skills\skyrim-mcp-control\scripts\mcp-client.js',
    [string]$Node = 'C:\nvm4w\nodejs\node.exe',
    [int]$StartupTimeoutSeconds = 40,
    [int]$LoadTimeoutSeconds = 40,
    [int]$ActivationWaitSeconds = 18,
    [switch]$ExerciseDeathDrops,
    [switch]$NoInputHotkeys,
    [switch]$DisableActorEquipment,
    [switch]$TestEncounters,
    [switch]$TestHuntContract
)

$ErrorActionPreference = 'Stop'
$startTime = Get-Date
$failures = New-Object System.Collections.Generic.List[string]
$checks = New-Object System.Collections.Generic.List[string]
$evidenceDir = 'D:\Works\source\skyrim-moddings\skyrim-runefall\Runefall\build\evidence'
New-Item -ItemType Directory -Force -Path $evidenceDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$reportPath = Join-Path $evidenceDir ("batch-test-{0}.md" -f $stamp)
$skseDir = Join-Path $env:USERPROFILE 'Documents\My Games\Skyrim Special Edition\SKSE'
$runefallLog = Join-Path $skseDir 'Runefall.log'
$probeLog = Join-Path $GameRoot 'Data\SKSE\Plugins\RunefallProbe.log'
$iniPath = Join-Path $GameRoot 'Data\Runefall\runefall.ini'
if (Test-Path $iniPath) {
    $ini = Get-Content $iniPath -Raw
    $ini = $ini -replace 'iGameplayDelaySeconds=\d+', 'iGameplayDelaySeconds=8'
    $ini = $ini -replace 'iRuntimeScanDelaySeconds=\d+', 'iRuntimeScanDelaySeconds=1'
    if ($TestEncounters) { $ini = $ini -replace 'bEnableEncounters=\d+', 'bEnableEncounters=1' }
    if ($TestHuntContract) { $ini = $ini -replace 'bEnableHuntContracts=\d+', 'bEnableHuntContracts=1' }
    if ($DisableActorEquipment) { $ini = $ini -replace 'bEnableActorEquipment=\d+', 'bEnableActorEquipment=0' }
    Set-Content -Path $iniPath -Value $ini -NoNewline
}

function Add-Check([string]$name, [bool]$ok, [string]$detail = '') {
    $mark = if ($ok) { 'PASS' } else { 'FAIL' }
    $checks.Add("| $name | $mark | $detail |")
    if (-not $ok) { $failures.Add("$name $detail") }
    Write-Host ("[{0}] {1} {2}" -f $mark, $name, $detail)
}

function Invoke-Mcp([string]$tool, [hashtable]$arguments = @{}) {
    $argsJson = ($arguments | ConvertTo-Json -Compress -Depth 5)
    $raw = & $Node $McpClient --call $tool --args $argsJson 2>&1 | Out-String
    return ($raw -replace '\\"', '"')
}

function Get-LogText {
    if (Test-Path $runefallLog) { return (Get-Content $runefallLog -Raw) }
    return ''
}

function Wait-LogPattern([string]$pattern, [int]$seconds = 10) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        if ((Get-LogText) -match $pattern) { return $true }
        Start-Sleep -Milliseconds 500
    }
    return ((Get-LogText) -match $pattern)
}

function Tap-Key([int]$scanCode) {
    return Invoke-Mcp 'call_papyrus_function' @{ className = 'Input'; functionName = 'TapKey'; args = "[$scanCode]" }
}

function Wait-Game {
    $deadline = (Get-Date).AddSeconds($StartupTimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $p = Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
            Where-Object { $_.ExecutablePath -eq (Join-Path $GameRoot 'SkyrimSE.exe') }
        if ($p) { return $true }
        Start-Sleep -Seconds 1
    }
    return $false
}

if (-not $SaveName) {
    $manifest = 'D:\Works\source\skyrim-moddings\_context\clean-save\CURRENT-CLEAN-SAVE.txt'
    if (Test-Path $manifest) {
        $line = Get-Content $manifest | Where-Object { $_ -like 'saveName=*' } | Select-Object -First 1
        if ($line) { $SaveName = ($line -replace '^saveName=', '') }
    }
}
if (-not $SaveName) { throw 'No saveName supplied and no clean-save manifest found.' }

Write-Host "Runefall batch test: save=$SaveName game=$GameRoot"
$proc = Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -eq (Join-Path $GameRoot 'SkyrimSE.exe') }
if (-not $proc) {
    Write-Host 'Starting Skyrim...'
    Start-Process -FilePath (Join-Path $GameRoot 'skse64_loader.exe') -WorkingDirectory $GameRoot
    if (-not (Wait-Game)) { throw 'Skyrim did not start within timeout.' }
} else {
    Write-Host "Using running Skyrim PID $($proc.ProcessId)"
}

$conn = ''
for ($attempt = 0; $attempt -lt 15; ++$attempt) {
    $conn = Invoke-Mcp 'check_connection'
    if ($conn -match '"connected":true') { break }
    Start-Sleep -Seconds 2
}
Add-Check 'MCP connected' ($conn -match '"connected":true') ''
$load = Invoke-Mcp 'load_save' @{ saveName = $SaveName }
$loadRequested = $load -match '"loading":true'
if (-not $loadRequested) {
    Start-Sleep -Seconds 2
    $load = Invoke-Mcp 'load_save' @{ saveName = $SaveName }
    $loadRequested = $load -match '"loading":true'
}
$loadDeadline = (Get-Date).AddSeconds($LoadTimeoutSeconds)
$player = ''
while ((Get-Date) -lt $loadDeadline) {
    Start-Sleep -Seconds 2
    $player = Invoke-Mcp 'get_player_info'
    if ($player -match '"name":"([^"]+)"') { break }
}
$playerName = if ($player -match '"name":"([^"]+)"') { $Matches[1] } else { '' }
Add-Check 'Player loaded' ($playerName -ne '') "name=$playerName"
Add-Check 'Load save requested' ($loadRequested -or $playerName -eq 'CleanSave') $SaveName
$cell = Invoke-Mcp 'get_cell_info'
Add-Check 'Cell query' ($cell -match '"formId"') ''
$inv = Invoke-Mcp 'get_inventory'
Add-Check 'Inventory query' ($inv -match '"totalItems"') ''
$plugins = Invoke-Mcp 'get_loaded_skse_plugins'
Add-Check 'Runefall plugin loaded' ($plugins -match 'Runefall.dll') ''
$menu = Invoke-Mcp 'get_menu_state'
Add-Check 'Menu state query' ($menu -match '"gameIsPaused"') ''

Start-Sleep -Seconds $ActivationWaitSeconds
$logText = Get-LogText
$probeText = if (Test-Path $probeLog) { Get-Content $probeLog -Raw } else { '' }
Add-Check 'Runefall loaded' ($logText -match 'Runefall v0') ''
$testMatch = if ($probeText -match 'pass=(\d+)\s+warn=0\s+fail=0') { [int]$Matches[1] } elseif ($logText -match 'pass=(\d+)\s+warn=0\s+fail=0') { [int]$Matches[1] } else { 0 }
Add-Check 'Self-tests all pass' ($testMatch -ge 19) "pass=$testMatch"
$activation = if ($logText -match 'gameplay activation scheduled in (\d+) seconds') { [int]$Matches[1] } else { -1 }
Add-Check 'Gameplay activation scheduled' ($activation -ge 0) "delay=$activation sec"
Add-Check 'Gameplay ready' (Wait-LogPattern 'gameplay-ready=true' 12) ''
Add-Check 'Damage event sink' (Wait-LogPattern 'damage-pipeline: TESHitEvent sink registered' 10) ''
Add-Check 'Damage hook installed' (Wait-LogPattern 'damage-pipeline: installed HandleHealthDamage hooks' 10) ''
Add-Check 'Load3D hooks installed' ($logText -match 'Load3D vtable hooks installed') ''
Add-Check 'Stage ladder loaded' ($logText -match 'stage-engine: loaded 100 stages, 10 tiers') ''

if (-not $NoInputHotkeys) {
    $okF10 = $false
    $okF11 = $false
    $okF12 = $false
    for ($attempt = 0; $attempt -lt 3 -and -not ($okF10 -and $okF11 -and $okF12); ++$attempt) {
        if (-not $okF10) {
            $null = Tap-Key 68
            $okF10 = Wait-LogPattern 'input: menu snapshot' 8
        }
        if (-not $okF11) {
            $null = Tap-Key 87
            $okF11 = Wait-LogPattern 'probe: pass=' 8
        }
        if (-not $okF12) {
            $null = Tap-Key 88
            $okF12 = Wait-LogPattern 'probe item injected|probe item injection disabled' 8
        }
        if (-not ($okF10 -and $okF11 -and $okF12)) { Start-Sleep -Seconds 2 }
    }
    $null = Tap-Key 64
Start-Sleep -Milliseconds 700
Add-Check 'Runtime content rescan' (Wait-LogPattern 'runtime-resolver: manual rescan removed' 10) ''
Add-Check 'F10 menu snapshot' $okF10 ''
Add-Check 'Delayed runtime scan' (Wait-LogPattern 'runtime-resolver: delayed startup scan complete' 15) ''
    Add-Check 'F11 selftest rerun' $okF11 ''
    Add-Check 'F12 probe path' $okF12 ''
}

# P4.2/P3.7 runtime paths. The spawned chest is a vanilla TESObjectCONT; Load3D must generate loot.
$spawnChest = Invoke-Mcp 'execute_console_command' @{ command = 'player.placeatme 0010FDE6 1' }
Add-Check 'Spawn test chest' ($spawnChest -match '"executed":true') ''
$containerGenerated = Wait-LogPattern 'container: generated' 12
Add-Check 'Container loot generated' $containerGenerated ''
if ($containerGenerated) {
    Add-Check 'Container item granted' ((Get-LogText) -match 'loot: container granted') ''
}

# Quest table consumer through the F9 debug hotkey (band 4 -> 1-3 items + runes).
$null = Tap-Key 67
Add-Check 'Quest reward granted' (Wait-LogPattern 'quest: debug reward band 4 granted' 12) ''

# Rune item-added path: F8 adds one R3 rune item; the sink must remove it and credit the wallet.
$null = Tap-Key 66
Add-Check 'Rune item aggregated' (Wait-LogPattern 'runes: aggregated 1 x R3' 12) ''

# Actor-rank path: spawned actor must receive a rank spell at Load3D.
$spawn = Invoke-Mcp 'execute_console_command' @{ command = 'player.placeatme 000EBC2D 1' }
if ($TestHuntContract) {
    $null = Tap-Key 62
    Add-Check 'Hunt contract started' (Wait-LogPattern 'hunt-contract: started ' 10) ''
}
Add-Check 'Spawn test Draugr' ($spawn -match '"executed":true') ''
Add-Check 'Actor package applied' (Wait-LogPattern 'actor-packager: package .* applied once' 12) ''
if ($DisableActorEquipment) {
    Add-Check 'Equipment suppressed' (-not ((Get-LogText) -match 'equipment: set .* applied')) ''
} else {
    Add-Check 'Actor equipment applied' (Wait-LogPattern 'equipment: set .* applied' 12) ''
    $equipmentLog = Get-LogText
Add-Check 'Equipment applied' (([regex]::Matches($equipmentLog, 'equipment: set .* applied')).Count -ge 1) ''
}
Start-Sleep -Seconds 2

if ($ExerciseDeathDrops) {
    $near = Invoke-Mcp 'get_nearby_np_cs' @{ radius = 2500 }
    if ($near -match '000EBC2D.*?(FF[0-9A-F]+)') {
        $ref = $Matches[1]
        $kill = Invoke-Mcp 'kill_actor' @{ refId = $ref }
        Add-Check 'Kill test Draugr' ($kill -match '"killed":true') "ref=$ref"
        Start-Sleep -Seconds 5
        $logText = Get-LogText
        Add-Check 'Hunt first kill' ($logText -match 'hunt: first kill family') ''
        Add-Check 'Hunt material grant' ($logText -match 'hunt: granted first-kill material') ''
        Add-Check 'Hunt unique item grant' ($logText -match 'hunt: granted unique affix') ''
        if ($TestHuntContract) { Add-Check 'Hunt contract completed' (Wait-LogPattern 'hunt-contract: completed ' 12) '' }
        $corpse = Invoke-Mcp 'get_npc_inventory' @{ refId = $ref }
        Add-Check 'Corpse has unique item' ($corpse -match 'Runed|enchantment') ''
    } else {
        Add-Check 'Find test Draugr' $false 'no spawned Draugr in nearby list'
    }
}

# Co-save round-trip: schema 3 must persist the generated-container/quest sets and not re-roll.
$null = Tap-Key 68
$null = Wait-LogPattern '"generatedContainers":[1-9][0-9]*' 10
$snapshotBefore = Get-LogText
$snapshotBeforeMatches = [regex]::Matches($snapshotBefore, '"generatedContainers":([0-9]+)')
$generatedBefore = 0
if ($snapshotBeforeMatches.Count -gt 0) { $generatedBefore = [int]$snapshotBeforeMatches[$snapshotBeforeMatches.Count - 1].Groups[1].Value }
Add-Check 'Co-save tracked containers' ($generatedBefore -gt 0) "count=$generatedBefore"
$saveProbe = Invoke-Mcp 'save_game' @{ saveName = 'RF_LOOT_PROBE' }
Add-Check 'Save probe session' ($saveProbe -match '"success":true') ''
Add-Check 'Co-save schema 3 written' (Wait-LogPattern 'co-save: wrote .*schema 3' 12) ''
$loadProbe = Invoke-Mcp 'load_save' @{ saveName = 'RF_LOOT_PROBE' }
Add-Check 'Reload probe session' ($loadProbe -match '"loading":true') ''
$reloadDeadline = (Get-Date).AddSeconds($LoadTimeoutSeconds)
while ((Get-Date) -lt $reloadDeadline) {
    Start-Sleep -Seconds 2
    $who = Invoke-Mcp 'get_player_info'
    if ($who -match '"name":"([^"]+)"') { break }
}
Start-Sleep -Seconds 12
Add-Check 'Co-save schema 3 loaded' (Wait-LogPattern 'co-save: loaded schema 3' 15) ''
$null = Tap-Key 68
Start-Sleep -Seconds 2
$snapshotText = Get-LogText
$snapshotMatches = [regex]::Matches($snapshotText, '"generatedContainers":([0-9]+)')
$generatedAfter = 0
if ($snapshotMatches.Count -gt 0) { $generatedAfter = [int]$snapshotMatches[$snapshotMatches.Count - 1].Groups[1].Value }
Add-Check 'Generated container set persisted' ($generatedAfter -ge $generatedBefore) "before=$generatedBefore after=$generatedAfter"

$latestCrash = Get-ChildItem $skseDir -Filter 'crash-*.log' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
$crashed = $latestCrash -and $latestCrash.LastWriteTime -gt $startTime
if ($TestEncounters) {
    $null = Tap-Key 63
    Add-Check 'Encounter spawned' (Wait-LogPattern '(encounter: spawned|rare-spawn: spawned)' 12) ''
    Add-Check 'Rare high-stage spawn' (Wait-LogPattern 'rare-spawn: spawned .* stage (8[0-9]|9[0-9]|100)' 12) ''
    Add-Check 'Boss fixed stage' (Wait-LogPattern 'boss-registry: actor .* fixed stage \d+' 12) ''
}
Add-Check 'No new crash log' (-not $crashed) ($(if ($latestCrash) { $latestCrash.Name + ' @ ' + $latestCrash.LastWriteTime } else { 'none' }))
$stillRunning = Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -eq (Join-Path $GameRoot 'SkyrimSE.exe') }
Add-Check 'Game still running' ([bool]$stillRunning) ''

$lines = @(
    "# Runefall batch test - $stamp",
    '',
    "- Game: $GameRoot",
    "- Save: $SaveName",
    "- Started: $($startTime.ToString('o'))",
    "- Duration: $([int]((Get-Date) - $startTime).TotalSeconds)s",
    '',
    '| Check | Result | Detail |',
    '|---|---|---|'
) + $checks + @(
    '',
    '## Runefall log tail',
    '~~~',
    ($(if (Test-Path $runefallLog) { Get-Content $runefallLog -Tail 40 } else { 'missing' }) -join [Environment]::NewLine),
    '~~~',
    '',
    "RESULT: $(if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' })"
)
Set-Content -Path $reportPath -Value $lines -Encoding UTF8
Write-Host "Report: $reportPath"
if ($failures.Count -gt 0) {
    Write-Host "FAILURES: $($failures -join '; ')"
    exit 1
}
exit 0
