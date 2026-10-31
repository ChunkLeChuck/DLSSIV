# Builds tools\test-corona\co.exe (x86, as the game): the corona hold's proof, the shipping shader
# (source\shaders\ivt_corona_ps.h) drawn by the shipping draw (source\transport\corona_pass.h) on the GPU, and the
# game's corona test run against it; co.cpp says what it proves. The same Visual Studio and fxc lookup as
# tools\test-steady. The tool's own shaders (the port of the game's test, the reader of its answer, and
# tools\test-steady's loader of an INTZ) are compiled into %TEMP%; nothing of them ships.
#
#     tools\test-corona\build.ps1
#     tools\test-corona\co.exe --d3d9 "<a game folder>\vulkan.dll" [--vs11 "<FusionFix>\...\deferred_lightingVS11.asm"] [--timing]
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
$gen = Join-Path $env:TEMP ('co-gen-' + $PID)
New-Item -ItemType Directory -Force -Path $gen | Out-Null
$own = @(
    @{ src = (Join-Path $here 'co_test_vs.hlsl');                 t = 'vs_3_0'; var = 'kCoTestVS'; h = 'co_test_vs.h' },
    @{ src = (Join-Path $here 'co_out_ps.hlsl');                  t = 'ps_3_0'; var = 'kCoOutPS';  h = 'co_out_ps.h' },
    @{ src = (Join-Path $here '..\test-steady\sd_load_ps.hlsl');  t = 'ps_3_0'; var = 'kSdLoadPS'; h = 'sd_load_ps.h' }
)
foreach ($s in $own) {
    & $fxc /nologo /T $s.t /E main /O3 /Fh (Join-Path $gen $s.h) /Vn $s.var $s.src | Out-Null
    if ($LASTEXITCODE -ne 0) { throw ('fxc failed on ' + $s.src) }
}
$cpp = Join-Path $here 'co.cpp'; $out = Join-Path $here 'co.exe'; $obj = Join-Path $env:TEMP ('co-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict /I`"$gen`" `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
# the old exe goes first: a failed build must leave none, or a run chained after it prints the OLD tool's verdict
Remove-Item -LiteralPath $out -ErrorAction SilentlyContinue
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
$code = $LASTEXITCODE
Remove-Item $obj -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $gen -ErrorAction SilentlyContinue
if ($code -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'co.exe did not build' }
Write-Host ("co.exe: {0:N0} bytes" -f (Get-Item $out).Length) -ForegroundColor Green
