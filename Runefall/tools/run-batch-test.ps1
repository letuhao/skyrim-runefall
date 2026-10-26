<#
.SYNOPSIS
  Automated in-game batch test for Runefall using the SkyLink MCP server.
.DESCRIPTION
  Loads the clean save, queries player/cell/inventory/SKSE plugins, exercises F10/F11/F12
  through hardware SendInput, reads Runefall logs, and (optionally) spawns and kills a
  Draugr to exercise loot/death-drop/hunt-first-kill. Writes an evidence report and exits
  non-zero if a crash or failed self-test is detected.
.PARAMETER ExerciseDeathDrops
  Spawn a Draugr with the console and kill it to exercise the death hook. Mutates the test
  save (adds a corpse/probe item); do not use against a real player save.
#>
[CmdletBinding()]
param(
    [string]$GameRoot = 'J:\SkyrimTest\1.6.1170',
    [string]$SaveName = '',
    [string]$McpClient = 'D:\Works\source\skyrim-moddings\.agents\skills\skyrim-mcp-control\scripts\mcp-client.js',
    [string]$Node = 'C:\nvm4w\nodejs\node.exe',
    [int]$StartupTimeoutSeconds = 40,
    [int]$LoadTimeoutSeconds = 40,
    [int]$ActivationWaitSeconds = 15,
    [switch]$ExerciseDeathDrops,
    [switch]$NoInputHotkeys
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

function Add-Check([string]$name, [bool]$ok, [string]$detail = '') {
    $mark = if ($ok) { 'PASS' } else { 'FAIL' }
    $checks.Add("| $name | $mark | $detail |")
    if (-not $ok) { $failures.Add("$name $detail") }
    Write-Host ("[{0}] {1} {2}" -f $mark, $name, $detail)
}

function Invoke-Mcp([string]$tool, [hashtable]$arguments = @{}) {
    $argsJson = ($arguments | ConvertTo-Json -Compress -Depth 5)
    $raw = & $Node $McpClient --call $tool --args $argsJson 2>&1 | Out-String
    # MCP embeds tool JSON as escaped text; normalize quotes before regex checks.
    return ($raw -replace '\\"', '"')
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

function Focus-Game {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class RFWindow {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@ -ErrorAction SilentlyContinue
    $p = Get-Process SkyrimSE -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($p -and $p.MainWindowHandle -ne 0) {
        [RFWindow]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
        Start-Sleep -Milliseconds 800
    }
}

function Send-Key([int]$vk) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class RFInput {
  [StructLayout(LayoutKind.Sequential)] public struct INPUT { public uint type; public KEYBDINPUT ki; }
  [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
  [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] p, int cb);
  public static void Key(ushort vk) {
    INPUT[] down = { new INPUT { type = 1, ki = new KEYBDINPUT { wVk = vk } } };
    INPUT[] up = { new INPUT { type = 1, ki = new KEYBDINPUT { wVk = vk, dwFlags = 2 } } };
    SendInput(1, down, Marshal.SizeOf(typeof(INPUT)));
    System.Threading.Thread.Sleep(80);
    SendInput(1, up, Marshal.SizeOf(typeof(INPUT)));
  }
}
"@ -ErrorAction SilentlyContinue
    [RFInput]::Key([uint16]$vk)
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
Add-Check 'Load save requested' ($load -match '"loading":true') $SaveName
$loadDeadline = (Get-Date).AddSeconds($LoadTimeoutSeconds)
$player = ''
while ((Get-Date) -lt $loadDeadline) {
    Start-Sleep -Seconds 2
    $player = Invoke-Mcp 'get_player_info'
    if ($player -match '"name":"([^"]+)"') { break }
}
$playerName = if ($player -match '"name":"([^"]+)"') { $Matches[1] } else { '' }
Add-Check 'Player loaded' ($playerName -ne '') "name=$playerName"
$cell = Invoke-Mcp 'get_cell_info'
Add-Check 'Cell query' ($cell -match '"formId"') ''
$inv = Invoke-Mcp 'get_inventory'
Add-Check 'Inventory query' ($inv -match '"totalItems"') ''
$plugins = Invoke-Mcp 'get_loaded_skse_plugins'
Add-Check 'Runefall plugin loaded' ($plugins -match 'Runefall.dll') ''
$menu = Invoke-Mcp 'get_menu_state'
Add-Check 'Menu state query' ($menu -match '"gameIsPaused"') ''

Start-Sleep -Seconds $ActivationWaitSeconds
$logText = if (Test-Path $runefallLog) { Get-Content $runefallLog -Raw } else { '' }
$probeText = if (Test-Path $probeLog) { Get-Content $probeLog -Raw } else { '' }
Add-Check 'Runefall loaded' ($logText -match 'Runefall v0') ''
Add-Check 'Self-tests 18/18' ($probeText -match 'pass=18\s+warn=0\s+fail=0' -or $logText -match 'pass=18\s+warn=0\s+fail=0') ''
$activation = if ($logText -match 'gameplay activation scheduled in (\d+) seconds') { [int]$Matches[1] } else { -1 }
Add-Check 'Gameplay activation scheduled' ($activation -ge 0) "delay=$activation sec"

if (-not $NoInputHotkeys) {
    try {
        $logBefore = if (Test-Path $runefallLog) { (Get-Content $runefallLog -Raw).Length } else { 0 }
        $wshell = New-Object -ComObject WScript.Shell
        $activated = $wshell.AppActivate('Skyrim Special Edition')
        Focus-Game
        Add-Check 'Game window activated' ([bool]$activated) ''
        Start-Sleep -Seconds 2
        Send-Key 0x79  # F10 snapshot
        Start-Sleep -Seconds 2
        Send-Key 0x7A  # F11 self-tests
        Start-Sleep -Seconds 2
        Send-Key 0x7B  # F12 probe injection
        Start-Sleep -Seconds 3
        $logText = Get-Content $runefallLog -Raw
        $newLog = if ($logText.Length -gt $logBefore) { $logText.Substring($logBefore) } else { '' }
        Add-Check 'F10 menu snapshot' ($newLog -match 'input: menu snapshot') ''
        Add-Check 'F11 selftest rerun' ($newLog -match 'probe: pass=') ''
        Add-Check 'F12 probe path' ($newLog -match 'probe item injected|probe item injection disabled') ''
        if ($newLog.Length -eq 0) { Add-Check 'New hotkey log lines' $false 'no log growth after SendInput' }
    } catch {
        Add-Check 'Input hotkeys' $false $_.Exception.Message
    }
}

if ($ExerciseDeathDrops) {
    $spawn = Invoke-Mcp 'execute_console_command' @{ command = 'player.placeatme 000EBC2D 1' }
    Add-Check 'Spawn test Draugr' ($spawn -match '"executed":true') ''
    Start-Sleep -Seconds 3
    $near = Invoke-Mcp 'get_nearby_np_cs' @{ radius = 2500 }
    if ($near -match '000EBC2D.*?(FF[0-9A-F]+)') {
        $ref = $Matches[1]
        $kill = Invoke-Mcp 'kill_actor' @{ refId = $ref }
        Add-Check 'Kill test Draugr' ($kill -match '"killed":true') "ref=$ref"
        Start-Sleep -Seconds 5
        $logText = Get-Content $runefallLog -Raw
        Add-Check 'Hunt first kill' ($logText -match 'hunt: first kill family') ''
        Add-Check 'Hunt material grant' ($logText -match 'hunt: granted first-kill material') ''
        Add-Check 'Hunt unique item grant' ($logText -match 'hunt: granted unique affix') ''
        $corpse = Invoke-Mcp 'get_npc_inventory' @{ refId = $ref }
        Add-Check 'Corpse has unique item' ($corpse -match 'Runed|enchantment') ''
    } else {
        Add-Check 'Find test Draugr' $false 'no spawned Draugr in nearby list'
    }
}

$latestCrash = Get-ChildItem $skseDir -Filter 'crash-*.log' -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
$crashed = $latestCrash -and $latestCrash.LastWriteTime -gt $startTime
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
    ($(if (Test-Path $runefallLog) { Get-Content $runefallLog -Tail 25 } else { 'missing' }) -join [Environment]::NewLine),
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