param(
    [string]$Project = (Join-Path $PSScriptRoot "..")
)
$ErrorActionPreference = "Stop"
$Project = (Resolve-Path $Project).Path
$build = Join-Path $Project "src\plugin\build\msvc"
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
cmd /c "call ""$vcvars"" >nul && ""$cmake"" --build ""$build"" --target RunefallCoreHost --config Release" *> (Join-Path $env:TEMP "runefall-host-build.log")
if ($LASTEXITCODE -ne 0) { throw "host build failed; see $env:TEMP\runefall-host-build.log" }
$exe = Join-Path $build "RunefallCoreHost.exe"
if (-not (Test-Path $exe)) { throw "RunefallCoreHost.exe missing" }
$out = & $exe (Join-Path $Project "build\dist\Data\Runefall\data") *>&1
$out | ForEach-Object { Write-Output $_ }
if ($LASTEXITCODE -ne 0) { throw "host test failed" }
exit 0
