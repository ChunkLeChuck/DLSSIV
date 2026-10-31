# Builds tools\test-dedither\dd.exe (x86, as the game): the dither pass's picture proof, the shipping shader
# (source\shaders\ivt_dedither_ps.h and its view) drawn by the shipping draw (source\transport\dedither_pass.h) on the
# GPU against pictures of known answer; dd.cpp says what it proves. The same Visual Studio lookup as tools\test-hdr-skip.
# The negative controls are wrong DRAWS (a POINT sampler, no mark, another size's texel), made in dd.cpp itself. One
# shader of the tool's own is compiled into %TEMP%: dd_ref_ps.hlsl, the pass's FIRST form, which proof 9 holds the
# shipping pass's picture to.
#
#     tools\test-dedither\build.ps1
#     tools\test-dedither\dd.exe --d3d9 "<a game folder>\vulkan.dll" [--timing]
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }
$fxc = $env:FXC_EXE
if (-not $fxc) {
    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    if (Test-Path $kits) {
        $found = Get-ChildItem -Path $kits -Filter 'fxc.exe' -Recurse -ErrorAction SilentlyContinue |
                 Where-Object { $_.FullName -match '\\x64\\' } | Sort-Object FullName -Descending | Select-Object -First 1
        if ($found) { $fxc = $found.FullName }
    }
}
if (-not $fxc -or -not (Test-Path $fxc)) { throw "fxc.exe not found under the Windows 10 SDK. Set FXC_EXE to its full path." }
$gen = Join-Path $env:TEMP ('dd-gen-' + $PID)
New-Item -ItemType Directory -Force -Path $gen | Out-Null
& $fxc /nologo /T ps_3_0 /E main /O3 /Fh (Join-Path $gen 'dd_ref_ps.h') /Vn kDdRefPS (Join-Path $here 'dd_ref_ps.hlsl') | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'fxc failed on dd_ref_ps.hlsl' }
$cpp = Join-Path $here 'dd.cpp'; $out = Join-Path $here 'dd.exe'; $obj = Join-Path $env:TEMP ('dd-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict /I`"$gen`" `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
# the old exe goes first: a failed build must leave none, or a run chained after it prints the OLD tool's verdict
Remove-Item -LiteralPath $out -ErrorAction SilentlyContinue
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'dd.exe did not build' }
Remove-Item $obj -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $gen -ErrorAction SilentlyContinue
Write-Host ("dd.exe: {0:N0} bytes" -f (Get-Item $out).Length) -ForegroundColor Green
