# Builds tools\test-hdr-skip\g9.exe (x86, as the game): G9's picture proof, the encode with the per-pixel restoration skip
# against the encode before it, on the GPU (g9.cpp says what it proves). The same Visual Studio lookup as b2-shared.
#
# IT ALSO GENERATES, into %TEMP%\g9gen and never into the repository (the audit of 2026-09-28):
#   - THE ENCODE BEFORE THE SKIP, the reference every comparison is made against, plain and absorbed (Stage F, 2026-09-30:
#     B3 switch 11's committed control headers left the source with the strip). The shipping source copied with the skip's
#     condition put back to what it was, `if (gHdr.w > 0.0)` (the patch must apply exactly once), compiled as
#     compile-shaders.ps1 compiles the encode (/O3, ps_3_0), the absorbed one with HDR_CGA defined, under the control
#     headers' names and symbols. At step5-instruments both came out byte-identical to hdr_encode_noskip_ps.h and
#     hdr_encode_cga_noskip_ps.h, which had been proved equal (2026-09-28, before the Xbox mask fix and the restoration switch
#     changed the encode) to the encodes committed before the skip (70ac12fd^).
#   - THREE WRONG SKIPS, the negative controls the proof must FAIL: the source-1 knee at 0.95 instead of kHdrKnee, the LUT path
#     skipped like the exact one, and source 0's white at 1.05 instead of 1.0. Each is the shipping source copied and patched
#     in one place (the patch must apply exactly once), compiled as compile-shaders.ps1 compiles the encode (/O3, ps_3_0).
#   - THE DEBUG VIEWS' SHADER BEFORE THE SKIP, from the same patched copy (/O3, ps_3_0, kHdrDebugOldPS), so the views
#     (hdr_view 6, 14, 17 ship) are compared old against new too: they call the same HdrShip. Until 2026-09-30 this was
#     git show 70ac12fd^ (the committed header before the skip), which went stale on 2026-09-29 when the Xbox mask fix
#     (322faa56) and the restoration's switch (b4c66b07) changed the views under the HUD mask and on source 1: at
#     step5-instruments seven view checks FAILED for that reason, not the skip's. The patch applied to 70ac12fd's own source
#     reproduces 70ac12fd^'s debug header (and both encode headers) byte for byte, so the reference means what it meant.
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$src = Split-Path -Parent (Split-Path -Parent $here)                          # src\DLSS-IV
$sh = Join-Path $src 'source\shaders'
$gen = Join-Path $env:TEMP 'g9gen'
if (Test-Path $gen) { Remove-Item -Recurse -Force $gen }
New-Item -ItemType Directory -Force $gen | Out-Null

$fxc = $env:FXC_EXE
if (-not $fxc) {
    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $found = Get-ChildItem -Path $kits -Filter 'fxc.exe' -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\x64\\' } | Sort-Object FullName -Descending | Select-Object -First 1
    if ($found) { $fxc = $found.FullName }
}
if (-not $fxc -or -not (Test-Path $fxc)) { throw 'fxc.exe not found (set FXC_EXE)' }

$noskip = Join-Path $gen 'noskip'; New-Item -ItemType Directory -Force $noskip | Out-Null
foreach ($f in 'hdr_encode_ps.hlsl', 'hdr_debug_ps.hlsl', 'hdr_common.hlsli', 'hdr_cga.hlsli') { Copy-Item (Join-Path $sh $f) (Join-Path $noskip $f) }
$p = Join-Path $noskip 'hdr_common.hlsli'; $t = [IO.File]::ReadAllText($p)
$skipRx = [regex]::Escape('[branch] if (gHdr.w > 0.0 && ((gHdr3.w < 0.5) ? (max(s.r, max(s.g, s.b)) > 1.0)') + '\r?\n[ \t]*' +
          [regex]::Escape(': (HdrLutPath(gHdr3, g3, flags) || max(lin.r, max(lin.g, lin.b)) > gHdr2.x)))')
