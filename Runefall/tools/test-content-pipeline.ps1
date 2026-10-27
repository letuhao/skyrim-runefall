param(
    [string]$Project = (Join-Path $PSScriptRoot "..")
)
$ErrorActionPreference = "Stop"
$Project = (Resolve-Path $Project).Path
$dotnet = "C:\Program Files\dotnet\dotnet.exe"
$generatorProject = Join-Path $Project "tools\Runefall.Build"
$generator = Join-Path $generatorProject "bin\Release\net8.0\Runefall.Build.dll"
$temp = Join-Path $env:TEMP "runefall-content-pipeline-test"
if (Test-Path $temp) { Remove-Item $temp -Recurse -Force }
New-Item -ItemType Directory -Force $temp | Out-Null

& $dotnet build $generatorProject -c Release --nologo *> (Join-Path $temp "build.log")
if ($LASTEXITCODE -ne 0) { throw "generator build failed" }

$validOut = Join-Path $temp "valid-out"
$valid = & $dotnet $generator --out $validOut --pack (Join-Path $Project "content\base\pack.json") 2>&1
$valid | Set-Content (Join-Path $temp "valid.log")
if ($LASTEXITCODE -ne 0 -or -not (($valid | Out-String) -match "LADDER PASS")) {
    throw "valid pack did not build: $($valid -join '; ')"
}

$badPack = Join-Path $temp "bad-pack"
Copy-Item (Join-Path $Project "content\base") $badPack -Recurse
$stagePath = Join-Path $badPack "data\stage_ladder.json"
$stage = Get-Content $stagePath -Raw | ConvertFrom-Json
$stage.stages[0].PSObject.Properties.Remove("powerIndex")
$stage | ConvertTo-Json -Depth 100 | Set-Content $stagePath
$badOut = Join-Path $temp "bad-out"
$bad = & $dotnet $generator --out $badOut --pack (Join-Path $badPack "pack.json") 2>&1
$bad | Set-Content (Join-Path $temp "bad.log")
if ($LASTEXITCODE -eq 0) { throw "invalid pack was accepted" }
$badText = $bad | Out-String
if ($badText -notmatch "CONTENT PACK FAIL" -or $badText -notmatch "powerIndex") {
    throw "invalid pack error did not identify the field: $($badText.Trim())"
}
Write-Output "PASS: valid JSON pack builds; invalid pack rejected with field path"
exit 0
