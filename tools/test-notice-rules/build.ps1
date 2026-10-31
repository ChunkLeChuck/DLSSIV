# Builds tools\test-notice-rules\nr.exe (x86, as the game): the notice's rules. prove.py writes nr_cut.h (the rules' own
# text, cut from source\notice.h) beside nr.cpp first and runs this; nr.cpp says what it does. No Direct3D.
# -Out <file>: another name for the exe (prove.py builds its controls beside the real one).
param([string]$Out = '')
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
if (-not (Test-Path (Join-Path $here 'nr_cut.h'))) { throw 'nr_cut.h is not there: run prove.py, which cuts it from the source' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }
$cpp = Join-Path $here 'nr.cpp'; $exe = if ($Out) { $Out } else { Join-Path $here 'nr.exe' }; $obj = Join-Path $env:TEMP ('nr-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 `"$cpp`" /Fe:`"$exe`" /Fo:`"$obj`" /link /INCREMENTAL:NO"
# the old exe goes first: a failed build must leave none, or a run chained after it prints the OLD tool's verdict
Remove-Item -LiteralPath $exe -ErrorAction SilentlyContinue
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
$code = $LASTEXITCODE
Remove-Item $obj -ErrorAction SilentlyContinue
if ($code -ne 0 -or -not (Test-Path $exe)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'nr.exe did not build' }
Write-Host ("{0}: {1:N0} bytes" -f (Split-Path -Leaf $exe), (Get-Item $exe).Length) -ForegroundColor Green
