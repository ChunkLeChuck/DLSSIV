<#
.SYNOPSIS
    Builds DLSS-IV.asi (x86) and optionally deploys it to a game folder.

.DESCRIPTION
    GTA IV is a 32-bit process, so this MUST build x86. The script verifies the
    PE machine type after linking rather than trusting the flags: a 64-bit .asi
    loads in nothing and the only symptom is silence, which looks exactly like
    the mod not working.

    Deploying is opt-in. This .asi is a standalone thing that may go out to
    other people, and the folder it belongs in depends on which install is
    being tested.

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Deploy GTAIV1080
    .\build.ps1 -Analyze
#>
[CmdletBinding()]
param(
    [string]$Deploy,
    # Run MSVC's static analyser instead of producing a binary, and compare the
    # warning count against the baseline recorded in
    # docs/reports/static-analysis-2026-09-09.md. Not part of a normal build:
    # /analyze roughly triples the compile and the shipped binary must never
    # depend on it. See the -Analyze block near the bottom of this file.
    [switch]$Analyze,
    # A RELEASE BUILD: DLSSIV_DEV=0, which takes every development fence in
    # source/DLSS-IV.cpp's STRIPPABLE SWITCHES table with it (the log=2 and log=3
    # call sites first among them). Without it the build is the development one,
    # which is what master builds while field runs are still owed. The define is
    # #ifndef-guarded in the source, so this is the only thing that has to change.
    [switch]$Release,
    # (THE THIRD BUILD, -Fg with -Release, the release configuration with frame generation's product code unarmed (RF), was
    # retired in round 8's deep strip, 2026-10-26: no player gets that shape, and -Ship is the release that carries frame
    # generation. Its helper half lives on as build-host.ps1 -Release -Fg, which -Ship builds.)
    # (THE ARMED DEVELOPMENT BUILD, -Arms, the fourth build of 2026-10-10, was retired in round 9's strip, 2026-10-26: the source's
    # switch DLSSIV_FG_ARMS was 1 in every build, so the development build is the armed one; round 9's sweep retired the switch,
    # and the source refuses a build that names it.)
    # THE SHAPE 1.1 SHIPS, BOTH BINARIES (2026-10-25, round 8's first prerequisite; docs\STRIP-INVENTORY-FG.md, its shape RA): the
    # armed release. DLSS-IV.asi with /DDLSSIV_DEV=0 /DDLSSIV_FG=1, the switches tools\test-fg-arms compiles its
    # "armed-ship" shape with: the release configuration (no development fence, no rig, no instrument, DLSSIV_DEVLOG 0, so
    # FGP_FORCED and FGP_FILL are 0 and the fill and the forced driver are out by the preprocessor), frame generation's product
    # (DLSSIV_FG 1) over the binding that arms (FgPA): the law, the fixed mode, the doubling, the VRR read, the Wine fence path.
    # THEN THE HELPER beside it as build-host.ps1 -Release -Fg (the helper has no ARMS switch: its shipped shape is the release
    # with frame generation's product), with the same -IdentityBuildId if one was given. Both into the tree, as -Release does.
    # The source's switch stays 0: this is the target the 1.1 package and tools\prove-release-identity.py --ship build; making
    # it package.ps1's default is round 8's decision. Its flavour word is "release", as -Release -Fg's and -Release's (the source decides the
    # word by DLSSIV_DEV and DLSSIV_FG alone): what tells the two apart is the map, FgMachine over FgPA and not over FgP.
    [switch]$Ship,
    # THE IDENTITY PROOF'S BUILD ID (2026-09-28, frame generation phase 1C, S6's gate G-C0a; a parameter since the gate's review,
    # never an environment variable, which a shell could leave set): tools\prove-release-identity.py builds the candidate with
    # the BASE's id so the two releases compare byte for byte. Nothing else passes it; a build that takes it says so.
    [string]$IdentityBuildId = '',
    # THE M4D VERIFY ARM, COMPILED AND NOTHING WRITTEN (the final audit's cfgAndList-compile-switches, finished 2026-10-01 after
    # the review of 9123f945): the development configuration with /DDLSSIV_M4D_INDEX_VERIFY=1, compile only, into %TEMP%, so an
    # arm that ships at 0 cannot rot unseen. prove-all runs it. No DLSS-IV.asi is touched.
    [switch]$VerifyArm
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# WHERE VISUAL STUDIO IS, resolved at run time (audit F152). This was the
# Community edition's path, hardcoded, which is a third of audit L40 that was
# never applied: a machine with Professional, Enterprise, Build Tools, or 2022
# installed anywhere but the default drive could not build this at all, and the
# only symptom was a refusal naming a path the developer does not have.
#
# Three steps, in order, each a deliberate fallback rather than a guess:
#   DLSSIV_VSROOT   an explicit override, which is what a machine with several
#                   toolchains needs and what CI would set.
#   vswhere         Microsoft's own locator, asking specifically for the x86/x64
#                   C++ tools rather than for any Visual Studio: an install with
#                   only the .NET workload has no vcvars and answering with it
#                   would trade a clear refusal for a confusing one.
#   the old path    last, so nothing changes on the machine this was written on.
#
# The vcvars refusal below is unchanged and is still what catches a wrong answer.
$vsRoot = $env:DLSSIV_VSROOT
if (-not $vsRoot) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * `
                            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
                            -property installationPath
        if ($LASTEXITCODE -eq 0 -and $found) { $vsRoot = ($found | Select-Object -First 1).Trim() }
    }
}
if (-not $vsRoot) { $vsRoot = 'C:\Program Files\Microsoft Visual Studio\2022\Community' }
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars32.bat not found at: $vcvars" }

$out = Join-Path $root 'DLSS-IV.asi'
$src = Join-Path $root 'source\DLSS-IV.cpp'
$obj = Join-Path $root 'source\DLSS-IV.obj'
$rc  = Join-Path $root 'source\DLSS-IV.rc'     # the version resource (2026-09-21); its number is source\version.h's
$res = Join-Path $root 'source\DLSS-IV.res'

# A SHADER EDITED WITHOUT A RECOMPILE SHIPS THE OLD SHADER, SILENTLY.
# The .h files under source\shaders are committed fxc output and are what this
# binary includes; the .hlsl beside each one is only what people read. Nothing
# here compiles them, so an edit to a .hlsl changes nothing about the build and
# the build says nothing about it. That happened on 2026-09-09: a guard clause
# was added to ivt_debug_ps.hlsl, the header was not regenerated, and the
# shipped bytecode was missing it. A verifier found it, not the toolchain.
#
# A timestamp compare costs nothing and catches exactly that case. It is not a
# proof (git checkout gives every file the same time, so a fresh clone is equal
# rather than newer, which passes); the proof is
# tools\compile-shaders.ps1 -Verify, which recompiles every one and compares.
# .hlsli COUNTS TOO, and missing that would have been the same bug by another
# road. hdr_common.hlsli has no header of its own and is included by most HDR shaders, directly or through
# hdr_encode_ps.hlsl, so editing it alone leaves every .hlsl older than its .h and a
# .hlsl-only comparison passes with stale headers. So the newest .hlsli in
# the folder is treated as an input to every header.
#
# AND ONE .hlsl IS AN INPUT TO ANOTHER'S HEADER (audit 2026-09-18, which found
# this gate blind to it). hdr_encode_cga_ps.hlsl is two lines: it defines
# HDR_CGA and includes hdr_encode_ps.hlsl, so the absorb's encode is the
# shipping encode's source compiled a second way. Editing hdr_encode_ps.hlsl
# therefore leaves hdr_encode_cga_ps.h stale while its own .hlsl, which nobody
# had to touch, is still older than it: the exact hole this gate exists to
# close, reopened by a file added on 2026-09-18. The map below is the general
# fix, and any future .hlsl that includes another belongs in it.
$shaderDir = Join-Path $root 'source\shaders'
$newestInc = Get-ChildItem "$shaderDir\*.hlsli" -ErrorAction SilentlyContinue |
             Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
$extraSrc = @{ 'hdr_encode_cga_ps' = @('hdr_encode_ps.hlsl');
               'hdr_encode_g10_ps' = @('hdr_encode_ps.hlsl'); 'hdr_encode_cga_g10_ps' = @('hdr_encode_ps.hlsl');
               'velocity_dmg_vs' = @('velocity_vs.hlsl'); 'velocity_skin_dmg_vs' = @('velocity_skin_vs.hlsl')   # the dented vehicle's variants (2026-10-26)
               'velocity_skin_prev_dmg_vs' = @('velocity_skin_prev_vs.hlsl') }
$stale = Get-ChildItem "$shaderDir\*.hlsl" -ErrorAction SilentlyContinue | Where-Object {
    $h = Join-Path $shaderDir ($_.BaseName + '.h')
    if (-not (Test-Path $h)) { return $false }
    $ht = (Get-Item $h).LastWriteTimeUtc
    $old = ($_.LastWriteTimeUtc -gt $ht) -or ($newestInc -and $newestInc.LastWriteTimeUtc -gt $ht)
    if (-not $old -and $extraSrc.ContainsKey($_.BaseName)) {
        foreach ($dep in $extraSrc[$_.BaseName]) {
            $d = Join-Path $shaderDir $dep
            if ((Test-Path $d) -and ((Get-Item $d).LastWriteTimeUtc -gt $ht)) { $old = $true }
        }
    }
    $old
}
if ($stale) {
    Write-Host ""
    Write-Host "STOPPING: these shader sources are newer than their committed headers, so this build" -ForegroundColor Yellow
    Write-Host "would ship the OLD bytecode for them:" -ForegroundColor Yellow
    $stale | ForEach-Object { Write-Host ("    " + $_.Name) -ForegroundColor Yellow }
    Write-Host ""
    Write-Host "  .\tools\compile-shaders.ps1            regenerates every header"
    Write-Host "  .\tools\compile-shaders.ps1 -Verify    checks every header without writing"
    throw "shader headers are out of date"
}

# THE CFG VERIFIER, WIRED IN (2026-09-10). tools/check-cfg.py checks three
# things a compiler cannot: that the positional IvtCfg initialiser still lines
# up with the struct field for field, that every int field is in exactly one of
# the two live/static key lists, and that every key the parser accepts is
# documented in CONFIG.md and vice versa. A field in neither key list is not
# "static", it is INVISIBLE to the live tick.
#
# IT WAS WIRED TO NOTHING UNTIL NOW, while a source comment claimed it was
# enforced. That is the same shape as audit F161, a complete verifier that no
# build ran, and it is worth more here than in package.ps1: a key list drifts
# while someone is editing, not when they cut a release.
#
# A MISSING PYTHON IS NOT A BUILD FAILURE. Someone building from a source zip
# need not have it; the check says it was skipped rather than failing them.
$py = (Get-Command python -ErrorAction SilentlyContinue)
if (-not $py) { $py = (Get-Command py -ErrorAction SilentlyContinue) }
if ($py) {
    $cfgOut = & $py.Source (Join-Path $root 'tools\check-cfg.py') 2>&1
    if ($LASTEXITCODE -ne 0) {
        $cfgOut | ForEach-Object { Write-Host $_ -ForegroundColor Yellow }
        throw "check-cfg.py failed: the cfg struct, its key lists and CONFIG.md disagree"
    }
} else {
    Write-Host "  check-cfg.py SKIPPED: no python on PATH" -ForegroundColor DarkYellow
}

# THE PRESENT MUST APPEAR EXACTLY ONCE, and this check outlived the script it
# came from (audit R4). `tools\strip_probes.py` was deleted with the other
# 2026-09-08 instruments; of its assertions this is the only one worth keeping,
# because it is the only one the compiler cannot make for itself. Unbalanced
# braces, which that script also checked, produce a compiler error with a line
# number one step later, so re-checking them here would be strictly worse than
# what already happens. A LOST OR DUPLICATED PRESENT COMPILES PERFECTLY and
# breaks the game, which is the same class of fault as a stale shader header and
# is why the two checks stand together.
#
# String and comment text is stripped first so that a mention of the call in
# prose cannot satisfy or trip the count.
$cppPath = Join-Path $root 'source\DLSS-IV.cpp'
$cpp     = [System.IO.File]::ReadAllText($cppPath)
$cpp     = [regex]::Replace($cpp, '//[^\r\n]*', '')
$cpp     = [regex]::Replace($cpp, '"(\\.|[^"\\])*"', '""')
$presents = ([regex]::Matches($cpp, [regex]::Escape('origSwapPresent(sc, src, dst, wnd, dirty, flags);'))).Count
if ($presents -ne 1) {
    Write-Host ""
    Write-Host "STOPPING: the real Present call appears $presents time(s) in DLSS-IV.cpp, expected exactly 1." -ForegroundColor Yellow
    Write-Host "A lost present is a black screen; a duplicated one presents twice a frame. Neither is a" -ForegroundColor Yellow
    Write-Host "compiler error, which is why this is checked here." -ForegroundColor Yellow
    throw "origSwapPresent is not called exactly once"
}

# /MT so the .asi carries its own CRT: it goes to other people's installs and
# cannot assume a redistributable is present.
# /GS- drops the per-function stack-buffer security check. THE REASON GIVEN
# HERE UNTIL 2026-09-09 WAS FALSE (audit F150). It said the cookie "would need
# CRT init", and the linker map disproves it: __DllMainCRTStartup@12 is the
# entry point and ___security_init_cookie is linked in, so the CRT is
# initialised and the cookie has everything it needs. What /GS- actually buys
# is dropping a prologue and epilogue from every function that holds a stack
# array, which here is the per-draw path. That makes the flag a MEASUREMENT
# question rather than a correctness one, and it has not been measured. It is
# also the only written defence of a disabled mitigation in a binary that runs
# inside a player's game and reads a named pipe, which is why the false
# sentence could not stay in a repository about to be published.
#
# MEASURED 2026-09-10 (F150 CLOSED) AND /GS IS RESTORED BELOW. Four passes of the
# game's own benchmark at 1280x720, two builds, profile=1, ProfReport pooled:
#
#     bucket                    /GS- ns/call   /GS ns/call
#     the tracker                   116.1         117.6
#     draw hook                     114.6         116.6
#     velocity redraw               677.7         688.1
#     state hooks                    13.5          13.7
#     vertex constants               77.3          77.1
#     pixel constants                27.5          27.5
#
# Weighted by call count that is ABOUT 1.7 us/FRAME, which is 0.02 percent of an
# 8 ms frame. The signal is coherent rather than noise: every bucket that holds a
# stack array is 1.3 to 1.7 percent slower, and the two that keep their scratch in
# STATIC arrays, the constant hooks, do not move at all. The binary grows 5,632
# bytes, which is the prologue and epilogue.
#
# THE BENCHMARK'S OWN FPS COULD NOT SEE IT and was never going to: 134.08 against
# 131.91 average, with a 6.59 fps spread between the two passes of the SAME build.
# The within-build noise is three times the between-build delta. That is the same
# structural limit docs/HANDOVER.md records, and it is why this was settled on the
# profiler rather than on the frame counter.
#
# So the mitigation goes back on. A disabled stack-protection flag in a binary
# that runs inside a player's game and reads a named pipe needs a better defence
# than 1.7 us/frame.
# /EHa, and NOT for the reason this comment used to give. It said "/EHa is
# required because this file uses SEH __try around game memory". __try compiles
# perfectly well under /EHsc. What /EHa actually buys is unwinding of C++ locals
# when a structured exception passes through, which matters here because the
# hooks hold RAII state while reading memory the game owns. There is no catch(
# anywhere in either binary. Audit M18.
# ALWAYS /O2. `-Configuration Debug` stood here until 2026-09-09 (audit F153)
# and it was a trap: it swapped /O2 for /Od /Zi, but the link line below carries
# no /DEBUG, so the linker emitted no program database for the .asi. A "Debug"
# build was therefore an UNOPTIMISED binary with nothing extra to debug it with,
# which is strictly worse than the Release one in both directions. If a
# debuggable build is ever genuinely wanted, /Zi on the compile AND /DEBUG on
# the link are both required, and it is the link half that was missing.
$optFlags = '/O2'

# /MAP is not optional. Two crashes sat unread for a day because the crash
# log named DLSS-IV.asi +0x491c and nothing could turn that into a
# function. With a map, +0x491c resolves to IvtDecideDepthMode+0x3c against
# the preferred load address 0x10000000, and the whole call chain with it.
# It costs nothing at runtime and the .map ships nowhere. It IS archived beside
# the zip by package.ps1, because otherwise the next local build overwrites the
# only thing that can read a crash report from the build a player is running.
# Audit M18.
$map = Join-Path $PSScriptRoot 'DLSS-IV.map'
$vkInc = Join-Path $root 'external\vulkan'   # Khronos Vulkan-Headers, for the transport

# WHICH SOURCE BUILT THIS BINARY, baked in as IVT_BUILD_ID and printed on the
# log's first line. The COMMIT, not the clock: a wall clock answers "when" when
# the only useful question is "what", and it is also the last thing standing
# between this project and a reproducible build (see -Analyze's sibling note and
# docs/reports/static-analysis-2026-09-09.md).
#
# "-dirty" is not decoration. A build from a modified tree is not the commit it
# names, and a field report that says 5d94a6a when the tree had uncommitted
# changes sends the next reader to the wrong source. `git status --porcelain`
# is empty exactly when the tree matches HEAD.
#
# git may be absent (a source archive, a machine without it). Then the id says
# so rather than inventing one, and both halves carry the same bare literal, so
# the host's build-id match at the handshake still passes. The clock is added
# only by the fallback INSIDE the sources (DLSS-IV.cpp, the host), which is
# reached only when neither script passes an id, that is a hand build with cl;
# the host does not compare two such ids, because they prove nothing.
$buildId = 'unknown-source'
try {
    $sha = (& git -C $root rev-parse --short HEAD 2>$null)
    if ($LASTEXITCODE -eq 0 -and $sha) {
        $dirty = (& git -C $root status --porcelain 2>$null)
        $buildId = if ($dirty) { "$sha-dirty" } else { "$sha" }
    }
} catch { }
# THE IDENTITY PROOF'S ID (see -IdentityBuildId above): an id of another length moves every string after it.
if ($IdentityBuildId) {
    $buildId = $IdentityBuildId
    Write-Host "  build id set by -IdentityBuildId: $buildId (the identity proof's, never a release)" -ForegroundColor Yellow
}

# /Brepro MAKES THE OUTPUT DETERMINISTIC. Measured 2026-09-09: two builds of
# identical source differed in exactly four bytes and all four were clocks. This
# switch is Microsoft's documented one for the two in the PE (the COFF
# TimeDateStamp and the debug directory's copy); IVT_BUILD_ID above removes the
# third. Together they mean one commit produces one binary, so an md5 identifies
# a SOURCE STATE and not merely a file, which is what makes a published zip
# checkable against published source (roadmap C7).
# THE PE DATE (2026-09-21): tools\pe-stamp.ps1 writes the COMMIT'S time where /Brepro wrote a hash that read as a date
# in 2058, one of the anomalies behind a machine-learning antivirus verdict on the 1.0.4 helper. As deterministic as
# the hash: one commit, one binary. Without git the hash stays.
$stamp = [uint32]0
try {
    $ct = (& git -C $root log -1 --format=%ct HEAD 2>$null)
    if ($LASTEXITCODE -eq 0 -and $ct) { $stamp = [uint32]$ct }
} catch { }
if ($Ship -and ($Release -or $Analyze -or $VerifyArm -or $Deploy)) { throw '-Ship is the shape 1.1 ships, whole (release, frame generation, armed) and both binaries: use it alone (with -IdentityBuildId at most). deploy.ps1 deploys a pair.' }
if ($Ship) { Write-Host '  THE SHAPE 1.1 SHIPS: DLSSIV_DEV=0 DLSSIV_FG=1 (the armed release), then the helper as build-host.ps1 -Release -Fg' -ForegroundColor Yellow }
$devDefine = if ($Ship) { '/DDLSSIV_DEV=0 /DDLSSIV_FG=1 ' } elseif ($Release) { '/DDLSSIV_DEV=0 ' } else { '' }
if ($Release) { Write-Host '  RELEASE build: DLSSIV_DEV=0, the development fences are compiled out' }
$cmd = "`"$vcvars`" >nul 2>&1 && rc /nologo /fo `"$res`" `"$rc`" && cl /nologo /LD /MT /GS /EHa /W3 /std:c++17 $optFlags " +
       "$devDefine/DWIN32 /D_WINDOWS /DIVT_BUILD_ID=\`"\`"\`"$buildId\`"\`"\`" /I`"$vkInc`" `"$src`" `"$res`" /Fe:`"$out`" /Fo:`"$obj`" " +
       "/link /SUBSYSTEM:WINDOWS /MACHINE:X86 /Brepro /MAP:`"$map`" d3d9.lib user32.lib version.lib"

# STATIC ANALYSIS, opt-in, and it exits instead of producing a binary.
#
# The baseline is docs/reports/static-analysis-2026-09-09.md: 24 warnings on the
# client once /wd6320 and /wd6322 suppress the deliberate SEH pattern, and 0 on
# the host; 29 after the 2026-09-14 re-run (the device thread's Dekker flags,
# the compiled-out profiler, the velocity batch); 42 after the 2026-09-19 re-run
# (fourteen of the new ones are the injection body's snapshot, which shadows the
# globals by name ON PURPOSE: the report's last section has every one); 41 on 2026-09-21; 40 after the latency audit
# (2026-09-24), the enforced baseline below. IT ANALYSES THE DEVELOPMENT CONFIGURATION ONLY: -Release with -Analyze is
# refused rather than silently analysing the other one (the final audit, 2026-09-30, deadB-13).
# Every one was triaged individually and is a false positive against a pattern
# this codebase uses on purpose. THAT is the point: against a known-clean
# baseline, a NEW warning is a signal instead of noise, which is the only state
# in which static analysis is useful on a mature codebase.
if ($Analyze) {
    if ($Release) { throw '-Analyze analyses the development configuration only; run it without -Release.' }
    $tmpObj = Join-Path $env:TEMP 'dlssiv-analyze.obj'
    $acmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /c /MT /GS /EHa /W3 /std:c++17 /O2 " +
            "/analyze:only /wd6320 /wd6322 /DWIN32 /D_WINDOWS " +
            "/DIVT_BUILD_ID=\`"\`"\`"$buildId\`"\`"\`" /I`"$vkInc`" `"$src`" /Fo:`"$tmpObj`""
    Write-Host "Analysing DLSS-IV.cpp (this is slower than a build)..." -ForegroundColor Cyan
    $aout = & cmd.exe /c $acmd 2>&1
    $aexit = $LASTEXITCODE
    # AN ANALYSIS THAT DID NOT RUN IS NOT A CLEAN ONE (the gaps audit 2026-09-19, G3.1). The exit code was never read here,
    # and it is the only thing that says the compiler ran: a failed vcvars32 short-circuits the && inside cmd with its
    # output already sent to nul, and a compile ERROR goes to stdout, where the warning pattern below does not match it.
    # Either way this printed "0 warning(s); baseline 42", advised lowering the baseline to 0, and exited 0, and
    # prove-all counted it a pass. The ordinary build a few lines down has always tested its exit code.
    if ($aexit -ne 0) {
        @($aout | Select-String -Pattern '\berror\b') | Select-Object -First 12 | ForEach-Object { Write-Host "  $_" }
        throw "The static analysis did not run to the end (exit $aexit). Nothing was analysed, so nothing was proved."
    }
    $warn = @($aout | Select-String -Pattern 'warning C\d+')
    $warn | ForEach-Object { Write-Host "  $_" }
    $baseline = 50   # 2026-10-28: 50 (fg_arm_life.h's FgpLongSay hands GetTickCount to FgShipLongStep, which takes differences only: a C28159 false positive: docs/reports/static-analysis-2026-09-09.md); 2026-10-27: 49 (the armed development build in view since round 9: six false positives, DT_BBS_BLIND's constant, two GetTickCount differences, FgDrivePlaceMed's sort; four fixed: docs/reports/static-analysis-2026-09-09.md); 2026-10-25: 44 (fg_machine_worker.h's one-writer read of dtPresTicksSum, a C28112 false positive: docs/reports/static-analysis-2026-09-09.md); 2026-10-24: 43 (fg_linux.h's close of the helper's copies, two C6387 false positives on one DuplicateHandle: docs/reports/static-analysis-2026-09-09.md); 2026-10-20: 41 (fg_plan.h's FgRideTake, a C6001 false positive: docs/reports/static-analysis-2026-09-09.md); 2026-10-10: 40 again (the merged placer's local `dead`, a C6244 for one commit, is `dline`); 2026-09-24: 40 after the latency audit (the latency work had left ten untriaged; all fixed, one documented idiom suppressed at its line); 41 since 2026-09-21 before that
    Write-Host ("  {0} warning(s); baseline {1}" -f $warn.Count, $baseline) -ForegroundColor Green
    if ($warn.Count -gt $baseline) {
        throw ("Static analysis regressed: {0} warnings against a baseline of {1}. " +
               "Read each new one. If it is another false positive, say why in " +
               "docs/reports/static-analysis-2026-09-09.md and raise the baseline there " +
               "and here, in the same commit." -f $warn.Count, $baseline)
    }
    # /analyze on this file has never produced fewer than 24. Zero means the compiler said nothing, not that it found nothing.
    if ($warn.Count -eq 0) { throw 'The static analysis reported 0 warnings, which it has never done on this file: it did not run.' }
    if ($warn.Count -lt $baseline) {
        Write-Host ("  fewer than the baseline: lower it to {0} here and in the report." -f $warn.Count) -ForegroundColor Yellow
    }
    return
}

if ($VerifyArm) {
    if ($Release) { throw '-VerifyArm compiles the development configuration only; run it without -Release.' }
    $tmpObj = Join-Path $env:TEMP 'dlssiv-verifyarm.obj'
    $vcmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /c /MT /GS /EHa /W3 /std:c++17 $optFlags /DDLSSIV_M4D_INDEX_VERIFY=1 /DWIN32 /D_WINDOWS " +
            "/DIVT_BUILD_ID=\`"\`"\`"$buildId\`"\`"\`" /I`"$vkInc`" `"$src`" /Fo:`"$tmpObj`""
    Write-Host 'Compiling the development configuration with DLSSIV_M4D_INDEX_VERIFY=1 (no binary)...' -ForegroundColor Cyan
    $vout = & cmd.exe /c $vcmd 2>&1
    $vexit = $LASTEXITCODE
    if ($vexit -ne 0 -or -not (Test-Path -LiteralPath $tmpObj)) {
        @($vout | Select-String -Pattern '\berror\b') | Select-Object -First 12 | ForEach-Object { Write-Host "  $_" }
        throw "The M4D verify arm does not compile (exit $vexit)."
    }
    Remove-Item -LiteralPath $tmpObj -Force
    Write-Host ("  the verify arm compiles ({0} warning(s))" -f @($vout | Select-String -Pattern 'warning C\d+').Count) -ForegroundColor Green
    return
}

Write-Host ("Building DLSS-IV.asi ({0}, x86)..." -f $(if ($Ship) { 'the armed release, 1.1''s shape' } elseif ($Release) { 'release' } else { 'development' })) -ForegroundColor Cyan
$output = & cmd.exe /c $cmd 2>&1
$output | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0) { throw "Build failed." }
if ($stamp) { & (Join-Path $root 'tools\pe-stamp.ps1') -Path $out -Stamp $stamp }
else { Write-Host '  no git: the PE date stays the /Brepro hash' -ForegroundColor Yellow }

$info = Get-Item $out
Write-Host ("  {0:N0} bytes" -f $info.Length) -ForegroundColor Green

# Confirm it really is a 32-bit image.
$fs = [System.IO.File]::OpenRead($out)
try {
    $br = New-Object System.IO.BinaryReader($fs)
    $fs.Position = 0x3C
    $peOff = $br.ReadInt32()
    $fs.Position = $peOff + 4
    $machine = $br.ReadUInt16()
} finally { $fs.Dispose() }
if ($machine -ne 0x014C) {
    throw ("NOT a 32-bit image (machine 0x{0:X4}) - GTA IV cannot load it." -f $machine)
}
Write-Host "  machine 0x014C (x86) - correct for GTA IV" -ForegroundColor Green

# THE SHAPE 1.1 SHIPS: THE HELPER BESIDE IT (see -Ship above), from the same tree, with the same borrowed id if one was given.
# Its own script, so its flags stay written once; a failure there is this build's failure.
if ($Ship) {
    $hostArgs = @{ Release = $true; Fg = $true }
    if ($IdentityBuildId) { $hostArgs.IdentityBuildId = $IdentityBuildId }
    & (Join-Path $root 'build-host.ps1') @hostArgs
    if (-not $?) { throw 'The helper (build-host.ps1 -Release -Fg) did not build: the shape 1.1 ships is not whole.' }
}

if ($Deploy) {
    # WHERE THE GAMES LIVE IS NOT IN THIS FILE. It used to be, as one developer's
    # Steam path, which is a private detail of one machine sitting in a tracked
    # script (audit L43). The repository already has the convention: gamepath.txt
    # at the repository root is gitignored precisely because it holds a local
    # install path. Order: the environment first, so CI or a second machine needs
    # no file at all, then that file, then a refusal that says what to do.
    # TWO THINGS ABOUT gamepath.txt, both found by reading the file rather than
    # assuming: it carries a leading COMMENT line, and it names a specific build
    # FOLDER rather than the root the folders sit in. So take the first line that
    # is not a comment, and then take its PARENT. Reading it raw, or using it as
    # the root, both produce a deploy to a path that does not exist.
    $games = $env:GTAIV_ROOT
    if (-not $games) {
        $gp = Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) 'gamepath.txt'
        if (Test-Path $gp) {
            $line = Get-Content $gp | Where-Object { $_.Trim() -and -not $_.Trim().StartsWith('#') } | Select-Object -First 1
            if ($line) { $games = Split-Path $line.Trim() -Parent }
        }
    }
    if (-not $games) {
        throw ("no game install root. Set the GTAIV_ROOT environment variable to the folder " +
               "that CONTAINS the game build folders, or put a path to one of those build " +
               "folders in gamepath.txt at the repository root (it is gitignored).")
    }
    if (-not (Test-Path $games)) { throw "the game install root does not exist: $games" }
    $dest  = Join-Path $games $Deploy
    if (-not (Test-Path $dest)) { throw "No such game folder: $dest" }
    Copy-Item $out $dest -Force
    Write-Host "Deployed -> $dest\DLSS-IV.asi" -ForegroundColor Green
}
