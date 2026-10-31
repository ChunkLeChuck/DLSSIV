# Builds tools\test-mask\mk.exe (x86, as the game): the mask's proof, the shipping shader
# (source\shaders\ivt_mask_ps.h) drawn by the shipping calls (source\transport\mask_pass.h) on the GPU against
# pictures of known answer; mk.cpp says what it proves. The same Visual Studio lookup as tools\test-rain.
# (This file was committed EMPTY on 2026-10-10, a7e6a26d, and written again the same day.)
#
#     tools\test-mask\build.ps1
#     tools\test-mask\mk.exe --d3d9 external\dxvk\x32\d3d9.dll [--timing]
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }
$cpp = Join-Path $here 'mk.cpp'; $out = Join-Path $here 'mk.exe'; $obj = Join-Path $env:TEMP ('mk-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
# the old exe goes first: a failed build must leave none, or a run chained after it prints the OLD tool's verdict
Remove-Item -LiteralPath $out -ErrorAction SilentlyContinue
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
$code = $LASTEXITCODE
Remove-Item $obj -ErrorAction SilentlyContinue
if ($code -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'mk.exe did not build' }
Write-Host ("mk.exe: {0:N0} bytes" -f (Get-Item $out).Length) -ForegroundColor Green