$n = ([regex]::Matches($t, $skipRx)).Count
if ($n -ne 1) { throw "the encode before the skip: its patch matched $n times, not once (hdr_common.hlsli moved on)" }
[IO.File]::WriteAllText($p, [regex]::Replace($t, $skipRx, 'if (gHdr.w > 0.0)'))
foreach ($o in @(@{ h = 'hdr_encode_noskip_ps.h'; var = 'kHdrEncodeNoSkipPS'; defs = @() }, @{ h = 'hdr_encode_cga_noskip_ps.h'; var = 'kHdrEncodeCgaNoSkipPS'; defs = @('/D', 'HDR_CGA=1') })) {
    $defs = $o.defs
    & $fxc /nologo /T ps_3_0 /E main /O3 @defs /Fh (Join-Path $gen $o.h) /Vn $o.var (Join-Path $noskip 'hdr_encode_ps.hlsl') | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "fxc failed on the encode before the skip ($($o.h))" }
}
& $fxc /nologo /T ps_3_0 /E main /O3 /Fh (Join-Path $gen 'hdr_debug_old_ps.h') /Vn kHdrDebugOldPS (Join-Path $noskip 'hdr_debug_ps.hlsl') | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'fxc failed on the debug views before the skip' }

$cond = 'HdrLutPath(gHdr3, g3, flags) || max(lin.r, max(lin.g, lin.b)) > gHdr2.x'
$wrong = @(
    @{ name = 'knee';  var = 'kG9WrongKneePS';  old = 'max(lin.r, max(lin.g, lin.b)) > gHdr2.x'; new = 'max(lin.r, max(lin.g, lin.b)) > 0.95' },
    @{ name = 'lut';   var = 'kG9WrongLutPS';   old = $cond; new = 'max(lin.r, max(lin.g, lin.b)) > gHdr2.x' },
    @{ name = 'white'; var = 'kG9WrongWhitePS'; old = '(max(s.r, max(s.g, s.b)) > 1.0)'; new = '(max(s.r, max(s.g, s.b)) > 1.05)' }
)
foreach ($w in $wrong) {
    $d = Join-Path $gen $w.name; New-Item -ItemType Directory -Force $d | Out-Null
    foreach ($f in 'hdr_encode_ps.hlsl', 'hdr_common.hlsli', 'hdr_cga.hlsli') { Copy-Item (Join-Path $sh $f) (Join-Path $d $f) }
    $p = Join-Path $d 'hdr_common.hlsli'; $t = [IO.File]::ReadAllText($p)
    $n = ([regex]::Matches($t, [regex]::Escape($w.old))).Count
    if ($n -ne 1) { throw "the wrong skip '$($w.name)': its patch matched $n times, not once (hdr_common.hlsli moved on)" }
    [IO.File]::WriteAllText($p, $t.Replace($w.old, $w.new))
    & $fxc /nologo /T ps_3_0 /E main /O3 /Fh (Join-Path $gen ("g9_wrong_" + $w.name + '.h')) /Vn $w.var (Join-Path $d 'hdr_encode_ps.hlsl') | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "fxc failed on the wrong skip '$($w.name)'" }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }
$cpp = Join-Path $here 'g9.cpp'; $out = Join-Path $here 'g9.exe'; $obj = Join-Path $env:TEMP ('g9-' + $PID + '.obj')
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /std:c++17 /fp:strict /I`"$gen`" `"$cpp`" /Fe:`"$out`" /Fo:`"$obj`" /link /INCREMENTAL:NO user32.lib"
$output = & cmd.exe /c $cmd 2>&1
$output | Where-Object { $_ -match 'warning|error' } | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $out)) { $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }; throw 'g9.exe did not build' }
Remove-Item $obj -ErrorAction SilentlyContinue
Write-Host ("g9.exe: {0:N0} bytes (the encode before the skip, three wrong skips and the old debug shader generated in {1})" -f (Get-Item $out).Length, $gen) -ForegroundColor Green
