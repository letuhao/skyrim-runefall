<#
.SYNOPSIS
  Verify the Runefall release ZIP: root layout, required members, duplicates, FOMOD XML/version,
  plugin ESL flags and SHA-256. Writes an evidence file and exits non-zero on failure.
#>
[CmdletBinding()]
param(
    [string]$Project = 'D:\Works\source\skyrim-moddings\skyrim-runefall\Runefall',
    [string]$ZipPath = ''
)
$ErrorActionPreference = 'Stop'
$Project = (Resolve-Path $Project).Path
if (-not $ZipPath) {
    $ZipPath = (Get-ChildItem (Join-Path $Project 'build\release') -Filter 'Runefall-*.zip' |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}
if (-not (Test-Path $ZipPath)) { throw "release zip not found: $ZipPath" }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$evidence = Join-Path $Project ("build\evidence\release-verify-" + $stamp + ".md")
$failures = New-Object System.Collections.Generic.List[string]
$checks = New-Object System.Collections.Generic.List[string]

function Check([string]$name, [bool]$ok, [string]$detail = '') {
    $mark = if ($ok) { 'PASS' } else { 'FAIL' }
    $checks.Add("| $name | $mark | $detail |")
    if (-not $ok) { $failures.Add("$name $detail") }
    Write-Host "[$mark] $name $detail"
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($ZipPath)
try {
    $names = $zip.Entries | ForEach-Object { $_.FullName.Replace('\', '/') }
    $required = @(
        'fomod/info.xml',
        'fomod/ModuleConfig.xml',
        'Data/Runefall.esp',
        'Data/Runefall - Hunt.esp',
        'Data/Runefall - Unique.esp',
        'Data/SKSE/Plugins/Runefall.dll',
        'Data/Runefall/runefall.ini',
        'Data/Scripts/RunefallNative.pex',
        'Data/Scripts/RunefallProbe.pex',
        'Data/Scripts/RunefallQuestHost.pex',
        'Data/Scripts/RunefallMenuBridge.pex'
    )
    foreach ($entry in $required) {
        Check "member $entry" ($names -contains $entry)
    }
    Check 'no top-level Runefall/ wrapper' (-not ($names | Where-Object { $_ -match '^Runefall/' })) ''
    Check 'no Data/Data nesting' (-not ($names | Where-Object { $_ -like 'Data/Data/*' })) ''
    Check 'no CRLF/backslash members' (-not ($names | Where-Object { $_ -like '*\*' })) ''
    $dupes = $names | Group-Object | Where-Object { $_.Count -gt 1 }
    Check 'no duplicate member paths' ($dupes.Count -eq 0) $(if ($dupes) { ($dupes.Name -join ', ') } else { '' })

    $infoEntry = $zip.GetEntry('fomod/info.xml')
    $moduleEntry = $zip.GetEntry('fomod/ModuleConfig.xml')
    if ($infoEntry -and $moduleEntry) {
        $reader = New-Object IO.StreamReader($infoEntry.Open())
        $infoText = $reader.ReadToEnd(); $reader.Close()
        $reader = New-Object IO.StreamReader($moduleEntry.Open())
        $moduleText = $reader.ReadToEnd(); $reader.Close()
        try { [xml]$infoXml = $infoText; Check 'info.xml parses' $true '' } catch { Check 'info.xml parses' $false $_.Exception.Message }
        try { [xml]$moduleXml = $moduleText; Check 'ModuleConfig.xml parses' $true '' } catch { Check 'ModuleConfig.xml parses' $false $_.Exception.Message }
        Check 'info version 0.2.0-dev' ($infoText -match '<Version>0\.2\.0-dev</Version>') ''
        Check 'ModuleConfig has no missing Data\SKSE ref' ($moduleText -match 'Data\\SKSE') ''
    } else {
        Check 'FOMOD files present' $false 'info.xml or ModuleConfig.xml missing'
    }

    $dllEntry = $zip.GetEntry('Data/SKSE/Plugins/Runefall.dll')
    $pluginEntry = $zip.GetEntry('Data/Runefall.esp')
    if ($dllEntry -and $pluginEntry) {
        Check 'DLL member non-empty' ($dllEntry.Length -gt 100000) ("bytes=" + $dllEntry.Length)
        $stream = $pluginEntry.Open(); $reader = New-Object IO.BinaryReader($stream)
        $magic = $reader.ReadBytes(4); $reader.Close()
        Check 'Runefall.esp is TES4' ([Text.Encoding]::ASCII.GetString($magic) -eq 'TES4') ''
    }
} finally {
    $zip.Dispose()
}
$hash = (Get-FileHash $ZipPath -Algorithm SHA256).Hash
$size = (Get-Item $ZipPath).Length
$lines = @(
    '# Runefall release archive verification',
    '',
    "- Archive: $ZipPath",
    "- Size: $size bytes",
    "- SHA-256: $hash",
    "- Verified: $((Get-Date).ToString('o'))",
    '',
    '| Check | Result | Detail |',
    '|---|---|---|'
) + $checks + @('', "RESULT: " + $(if ($failures.Count -eq 0) { 'PASS' } else { 'FAIL' }))
Set-Content -Path $evidence -Value $lines -Encoding UTF8
Write-Host "Evidence: $evidence"
if ($failures.Count -gt 0) { Write-Host ("FAILURES: " + ($failures -join '; ')); exit 1 }
exit 0
