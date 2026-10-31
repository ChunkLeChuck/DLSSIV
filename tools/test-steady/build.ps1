# Builds tools\test-steady\sd.exe (x86, as the game): the steady depth's proof, the shipping shaders
# (source\shaders\ivt_steady_ps.h, ivt_steady_probe_ps.h) drawn by the shipping draw (source\transport\steady_depth_pass.h)
# on the GPU against depth pictures of known answer; sd.cpp says what it proves. The same Visual Studio lookup as
# tools\test-dedither, and the same fxc lookup as tools\compile-shaders.ps1 for the one shader that is the proof's own
# (sd_load_ps.hlsl, which loads a picture into an INTZ; compiled into %TEMP%, never into the tree). The negative
# controls are wrong DRAWS and wrong inputs made in sd.cpp itself.
#
#     tools\test-steady\build.ps1
#     tools\test-steady\sd.exe --d3d9 "<a game folder>\vulkan.dll" [--timing] [--explore]
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
$gen = Join-Path $env:TEMP ('sd-gen-' + $PID)
New-Item -ItemType Directory -Force -Path $gen | Out-Null
& $fxc /nologo /T ps_3_0 /E main /O3 /Fh (Join-Path $gen 'sd_load_ps.h') /Vn kSdLoadPS (Join-Path $here 'sd_load_ps.hlsl') | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'fxc failed on sd_load_ps.hlsl' }
$cpp = Join-Path $here 'sd.cpp'; $out = Join-Path $here 'sd.exe'; $obj = Join-Path $env:TEMP ('sd-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict /I`"$gen`" `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
# the old exe goes first: a failed build must leave none, or a run chained after it prints the OLD tool's verdict
Remove-Item -LiteralPath $out -ErrorAction SilentlyContinue
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
$code = $LASTEXITCODE
Remove-Item $obj -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $gen -ErrorAction SilentlyContinue
if ($code -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'sd.exe did not build' }
Write-Host ("sd.exe: {0:N0} bytes" -f (Get-Item $out).Length) -ForegroundColor Green
