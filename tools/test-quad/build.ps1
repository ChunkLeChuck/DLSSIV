# Builds tools\test-quad\qv.exe (x86, as the game): the proof of quad_uv, the hook's own reading and correcting code
# (source\transport\quad_uv.h) on the GPU against a stand-in for the game's distance pass; qv.cpp says what it proves.
# The same Visual Studio and fxc lookup as tools\test-corona. The tool's two shaders are compiled into %TEMP%; nothing
# of them ships.
#
#     tools\test-quad\build.ps1
#     tools\test-quad\qv.exe --d3d9 "<a game folder>\vulkan.dll" [--ff "<FusionFix>\shaders\GTAIV.EFLC.FusionShaders\win32_30_nv8"] [--timing]
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
$gen = Join-Path $env:TEMP ('qv-gen-' + $PID)
New-Item -ItemType Directory -Force -Path $gen | Out-Null
$own = @(
    @{ src = (Join-Path $here 'qv_pass_vs.hlsl'); t = 'vs_3_0'; var = 'kQvPassVS'; h = 'qv_pass_vs.h' },
    @{ src = (Join-Path $here 'qv_dist_ps.hlsl'); t = 'ps_3_0'; var = 'kQvDistPS'; h = 'qv_dist_ps.h' },
    @{ src = (Join-Path $here 'qv_wvp_vs.hlsl');  t = 'vs_3_0'; var = 'kQvWvpVS';  h = 'qv_wvp_vs.h' }
)
foreach ($s in $own) {
    & $fxc /nologo /T $s.t /E main /O3 /Fh (Join-Path $gen $s.h) /Vn $s.var $s.src | Out-Null
    if ($LASTEXITCODE -ne 0) { throw ('fxc failed on ' + $s.src) }
}
$cpp = Join-Path $here 'qv.cpp'; $out = Join-Path $here 'qv.exe'; $obj = Join-Path $env:TEMP ('qv-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict /I`"$gen`" `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
# the old exe goes first: a failed build must leave none, or a run chained after it prints the OLD tool's verdict
Remove-Item -LiteralPath $out -ErrorAction SilentlyContinue
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
$code = $LASTEXITCODE
Remove-Item $obj -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $gen -ErrorAction SilentlyContinue
if ($code -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'qv.exe did not build' }
Write-Host ("qv.exe: {0:N0} bytes" -f (Get-Item $out).Length) -ForegroundColor Green
