<#
.SYNOPSIS
    Compiles EVERY shader to its checked-in header under source\shaders, or
    with -Verify, checks that every checked-in header is what its .hlsl
    currently compiles to.

.DESCRIPTION
    build.ps1 has no shader step: the .h files are fxc /Fh output committed to
    the repo, and the binary uses the header, not the .hlsl. So a .hlsl edited
    without a recompile ships the OLD shader while the source says otherwise,
    silently, and nothing in the build says a word.

    THAT HAPPENED ON 2026-09-09. ivt_debug_ps.hlsl gained a guard clause, the
    header was not regenerated, and the shipped bytecode was missing it. It was
    caught by a verifier, not by the toolchain. The cause was this script:
    it covered the five HDR shaders and nothing else, so ten of the fifteen had
    no regeneration path at all.

    It now covers every shader (the list below decides how many; counts quoted here went stale three times, so none is
    given), and -Verify makes a stale header an error rather than something you have to think to check.

        .\tools\compile-shaders.ps1            regenerate every header
        .\tools\compile-shaders.ps1 -Verify    recompile to a temp dir and
                                               compare; exit 1 on any mismatch

    THE FLAGS ARE PER SHADER AND THEY ARE NOT A STYLE CHOICE. Each entry below
    carries the flag set that reproduces its committed header byte for byte,
    determined by compiling each file at every optimisation level and comparing
    (2026-09-09). The HDR shaders and rcas are /O3; the rest carry no
    optimisation flag. Change one and the header changes, so -Verify would then
    report every other file as stale: if you mean to change a flag, change it
    here and regenerate that shader in the same commit.
#>
param([switch]$Verify)

$ErrorActionPreference = 'Stop'
# fxc, FOUND RATHER THAN PINNED. This was a hardcoded SDK build number, which
# breaks on any other machine and on this one after an SDK update, with an error
# about a missing file rather than about the SDK. Newest first, so a machine with
# several SDKs uses the current one. Audit L43's class of fault.
$fxc = $env:FXC_EXE
if (-not $fxc) {
    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    if (Test-Path $kits) {
        $found = Get-ChildItem -Path $kits -Filter 'fxc.exe' -Recurse -ErrorAction SilentlyContinue |
                 Where-Object { $_.FullName -match '\\x64\\' } |
                 Sort-Object FullName -Descending |
                 Select-Object -First 1
        if ($found) { $fxc = $found.FullName }
    }
}
if (-not $fxc) { throw "fxc.exe not found under the Windows 10 SDK. Set FXC_EXE to its full path." }
if (-not (Test-Path $fxc)) { throw "fxc not found at $fxc" }
$dir = Join-Path $PSScriptRoot '..\source\shaders'

