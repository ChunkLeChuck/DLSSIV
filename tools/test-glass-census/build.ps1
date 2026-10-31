# Builds tools\test-glass-census\gc.exe (x86, as the game): the glass census's proof. prove.py writes gc_cut.h (the
# census's own text, cut from source\DLSS-IV.cpp) beside gc.cpp first and runs this; gc.cpp says what it plays.
# The same Visual Studio lookup as tools\test-notice.
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
if (-not (Test-Path (Join-Path $here 'gc_cut.h'))) { throw 'gc_cut.h is not there: run prove.py, which cuts it from the source' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }
$cpp = Join-Path $here 'gc.cpp'; $out = Join-Path $here 'gc.exe'; $obj = Join-Path $env:TEMP ('gc-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
# the old exe goes first: a failed build must leave none, or a run chained after it prints the OLD tool's verdict
Remove-Item -LiteralPath $out -ErrorAction SilentlyContinue
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
$code = $LASTEXITCODE
Remove-Item $obj -ErrorAction SilentlyContinue
if ($code -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'gc.exe did not build' }
Write-Host ("gc.exe: {0:N0} bytes" -f (Get-Item $out).Length) -ForegroundColor Green
