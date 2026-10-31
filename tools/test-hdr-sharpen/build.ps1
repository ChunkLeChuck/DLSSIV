# Builds tools\test-hdr-sharpen\g10.exe (x86, as the game): G10's picture proof, the HDR encode that sharpens at its own
# sample against the pipeline it replaces (the sharpen pass, then the encode), on the GPU; g10.cpp says what it proves.
# The same Visual Studio and fxc lookup as tools\test-hdr-skip.
#
# IT ALSO GENERATES, into %TEMP%\g10gen and never into the repository, FIVE WRONG G10 ENCODES, the negative controls the
# proof must FAIL, each the shipping source copied and patched in one place (the patch must apply exactly once) and compiled
# as compile-shaders.ps1 compiles the G10 headers (/O3, ps_3_0, HDR_G10 defined):
#   mask     the HUD mask compares the SHARPENED sample (the fault the design exists to avoid)
#   maskcga  the same in the absorbed encode
#   ceiling  the ceiling always 1, as if values above white never reached the display
#   lanes    the attenuation read from the ceiling's lane
#   order    the absorbed encode putting FusionFix's curve on the centre BEFORE sharpening (review of 6fb08430: a control
#            that the absorbed rule, which is looser, can fail)
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$src = Split-Path -Parent (Split-Path -Parent $here)                          # src\DLSS-IV
$sh = Join-Path $src 'source\shaders'
$gen = Join-Path $env:TEMP 'g10gen'
if (Test-Path $gen) { Remove-Item -Recurse -Force $gen }
New-Item -ItemType Directory -Force $gen | Out-Null

$fxc = $env:FXC_EXE
if (-not $fxc) {
    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $found = Get-ChildItem -Path $kits -Filter 'fxc.exe' -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\x64\\' } | Sort-Object FullName -Descending | Select-Object -First 1
    if ($found) { $fxc = $found.FullName }
}
if (-not $fxc -or -not (Test-Path $fxc)) { throw 'fxc.exe not found (set FXC_EXE)' }

$wrong = @(
    @{ name = 'mask';    var = 'kG10WrongMaskPS';    cga = $false; old = '#define HDR_MASK_S sM'; new = '#define HDR_MASK_S s' },
    @{ name = 'maskcga'; var = 'kG10WrongMaskCgaPS'; cga = $true;  old = '#define HDR_MASK_S sM'; new = '#define HDR_MASK_S s' },
    @{ name = 'ceiling'; var = 'kG10WrongCeilingPS'; cga = $false; old = 'const float  top  = gHdr3.y;'; new = 'const float  top  = 1.0;' },
    @{ name = 'lanes';   var = 'kG10WrongLanesPS';   cga = $false; old = 'const float  att  = gHdr3.z;'; new = 'const float  att  = gHdr3.y;' },
    @{ name = 'order';   var = 'kG10WrongOrderPS';   cga = $true;  old = 'CgaCurve(HdrG10Rcas(e0, uv), uv, gCga)'; new = 'HdrG10Rcas(CgaCurve(e0, uv, gCga), uv)' }
)
foreach ($w in $wrong) {
    $d = Join-Path $gen $w.name; New-Item -ItemType Directory -Force $d | Out-Null
    foreach ($f in 'hdr_encode_ps.hlsl', 'hdr_common.hlsli', 'hdr_cga.hlsli') { Copy-Item (Join-Path $sh $f) (Join-Path $d $f) }
    $p = Join-Path $d 'hdr_encode_ps.hlsl'; $t = [IO.File]::ReadAllText($p)
    $n = ([regex]::Matches($t, [regex]::Escape($w.old))).Count
    if ($n -ne 1) { throw "the wrong encode '$($w.name)': its patch matched $n times, not once (hdr_encode_ps.hlsl moved on)" }
    [IO.File]::WriteAllText($p, $t.Replace($w.old, $w.new))
    $defs = @('/D', 'HDR_G10=1'); if ($w.cga) { $defs += @('/D', 'HDR_CGA=1') }
    & $fxc /nologo /T ps_3_0 /E main /O3 @defs /Fh (Join-Path $gen ("g10_wrong_" + $w.name + '.h')) /Vn $w.var $p | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "fxc failed on the wrong encode '$($w.name)'" }
}