# src (without extension), target, /Vn symbol, and whether the header was built
# with /O3. The header name is always <src>.h.
#
# THE /O3 SPLIT IS ARCHAEOLOGY, NOT A TUNED DECISION (audit F134). It records
# which flag each committed header was actually produced with, so that -Verify
# can reproduce it byte for byte; it does not mean the /O3 ones were
# measured to want /O3 and the others measured not to.
#
# MOVING THE REST TO /O3 IS NOT FREE, which is the answer to "make it more
# optimised without regression". /O3 lets fxc reassociate floating-point
# arithmetic, and the picture shaders write oDepth against a 1e-5 bias; changing
# the order of those operations changes the depth they emit. So the change would
# have to be proved on the picture rather than assumed, and nothing measured says
# there is anything to win. Leave the split alone unless a measurement asks for it.
$list = @(
    @{ src = 'hdr_encode_ps';         t = 'ps_3_0'; var = 'kHdrEncodePS';        o3 = $true  },
    @{ src = 'hdr_debug_ps';          t = 'ps_3_0'; var = 'kHdrDebugPS';         o3 = $true  },
    @{ src = 'hdr_lutprofile_ps';     t = 'ps_3_0'; var = 'kHdrLutProfilePS';    o3 = $true  },
    @{ src = 'hdr_cgprobe_ps';        t = 'ps_3_0'; var = 'kHdrCgProbePS';       o3 = $true  },
    @{ src = 'hdr_encode_cga_ps';     t = 'ps_3_0'; var = 'kHdrEncodeCgaPS';     o3 = $true  },
    @{ src = 'hdr_encode_g10_ps';     t = 'ps_3_0'; var = 'kHdrEncodeG10PS';     o3 = $true  },   # G10 (shipped 2026-09-30): both builds
    @{ src = 'hdr_encode_cga_g10_ps'; t = 'ps_3_0'; var = 'kHdrEncodeCgaG10PS';  o3 = $true  },   # the same, absorbed
    @{ src = 'hdr_cgaproof_ps';       t = 'ps_3_0'; var = 'kHdrCgaProofPS';      o3 = $true  },
    @{ src = 'hdr_cgablit_ps';        t = 'ps_3_0'; var = 'kHdrCgaBlitPS';       o3 = $true  },
    @{ src = 'rcas_ps';               t = 'ps_3_0'; var = 'kRcasPS';             o3 = $true  },
    @{ src = 'ivt_guides_ps';         t = 'ps_3_0'; var = 'kIvtGuidesPS';        o3 = $false },
    @{ src = 'ivt_dedither_ps';       t = 'ps_3_0'; var = 'kIvtDeditherPS';      o3 = $true  },   # the game's dither smoothed before the upscaler (2026-10-03)
    @{ src = 'ivt_dedither_home_ps';  t = 'ps_3_0'; var = 'kIvtDeditherHomePS';  o3 = $true  },   # the pixels the pass changed, written into the scene buffer itself
    @{ src = 'ivt_dedither_count_ps'; t = 'ps_3_0'; var = 'kIvtDeditherCountPS'; o3 = $true  },   # is anything dithered at all (one texel in sixteen, inside an occlusion query): the pass is left out while the answer is no
    @{ src = 'ivt_dedither_view_ps';  t = 'ps_3_0'; var = 'kIvtDeditherViewPS';  o3 = $true  },   # its view (what the pass decides, painted): development builds only
    @{ src = 'ivt_dedither_probe_ps'; t = 'ps_3_0'; var = 'kIvtDeditherProbePS'; o3 = $true  },   # its counter (drawn inside an occlusion query): development builds only
    @{ src = 'ivt_steady_ps';         t = 'ps_3_0'; var = 'kIvtSteadyPS';        o3 = $true  },   # the depth the post chain reads, resolved over time (steady_depth, 2026-10-03)
    @{ src = 'ivt_steady_home_ps';    t = 'ps_3_0'; var = 'kIvtSteadyHomePS';    o3 = $true  },   # what the resolve holds that differs, written back into the texture the post chain reads (a colour copy or the scene's depth)
    @{ src = 'ivt_rain_ps';           t = 'ps_3_0'; var = 'kIvtRainPS';           o3 = $true  },   # the rain, drawn into a layer of our own, added to the picture the upscaler made (rain_layer)
    @{ src = 'ivt_corona_ps';         t = 'ps_3_0'; var = 'kIvtCoronaPS';         o3 = $true  },   # the depth the game's corona test reads, held while the camera stands still (corona_hold)
    @{ src = 'ivt_mask_ps';             t = 'ps_3_0'; var = 'kIvtMaskPS';           o3 = $true  },   # the mask for transparent things: the finished scene against the scene right after the fog quad (roadmap item 5; not in any binary yet)
    @{ src = 'ivt_notice_ps';           t = 'ps_3_0'; var = 'kIvtNoticePS';         o3 = $true  },   # the on-screen notice: a small picture of text laid over the finished frame (transport/notice_pass.h)
    @{ src = 'ivt_steady_probe_ps';   t = 'ps_3_0'; var = 'kIvtSteadyProbePS';   o3 = $true  },   # its counter (drawn inside an occlusion query): development builds only
    @{ src = 'ivt_fs_vs';             t = 'vs_3_0'; var = 'kIvtFsVS';            o3 = $false },
    @{ src = 'velocity_ps';           t = 'ps_3_0'; var = 'kVelocityPS';         o3 = $false },
    @{ src = 'velocity_vs';           t = 'vs_3_0'; var = 'kVelocityVS';         o3 = $false },
    @{ src = 'velocity_skin_vs';      t = 'vs_3_0'; var = 'kVelocitySkinVS';     o3 = $false },
    @{ src = 'velocity_skin_prev_vs'; t = 'vs_3_0'; var = 'kVelocitySkinPrevVS'; o3 = $false },
    @{ src = 'velocity_dmg_vs';            t = 'vs_3_0'; var = 'kVelocityDmgVS';          o3 = $false },   # the three above with the game's vehicle damage first (velocity_dmg.hlsli; 2026-10-26)
    @{ src = 'velocity_skin_dmg_vs';       t = 'vs_3_0'; var = 'kVelocitySkinDmgVS';      o3 = $false },
    @{ src = 'velocity_skin_prev_dmg_vs';  t = 'vs_3_0'; var = 'kVelocitySkinPrevDmgVS';  o3 = $false },
    @{ src = 'vtf_test_ps';           t = 'ps_3_0'; var = 'kVtfTestPS';          o3 = $false },
    @{ src = 'vtf_test_vs';           t = 'vs_3_0'; var = 'kVtfTestVS';          o3 = $false }
)

