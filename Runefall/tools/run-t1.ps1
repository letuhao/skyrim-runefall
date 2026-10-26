# Runs the single T1 batch smoke test in the standalone test environment.
# Refuses to run while any other Skyrim process is active (the user's Mantras game must be closed).
param(
    [string]$TestEnv = "J:\SkyrimTest\1.6.1170",
    [int]$WaitSeconds = 90,
    [switch]$StartNewGame,
    [switch]$KeepGameRunning
)
$ErrorActionPreference = "Stop"
$TestEnv = (Resolve-Path $TestEnv).Path
$exe = Join-Path $TestEnv "SkyrimSE.exe"
$loader = Join-Path $TestEnv "skse64_loader.exe"
if (-not (Test-Path $exe) -or -not (Test-Path $loader)) { throw "test environment is incomplete: $TestEnv" }

# Guard: never collide with the user's live game.
$running = Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue
$foreign = $running | Where-Object { $_.ExecutablePath -and $_.ExecutablePath -ne $exe }
if ($foreign) {
    $paths = ($foreign | ForEach-Object { $_.ExecutablePath }) -join ", "
    throw "Refusing T1: another Skyrim is already running ($paths). Close it first."
}

$plugins = Join-Path $env:LOCALAPPDATA "Skyrim Special Edition\Plugins.txt"
$backup = Join-Path $env:TEMP ("Plugins.backup-" + [guid]::NewGuid().ToString("N") + ".txt")
if (Test-Path $plugins) { Copy-Item $plugins $backup -Force }
$loadOrder = @(
    "# Runefall T1 temporary load order",
    "*Skyrim.esm", "*Update.esm", "*Dawnguard.esm", "*HearthFires.esm", "*Dragonborn.esm",
    "*unofficial skyrim special edition patch.esp", "*alternate start - live another life.esp",
    "*Runefall.esp", "*Runefall - Unique.esp", "*Runefall - Hunt.esp"
)

$skseLogDir = Join-Path $env:USERPROFILE "Documents\My Games\Skyrim Special Edition\SKSE"
$probe = Join-Path $TestEnv "Data\SKSE\Plugins\RunefallProbe.log"
$runefallLog = Join-Path $skseLogDir "Runefall.log"
$skseLog = Join-Path $skseLogDir "skse64.log"
$evidence = Join-Path (Split-Path $PSScriptRoot -Parent) "build\evidence\t1"
New-Item -ItemType Directory -Force $evidence | Out-Null

try {
    Set-Content -Path $plugins -Value $loadOrder -Encoding ASCII
    Remove-Item $probe, $runefallLog, $skseLog -Force -ErrorAction SilentlyContinue
    Write-Output "Starting T1 at $(Get-Date -Format o)"
    Start-Process -FilePath $loader -WorkingDirectory $TestEnv
    if ($StartNewGame) {
        Write-Output "Waiting for main menu, then starting a new game..."
        Start-Sleep -Seconds 30
        $wshell = New-Object -ComObject WScript.Shell
        $null = $wshell.AppActivate("Skyrim Special Edition")
        Start-Sleep -Milliseconds 800
        $wshell.SendKeys("{ENTER}")
        Write-Output "New-game input sent; waiting $($WaitSeconds - 30) more seconds"
        Start-Sleep -Seconds ($WaitSeconds - 30)
    } else {
        Start-Sleep -Seconds $WaitSeconds
    }

    $testProcess = Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.ExecutablePath -eq $exe }
    if ($testProcess) {
        Write-Output "Test game running (PID $($testProcess.ProcessId))"
    } else {
        Write-Output "Test game is not running (crashed or exited early)"
    }

    Copy-Item $runefallLog (Join-Path $evidence "Runefall.log") -Force -ErrorAction SilentlyContinue
    Copy-Item $probe (Join-Path $evidence "RunefallProbe.log") -Force -ErrorAction SilentlyContinue
    Copy-Item $skseLog (Join-Path $evidence "skse64.log") -Force -ErrorAction SilentlyContinue

    Write-Output "--- Runefall.log ---"
    if (Test-Path $runefallLog) { Get-Content $runefallLog } else { "NO Runefall.log" }
    Write-Output "--- RunefallProbe.log ---"
    if (Test-Path $probe) { Get-Content $probe } else { "NO probe log" }
    Write-Output "--- SKSE Runefall lines ---"
    if (Test-Path $skseLog) {
        Select-String -Path $skseLog -Pattern "Runefall|SKSEMenuFramework" | ForEach-Object { $_.Line }
    }
} finally {
    if (Test-Path $backup) { Copy-Item $backup $plugins -Force; Remove-Item $backup -Force -ErrorAction SilentlyContinue }
    if (-not $KeepGameRunning) {
        Get-CimInstance Win32_Process -Filter "name='SkyrimSE.exe'" -ErrorAction SilentlyContinue |
            Where-Object { $_.ExecutablePath -eq $exe } |
            ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
    }
    Write-Output "Plugins.txt restored; evidence: $evidence"
}