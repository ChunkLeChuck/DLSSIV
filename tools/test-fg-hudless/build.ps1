# Builds hudless.exe (x86, as the game): the HUD-less picture's picture proof (hudless.cpp says what it proves), in a folder
# of this tool's own under %TEMP% (dlssiv-test-fg-hudless), and prints the exe's path as its last line.
#
# hudless.cpp includes tools\test-hdr-sharpen\g10.cpp whole, and that file includes the wrong encodes its own build
# generates. So this script runs that build first, WITH %TEMP% POINTED AT THIS TOOL'S FOLDER: g10's build empties
# %TEMP%\g10gen before it writes, and another worktree's gate may be reading its own at that moment.
# Nothing is written into the repository but g10.exe beside g10.cpp, as that build always does.
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$own = Join-Path $env:TEMP 'dlssiv-test-fg-hudless'
New-Item -ItemType Directory -Force $own | Out-Null
$old = @($env:TEMP, $env:TMP)
$env:TEMP = $own; $env:TMP = $own
try { & (Join-Path $here '..\test-hdr-sharpen\build.ps1') | Out-Null } finally { $env:TEMP = $old[0]; $env:TMP = $old[1] }
$gen = Join-Path $own 'g10gen'
if (-not (Test-Path (Join-Path $gen 'g10_wrong_mask.h'))) { throw "g10's generated headers are not in $gen" }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }
$cpp = Join-Path $here 'hudless.cpp'; $out = Join-Path $own 'hudless.exe'; $obj = Join-Path $own 'hudless.obj'
# (2026-10-11, proof h: the product's own Vulkan lines are compiled in: the transport's loader, Khronos's headers and source\fg)
$inc = "/I`"$(Join-Path $here '..\..\external\vulkan')`" /I`"$(Join-Path $here '..\..\source\transport')`" /I`"$(Join-Path $here '..\..\source\fg')`""
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict /DWIN32 /I`"$gen`" $inc `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'hudless.exe did not build' }
# THE SAME PROGRAM UNDER A NAME THE ONE-GPU-USER RULE LISTS (vklines*.exe): hudless.exe makes a real device on the card and
# times it, and a rule that reads the process list cannot see a name it does not know. Run this copy where others share the card.
$listed = Join-Path $own 'vklines_hudless.exe'
Copy-Item $out $listed -Force
Write-Host ("hudless.exe: {0:N0} bytes (and the same file as vklines_hudless.exe)" -f (Get-Item $out).Length) -ForegroundColor Green
Write-Host $out