# Every .hlsl in the folder must be in the list. A shader added without an entry
# is exactly the hole this script was rewritten to close, so say so loudly
# rather than quietly not building it.
$known = $list | ForEach-Object { $_.src }
$missing = Get-ChildItem "$dir\*.hlsl" | Where-Object { $known -notcontains $_.BaseName } | ForEach-Object { $_.BaseName }
if ($missing) { throw ("these shaders have no entry in compile-shaders.ps1: " + ($missing -join ', ')) }

$tmp = $null
if ($Verify) {
    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("shaderverify-" + [System.Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $tmp | Out-Null
}

$bad = 0
foreach ($s in $list) {
    $in  = Join-Path $dir ($s.src + '.hlsl')
    $hdr = Join-Path $dir ($s.src + '.h')
    $out = if ($Verify) { Join-Path $tmp ($s.src + '.h') } else { $hdr }

    $a = @('/nologo', '/T', $s.t, '/E', 'main')
    if ($s.o3) { $a += '/O3' }
    $a += @('/Fh', $out, '/Vn', $s.var, $in)
    & $fxc @a
    if ($LASTEXITCODE -ne 0) { throw "fxc failed on $($s.src).hlsl" }

    $line = (Select-String -Path $out -Pattern 'approximately' | Select-Object -First 1)
    $note = if ($line) { $line.Line.Trim() } else { '' }

    if ($Verify) {
        if (-not (Test-Path $hdr)) { Write-Host ("  {0,-22} MISSING HEADER" -f $s.src); $bad++; continue }
        $a1 = (Get-FileHash $hdr -Algorithm SHA256).Hash
        $b1 = (Get-FileHash $out -Algorithm SHA256).Hash
        if ($a1 -eq $b1) { Write-Host ("  {0,-22} ok    {1}" -f $s.src, $note) }
        else {
            Write-Host ("  {0,-22} STALE: the committed header is not what this .hlsl compiles to" -f $s.src)
            Write-Host ("  {0,-22}        committed {1}" -f '', $a1.Substring(0,16))
            Write-Host ("  {0,-22}        compiled  {1}" -f '', $b1.Substring(0,16))
            $bad++
        }
    }
    else { Write-Host ("  {0,-22} {1}" -f $s.src, $note) }
}

if ($Verify) {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    if ($bad) {
        Write-Host ""
        Write-Host "$bad shader header(s) are stale. Run this script with no arguments to regenerate them,"
        Write-Host "and commit the .h alongside the .hlsl: the binary uses the header, not the source."
        exit 1
    }
    Write-Host ""
    Write-Host "all $($list.Count) shader headers match their source"
}