# AND THE SHIPPING ENCODE AS IT WAS BEFORE THE XBOX CURVE'S FIX (2026-09-29), plain and absorbed, the control of check 5: the
# mask's Xbox 360 curve above white put back to the straight line at the table's top slope, in hdr_common.hlsli.
# (Check 4's absorbed mask view, built the same way from hdr_debug_ps.hlsl, went with the view at Stage F, 2026-09-30.)
foreach ($o in @(@{ name = 'oldline'; var = 'kOldLinePS'; cga = $false }, @{ name = 'oldlinecga'; var = 'kOldLineCgaPS'; cga = $true })) {
    $d = Join-Path $gen $o.name; New-Item -ItemType Directory -Force $d | Out-Null
    foreach ($f in 'hdr_encode_ps.hlsl', 'hdr_common.hlsli', 'hdr_cga.hlsli') { Copy-Item (Join-Path $sh $f) (Join-Path $d $f) }
    $p = Join-Path $d 'hdr_common.hlsli'; $t = [IO.File]::ReadAllText($p)
    $old = ': hi[c];'; $n = ([regex]::Matches($t, [regex]::Escape($old))).Count
    if ($n -ne 1) { throw "the old-line encode: its patch matched $n times, not once (hdr_common.hlsli moved on)" }
    [IO.File]::WriteAllText($p, $t.Replace($old, ': 1.0 + flags.y * (v - 1.0);'))
    $defs = @(); if ($o.cga) { $defs += @('/D', 'HDR_CGA=1') }
    & $fxc /nologo /T ps_3_0 /E main /O3 @defs /Fh (Join-Path $gen ('g10_' + $o.name + '.h')) /Vn $o.var (Join-Path $d 'hdr_encode_ps.hlsl') | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "fxc failed on the old-line encode '$($o.name)'" }
}

# AND THE SHIPPING ENCODE AS IT WAS BEFORE THE RESTORATION'S SWITCH (2026-09-29 evening), plain and absorbed, the control of
# checks 5 (b) and 6 (Stage F, 2026-09-30: B3 switch 14's committed control headers left the source with the strip): the
# exact ratio's unclipped value put back through ConsoleGamma, the line, in HdrExactRatio (the patch must apply exactly once),
# the mask already on FusionFix's formula. Under the control headers' names and symbols; at step5-instruments both came out
# byte-identical to hdr_encode_rline_ps.h and hdr_encode_cga_rline_ps.h (567f1bba's shipping encode byte for byte).
$d = Join-Path $gen 'rline'; New-Item -ItemType Directory -Force $d | Out-Null
foreach ($f in 'hdr_encode_ps.hlsl', 'hdr_common.hlsli', 'hdr_cga.hlsli') { Copy-Item (Join-Path $sh $f) (Join-Path $d $f) }
$p = Join-Path $d 'hdr_common.hlsli'; $t = [IO.File]::ReadAllText($p)
$old = 'const float3 full = ConsoleGammaExact(g, xg, flags);'; $n = ([regex]::Matches($t, [regex]::Escape($old))).Count
if ($n -ne 1) { throw "the encode before the restoration's switch: its patch matched $n times, not once (hdr_common.hlsli moved on)" }
[IO.File]::WriteAllText($p, $t.Replace($old, 'const float3 full = ConsoleGamma(g, xg, flags);'))
foreach ($o in @(@{ h = 'hdr_encode_rline_ps.h'; var = 'kHdrEncodeRLinePS'; defs = @() }, @{ h = 'hdr_encode_cga_rline_ps.h'; var = 'kHdrEncodeCgaRLinePS'; defs = @('/D', 'HDR_CGA=1') })) {
    $defs = $o.defs
    & $fxc /nologo /T ps_3_0 /E main /O3 @defs /Fh (Join-Path $gen $o.h) /Vn $o.var (Join-Path $d 'hdr_encode_ps.hlsl') | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "fxc failed on the encode before the restoration's switch ($($o.h))" }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }
$cpp = Join-Path $here 'g10.cpp'; $out = Join-Path $here 'g10.exe'; $obj = Join-Path $env:TEMP ('g10-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict /I`"$gen`" `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'g10.exe did not build' }
Remove-Item $obj -ErrorAction SilentlyContinue
Write-Host ("g10.exe: {0:N0} bytes (five wrong G10 encodes, the old-line encodes and the encodes before the restoration's switch generated in {1})" -f (Get-Item $out).Length, $gen) -ForegroundColor Green
