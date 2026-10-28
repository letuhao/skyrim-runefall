# Runs all static gates and writes an evidence bundle under build/evidence.
param(
    [string]$Project = (Join-Path $PSScriptRoot ".."),
    [string]$PapyrusCompiler = "E:\SteamLibrary\steamapps\common\Skyrim Special Edition\Papyrus Compiler\PapyrusCompiler.exe",
    [string]$PapyrusSource = "E:\SteamLibrary\steamapps\common\Skyrim Special Edition\Data\Source\Scripts",
    [string]$BuildScript = "C:\Users\NeneScarlet\.cache\build-runefall.bat"
)
$ErrorActionPreference = "Stop"
$Project = (Resolve-Path $Project).Path
$evidence = Join-Path $Project "build\evidence"
New-Item -ItemType Directory -Force $evidence | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$log = Join-Path $evidence "static-gates-$stamp.md"
$lines = New-Object System.Collections.Generic.List[string]
$failures = 0

function Gate($name, [scriptblock]$body) {
    Write-Output "GATE: $name"
    try {
        $result = & $body
        $detail = if ($result) { ": $result" } else { "" }
        $lines.Add("PASS  $name$detail")
        return $true
    } catch {
        $script:failures++
        $lines.Add("FAIL  ${name}: $($_.Exception.Message)")
        return $false
    }
}

Gate "native DLL build" {
    cmd /c $BuildScript *> (Join-Path $evidence "native-build-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "native build exit $LASTEXITCODE" }
    $dll = Join-Path $Project "src\plugin\build\msvc\Runefall.dll"
    if (-not (Test-Path $dll)) { throw "Runefall.dll missing" }
    "hash=" + (Get-FileHash $dll -Algorithm SHA256).Hash
}

Gate "content generator" {
    $dotnet = "C:\Program Files\dotnet\dotnet.exe"
    $proj = Join-Path $Project "tools\Runefall.Build"
    & $dotnet build $proj -c Release --nologo *> (Join-Path $evidence "generator-build-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "generator build exit $LASTEXITCODE" }
    $dll = Join-Path $proj "bin\Release\net8.0\Runefall.Build.dll"
    $out = & $dotnet $dll --out (Join-Path $Project "build\dist") 2>&1
    $out | Set-Content (Join-Path $evidence "generator-run-$stamp.log")
    $text = $out | Out-String
    if (-not ($text -match "RESULT: PASS")) { throw "generator validation did not PASS" }
    ($out | Select-String "PASS plugin" | Measure-Object).Count.ToString() + " plugin checks"
}

Gate "JSON schemas and base pack" {
    $out = & python (Join-Path $Project "content\schema\validate_content.py") 2>&1
    $out | Set-Content (Join-Path $evidence "json-schemas-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "JSON schema validation failed: $($out -join '; ')" }
    ($out | Select-Object -Last 1)
}

Gate "JSON content pipeline" {
    $out = & (Join-Path $Project "tools\test-content-pipeline.ps1") 2>&1
    $out | Set-Content (Join-Path $evidence "json-pipeline-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "JSON content pipeline failed: $($out -join '; ')" }
    ($out | Select-Object -Last 1)
}

Gate "English string table" {
    $strings = Join-Path $Project "build\dist\Data\Runefall\data\strings.json"
    $out = & python (Join-Path $Project "tools\validate-strings.py") $strings 2>&1
    $out | Set-Content (Join-Path $evidence "strings-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "English string table validation failed: $($out -join '; ')" }
    ($out | Select-Object -Last 1)
}

Gate "host core tests" {
    $out = & (Join-Path $Project "tools\run-host-tests.ps1") 2>&1
    $out | Set-Content (Join-Path $evidence "host-core-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "host core tests failed: $($out -join '; ')" }
    ($out | Select-Object -Last 1)
}

Gate "Papyrus policy" {
    $policy = & (Join-Path $Project "tools\check-papyrus-policy.ps1") -Project $Project 2>&1
    $policy | Set-Content (Join-Path $evidence "papyrus-policy-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "Papyrus policy failed: $($policy -join '; ')" }
    ($policy | Select-Object -Last 1)
}

Gate "Papyrus compile" {
    $out = Join-Path $Project "build\dist\Data\Scripts"
    New-Item -ItemType Directory -Force $out | Out-Null
    foreach ($script in Get-ChildItem (Join-Path $Project "src\papyrus") -Filter "*.psc") {
        & $PapyrusCompiler $script.FullName "-f=$(Join-Path $PapyrusSource 'TESV_Papyrus_Flags.flg')" "-i=$(Join-Path $Project 'src\papyrus');$PapyrusSource" "-o=$out" *> (Join-Path $evidence ("papyrus-" + $script.BaseName + "-$stamp.log"))
        if ($LASTEXITCODE -ne 0) { throw "Papyrus compile failed: $($script.Name)" }
    }
    "compiled " + (Get-ChildItem $out -Filter "Runefall*.pex").Count + " PEX"
}

Gate "FOMOD XML" {
    foreach ($xml in Get-ChildItem (Join-Path $Project "fomod") -Filter "*.xml") {
        [xml]$doc = Get-Content $xml.FullName -Raw
        if (-not $doc.DocumentElement) { throw "empty XML: $($xml.Name)" }
    }
    "info.xml + ModuleConfig.xml parsed"
}

Gate "release tree" {
    & (Join-Path $Project "tools\build-release.ps1") -Project $Project *> (Join-Path $evidence "release-$stamp.log")
    if ($LASTEXITCODE -ne 0) { throw "release build exit $LASTEXITCODE" }
    $zip = Get-ChildItem (Join-Path $Project "build\release") -Filter "Runefall-*.zip" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $zip) { throw "release archive missing" }
    "archive=" + $zip.Name + " sha256=" + (Get-FileHash $zip.FullName -Algorithm SHA256).Hash
}

$header = @("# Static gates - $stamp", "", "Project: $Project", "")
$header + $lines | Set-Content $log
Write-Output ""
Write-Output ("GATES: " + ($lines | Where-Object { $_ -like "PASS*" }).Count + " pass, " + $failures + " fail")
Write-Output "evidence: $log"
if ($failures -gt 0) { exit 1 }
