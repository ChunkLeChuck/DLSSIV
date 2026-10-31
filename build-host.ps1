<#
.SYNOPSIS
    Builds dlssiv-helper.exe (x64), the resolve host, and optionally deploys it.

.DESCRIPTION
    NGX is 64-bit only, so the host is the one 64-bit piece next to the 32-bit
    .asi. Built with the NVIDIA DLSS SDK headers and import library in
    external\ngx (fetched from NVIDIA's public repository; licence alongside)
    and the Feeder's protocol header in source\transport (MIT).

    /MT with nvsdk_ngx_s.lib, the static-CRT import library, so the exe needs
    no VC++ redistributable on another machine. /EHa for the SEH around NGX.

.EXAMPLE
    .\build-host.ps1
    .\build-host.ps1 -Deploy "GTAIV1080 - Copy"
#>
[CmdletBinding()]
param(
    [string]$Deploy,
    # A RELEASE BUILD: DLSSIV_DEV=0, the host's half of the switch build.ps1 -Release
    # throws for the .asi. Both halves of one package are built the same way.
    [switch]$Release,
    # THE THIRD BUILD (2026-10-05, frame generation phase 2, stage 1; DECISIONS "PHASE 2, STAGE 0", A1): with -Release, adds
    # /DDLSSIV_FG=1, so frame generation's PRODUCT code is compiled into a release configuration and nothing of the
    # development build is. Its flavour word is "release" ("rel+gen" until round 8). Since 2026-10-25 it is the helper of the
    # shape 1.1 ships (build.ps1 -Ship), which tools\prove-fg-off.py --ship and prove-release-identity.py --ship read (the
    # .asi half of the third build, build.ps1 -Release -Fg, and its compare --fg were retired in round 8's deep strip).
    # Without -Release it is refused: a development build has that code already.
    # ITS FRAME GENERATION RUNS ON THE MOD'S OWN LIBRARY ALONE (2026-10-25; host\fg_own.h, "THE SHIPPED HELPER"): dlssiv-fg.dll
    # beside the helper, held to the size and SHA-256 of tools\ffx-own\RECORD-dlssiv-fg-cut.txt, which this script reads and
    # hands to the compiler (DLSSIV_FG_OWN_SIZE, DLSSIV_FG_OWN_SHA). Never typed: a new record is a new build.
    [switch]$Fg,
    # THE IDENTITY PROOF'S BUILD ID (2026-09-28, S6's gate G-C0a): as build.ps1's, for tools\prove-release-identity.py only.
    [string]$IdentityBuildId = ''
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
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found at: $vcvars" }

$src = Join-Path $root 'host\dlssiv-helper.cpp'
$out = Join-Path $root 'host\dlssiv-helper.exe'
$obj = Join-Path $root 'host\dlssiv-helper.obj'
$rc  = Join-Path $root 'host\dlssiv-helper.rc'   # the version resource (2026-09-21); its number is source\version.h's
$res = Join-Path $root 'host\dlssiv-helper.res'
$ngxInc = Join-Path $root 'external\ngx\include'
$ngxLib = Join-Path $root 'external\ngx\lib\nvsdk_ngx_s.lib'
# THE IMPORT LIBRARY IS NOT IN THE REPOSITORY, on purpose: .gitignore excludes
# *.lib, and the NVIDIA DLSS SDK licence permits shipping the SDK inside a built
# binary rather than as a file of its own. Without this check a fresh clone gets
# LNK1181 from the linker and nothing to explain it, which is audit finding M17.
# Refuse here instead, with the one command that fixes it.
#
# ("and touch the network only if it is actually missing" used to end that
# paragraph and is no longer true, which is the whole of F149 below: -Check
# reads the file on disk and does not fetch. Only fetch-ngx.ps1 without -Check
# touches the network.)
#
# THE CHECK RUNS WHETHER OR NOT THE FILE IS THERE (audit F149). It used to run
# only when the library was ABSENT, which is the one case in which there is
# nothing to verify: a library that was PRESENT was linked unexamined, so the
# hash recorded in fetch-ngx.ps1 pinned nothing that a build could ever see.
# -Check now exits non-zero on a missing file, on something that is not a COFF
# archive, and on a hash that is not the recorded one, so this is the line that
# decides whether the host is provably linked against the library that
# external\ngx\VERSION.txt names. A developer deliberately on a newer SDK
# passes -AllowDifferentHash.
#
# $global:LASTEXITCODE IS LOAD-BEARING and is not decoration. A PowerShell
# script that returns without calling `exit` leaves $LASTEXITCODE untouched, so
# an unrelated earlier failure would be read here as a bad library. Measured,
# not assumed. Same reason it is seeded in package.ps1.
$global:LASTEXITCODE = 0
& (Join-Path $root 'tools\fetch-ngx.ps1') -Check
if ($global:LASTEXITCODE -ne 0) {
    if (-not (Test-Path $ngxLib)) {
        throw "the NGX import library is missing. Run .\tools\fetch-ngx.ps1 and build again."
    }
    throw "the NGX import library is not the one this project pins. See the lines above."
}
# ffx-api HEADERS ONLY. There is deliberately no import library: AMD's
# library (2.x's amd_fidelityfx_loader_dx12.dll, or 1.1's amd_fidelityfx_dx12.dll)
# is resolved at runtime through their own ffx_api_loader.h, so this exe still
# starts when it is absent.
$ffxInc = Join-Path $root 'external\ffx'
# INTEL XeSS: HEADERS ONLY, the same way (2026-10-28, X2; host\xess_host.h). libxess.dll is loaded by its full path beside the
# helper with GetProcAddress, so there is no import library and this exe starts when the file is absent.
$xessInc = Join-Path $root 'external\xess\inc'
if (-not (Test-Path (Join-Path $xessInc 'xess\xess_d3d12.h'))) { throw "Intel's XeSS headers are missing: $xessInc\xess\xess_d3d12.h. Nothing has been built." }

# /MAP, for the same reason the client has one: a crash address in the host is
# unreadable without it, and the host is the process that talks to NGX, ffx-api
# and the pipe. It had none at all until 2026-09-09. Audit M18.
$map = Join-Path $root 'host\dlssiv-helper.map'

# The commit that built this, and /Brepro so one commit gives one binary. Both
# halves must ship together, so seeing both build ids in one log is how a
# version mismatch reads as a mismatch rather than as a fault. The reasoning is
# written out once, at the same define in source/DLSS-IV.cpp.
$buildId = 'unknown-source'
try {
    $sha = (& git -C $root rev-parse --short HEAD 2>$null)
    if ($LASTEXITCODE -eq 0 -and $sha) {
        $dirty = (& git -C $root status --porcelain 2>$null)
        $buildId = if ($dirty) { "$sha-dirty" } else { "$sha" }
    }
} catch { }
# THE IDENTITY PROOF'S ID (see -IdentityBuildId above).
if ($IdentityBuildId) {
    $buildId = $IdentityBuildId
    Write-Host "  build id set by -IdentityBuildId: $buildId (the identity proof's, never a release)" -ForegroundColor Yellow
}

# THE PE DATE (2026-09-21): tools\pe-stamp.ps1 writes the COMMIT'S time where /Brepro wrote a hash that read as a date
# in 2058, one of the anomalies behind a machine-learning antivirus verdict on the 1.0.4 helper. As deterministic as
# the hash: one commit, one binary. Without git the hash stays.
$stamp = [uint32]0
try {
    $ct = (& git -C $root log -1 --format=%ct HEAD 2>$null)
    if ($LASTEXITCODE -eq 0 -and $ct) { $stamp = [uint32]$ct }
} catch { }
if ($Fg -and -not $Release) { throw '-Fg is the release configuration with frame generation''s product code compiled in: use it with -Release. A development build has that code already.' }
$devDefine = if ($Release -and $Fg) { '/DDLSSIV_DEV=0 /DDLSSIV_FG=1 ' } elseif ($Release) { '/DDLSSIV_DEV=0 ' } else { '' }
# THE OWN LIBRARY'S RECORD (-Release -Fg; see -Fg above): its file name, size and SHA-256, each read by its key at the start of
# a line, and every one of them there exactly once, or nothing is built
$ownDefine = ''
if ($Release -and $Fg) {
    $ownRecord = Join-Path $root 'tools\ffx-own\RECORD-dlssiv-fg-cut.txt'
    if (-not (Test-Path $ownRecord)) { throw "the own library's record is missing: $ownRecord. Nothing has been built." }
    $ownText = [System.IO.File]::ReadAllText($ownRecord)
    $ownLib  = [regex]::Matches($ownText, '(?m)^library (\S+)\r?$')
    $ownSize = [regex]::Matches($ownText, '(?m)^size (\d+)\r?$')
    $ownSha  = [regex]::Matches($ownText, '(?m)^sha256 ([0-9a-f]{64})\r?$')
    if ($ownLib.Count -ne 1 -or $ownSize.Count -ne 1 -or $ownSha.Count -ne 1 -or $ownLib[0].Groups[1].Value -ne 'dlssiv-fg.dll') {
        throw "the own library's record ($ownRecord) does not hold one 'library dlssiv-fg.dll', one 'size' and one 'sha256' line. Nothing has been built."
    }
    $ownDefine = "/DDLSSIV_FG_OWN_SIZE=$($ownSize[0].Groups[1].Value)ull /DDLSSIV_FG_OWN_SHA=\`"\`"\`"$($ownSha[0].Groups[1].Value)\`"\`"\`" "
    Write-Host ("  frame generation's library, held to its record: dlssiv-fg.dll, {0} bytes, sha256 {1}" -f $ownSize[0].Groups[1].Value, $ownSha[0].Groups[1].Value)
}
if ($Release) { Write-Host '  RELEASE build: DLSSIV_DEV=0, the development log is compiled out' }
if ($Fg) { Write-Host '  WITH FRAME GENERATION''S PRODUCT CODE: DLSSIV_FG=1 (the shipped helper: build.ps1 -Ship builds it so)' -ForegroundColor Yellow }
$cmd = "`"$vcvars`" >nul 2>&1 && rc /nologo /fo `"$res`" `"$rc`" && cl /nologo /O2 /EHa /MT /W3 /std:c++17 $devDefine$ownDefine/DWIN32 /DUNICODE " +
       "/DIVT_BUILD_ID=\`"\`"\`"$buildId\`"\`"\`" " +
       "/I`"$ngxInc`" /I`"$ffxInc`" /I`"$xessInc`" `"$src`" `"$res`" /Fe:`"$out`" /Fo:`"$obj`" " +
       "/link /SUBSYSTEM:CONSOLE /MACHINE:X64 /Brepro /MAP:`"$map`" `"$ngxLib`" d3d12.lib dxgi.lib kernel32.lib user32.lib advapi32.lib"

Write-Host "Building dlssiv-helper.exe (x64)..." -ForegroundColor Cyan
$output = & cmd.exe /c $cmd 2>&1
$output | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0) { throw "Build failed." }
if ($stamp) { & (Join-Path $root 'tools\pe-stamp.ps1') -Path $out -Stamp $stamp }
else { Write-Host '  no git: the PE date stays the /Brepro hash' -ForegroundColor Yellow }

$info = Get-Item $out
Write-Host ("  {0:N0} bytes" -f $info.Length) -ForegroundColor Green
if (Test-Path $map) {
    Write-Host ("  {0:N0} bytes of symbols in {1}" -f (Get-Item $map).Length, (Split-Path $map -Leaf)) -ForegroundColor DarkGray
}

$fs = [System.IO.File]::OpenRead($out)
try {
    $br = New-Object System.IO.BinaryReader($fs)
    $fs.Position = 0x3C
    $peOff = $br.ReadInt32()
    $fs.Position = $peOff + 4
    $machine = $br.ReadUInt16()
    # DllCharacteristics, checked because the machine type already is and this is
    # the same class of fact: something the toolchain decides that nobody looks
    # at again. 0x0020 HIGH_ENTROPY_VA, 0x0040 DYNAMICBASE, 0x0100 NX_COMPAT,
    # 0x4000 GUARD_CF. Reported rather than enforced: what is correct here is a
    # decision, and a build script is the wrong place to make it. Audit M18.
    $fs.Position = $peOff + 4 + 20 + 70
    $dllChar = $br.ReadUInt16()
} finally { $fs.Dispose() }
if ($machine -ne 0x8664) { throw ("NOT a 64-bit image (machine 0x{0:X4})." -f $machine) }
Write-Host "  machine 0x8664 (x64) - correct for the NGX host" -ForegroundColor Green
Write-Host ('  DllCharacteristics 0x{0:X4}: DYNAMICBASE {1}, NX_COMPAT {2}, HIGH_ENTROPY_VA {3}, GUARD_CF {4}' -f $dllChar,
    [bool]($dllChar -band 0x40), [bool]($dllChar -band 0x100), [bool]($dllChar -band 0x20), [bool]($dllChar -band 0x4000))

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
    $dest  = Join-Path (Join-Path $games $Deploy) 'DLSSIV'   # host64 until 2026-09-10; see source/transport/dlssiv_layout.h
    if (-not (Test-Path $dest)) { throw "No such DLSSIV folder: $dest (deploy.ps1 renames an old host64 folder; run it once)" }
    Copy-Item $out $dest -Force
    Write-Host "Deployed -> $dest\dlssiv-helper.exe" -ForegroundColor Green
}
