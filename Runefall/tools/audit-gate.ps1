# Runefall audit gate wrapper. No rebuild, no game, no external absolute paths.
param(
    [string]$StatePath = "EXECUTION-STATE.json",
    [ValidateSet("truthfulness","release")][string]$Mode = "truthfulness",
    [switch]$Bootstrap,
    [switch]$SkipGitCheck
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$state = if ([IO.Path]::IsPathRooted($StatePath)) { $StatePath } else { Join-Path $repo $StatePath }
$args = @((Join-Path $repo "tools\audit-validators\audit_state.py"), "--state", $state, "--repo", $repo, "--mode", $Mode)
if ($Bootstrap) { $args += "--bootstrap" }
if ($SkipGitCheck) { $args += "--skip-git" }
& python @args
exit $LASTEXITCODE
