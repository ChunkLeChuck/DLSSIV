<#
.SYNOPSIS
    Builds ffx-own-fg.dll (x64): AMD's analytical frame generation provider, from AMD's public source as vendored in
    external\ffx\ffx_api_2.3.0. The own build of AMD's frame generation, stage 1a (2026-10-06). SINCE STAGE 2
    (2026-10-07) ALSO dlssiv-fg.dll, with -Target dlssiv-fg: the same objects behind the mod's own interface.

.DESCRIPTION
    AMD publishes the source of its frame generation provider 3.1.6 (frame interpolation and optical flow, MIT) and no
    way to build it: no project file, no shader tool, no provider list. This script is the mod's own build of it.

    WHAT IS COMPILED. Twelve of AMD's files, unmodified (tools\ffx-own\ffx_manifest.py holds each to AMD's tag), and
    two of ours from ffx-own\glue: the provider list AMD's tree declares and does not define, and two stubs for the
    swapchain this library does not contain. The 72 shader headers AMD's C++ includes are the committed ones in
    ffx-own\shaders (tools\ffx-own\gen_shaders.py makes and verifies them). AMD's frame interpolation swapchain is
    NOT compiled and its files are not in the repository.

    THE NAME IS A RULE. The output is ffx-own-fg.dll and nothing else. It is never named like a file of AMD's: players
    add FSR 4 by dropping a community file over amd_fidelityfx_upscaler_dx12.dll, the helper loads AMD's files through
    AMD's signed loader by name, and a build of ours under one of those names would be mistaken for theirs by both.
    WHAT THE SCRIPT DOES ABOUT IT, exactly: the names it writes are fixed below and none begins with "amd_"; and it
    does not build into a folder that holds a file named amd_fidelityfx*. It looks before anything else, and if there
    is one it stops there: nothing in that folder is deleted, nothing is written. A build folder of ours and a folder
    with AMD's files are never the same folder.

    WHAT IT TOUCHES IN ITS OUTPUT FOLDER: FILES OF ITS OWN NAMES, AND NO OTHER. Before a build it removes the five
    files it delivers ($name.dll, $name.map, build.log, reads.txt, toolchain.txt) and the files older versions of
    this script left there, each by its name. It used to remove every .dll, .cmd, .lib, .map, .obj, .exp and .rsp in
    the folder whoever had put it there (the review of 2026-10-06, R3.1: the /MD twin's build deleted the proven /MT
    library, and somebody else's .dll and .cmd went with it). The twin now has a default folder of its own as well.

    A FAILED BUILD LEAVES NO LIBRARY (the same review, R3.2: a build that failed on a warning left ffx-own-fg.dll at
    the very path the next stage loads from). Everything is compiled and linked inside the temp copy; the exit codes,
    the warnings, the 64-bit check and the toolchain's versions are all read THERE; and only after the last check are
    the five files copied to the output folder, the library last. On any failure the output folder holds build.log,
    to read why, and no library of this script's: the old one was removed at the start and no new one arrived.
    REFUSALS ARE NOT FAILURES OF A BUILD: wrong arguments, no compiler, a missing source, a folder with AMD's files,
    a temp folder it cannot use (too deep, or with a letter outside ASCII) stop the script BEFORE it has touched
    anything, and the output folder is then exactly as it was.

    IT CARRIES NO VERSION RESOURCE AND NO BUILD ID in this stage, so its bytes do not depend on the commit, the date
    or the folder: /Brepro on both cl and link; no /Zi, so no PDB and no PDB path in the file. They depend on the
    vendored source, the two glue files, the headers, the options written in this script and THE TOOLCHAIN: another
    cl, link or Windows SDK makes other bytes. So the script writes the three versions it found into toolchain.txt
    beside the library and prints them, and tools\ffx-own\RECORD.txt holds the library's size and SHA-256 together
    with the toolchain they belong to: tools\prove-ffx-own.py builds it in two folders, compares the bytes with each
    other and with that record, and fails on another toolchain with a line that says so. (When it is packaged it gets
    the mod's own version resource, never AMD's.)

    THE COMPILER TAKES NO OPTION FROM THE ENVIRONMENT. cl reads the variables CL and _CL_ and link reads LINK and
    _LINK_ as if they were typed on its command line, silently: with _CL_=/fp:fast in the caller's environment this
    script once made another library and said nothing (the review of 2026-10-06 did it). The four are cleared for the
    compiler and the linker, and the script says so when one was set.

    /MT, as dlssiv-helper.exe: no VC++ runtime DLL is needed beside it. -DynamicCrt builds the /MD twin
    (ffx-own-fg-md.dll, into obj\ffx-own-md) and exists for one question of the next stage only: AMD's own library
    takes its float math (tanf, atanf, log2f, sinf, cosf) from the system's C runtime, ours from the static one, and
    if the generated picture ever differs in a last bit, the twin says whether that is why.

    /std:c++20 because AMD's source needs it (std::span, std::popcount). /EHsc, /O2 /Ob2, /W3 as the trial build had
    them; any compiler or linker warning fails the build. No /GL.

    -Target dlssiv-fg BUILDS THE SECOND LIBRARY (2026-10-07, the own build's stage 2): dlssiv-fg.dll, into obj\dlssiv-fg.
    It is AMD's frame interpolation, optical flow and backend behind THE MOD'S OWN INTERFACE (ffx-own\glue\dlssiv_fg.h,
    implemented by ffx-own\glue\dlssiv_fg.cpp), with none of AMD's five entry points: nine of the twelve files of
    AMD's, the two stubs for the swapchain, and that one file of ours. AMD's ffx-api core, its query fallback and its
    provider layer are NOT linked into it. THE OBJECTS OF AMD'S FILES IN IT ARE THE REFERENCE LIBRARY'S OWN, and that is
    made true by how it is built and not by a comparison afterwards: one compiler run makes every object, the linker
    then makes ffx-own-fg.dll from them exactly as the default target does (that library stays in the temp copy and
    is NOT delivered), and then dlssiv-fg.dll from nine of the same object files. objects.txt, delivered beside the
    library, holds the SHA-256 of that sibling and of every object linked: tools\prove-ffx-own.py holds the sibling
    to tools\ffx-own\RECORD.txt, so the objects in dlssiv-fg.dll are the ones the recorded reference library is made
    of. dlssiv-fg.dll has a record of its own, tools\ffx-own\RECORD-dlssiv-fg.txt. It carries no version resource
    and no build id either, for the same reason (the package stage gives it the mod's own, never AMD's).
    -DynamicCrt is the reference library's instrument and is refused with this target.

    -TestName alt-fi or alt-of IS FOR tools\ffx-own\build_alt.py AND FOR NOTHING ELSE. That script makes a temp copy
    of this script and its inputs, changes ONE constant in one of AMD's shaders there, regenerates the headers from
    the changed copy and runs the copy of this script with -TestName: the result is ffx-own-fg-alt-fi.dll or
    ffx-own-fg-alt-of.dll, a library that must make ANOTHER picture and that the next stage's picture compare must
    therefore fail on (with -Target dlssiv-fg: dlssiv-fg-alt-fi.dll or dlssiv-fg-alt-of.dll, the same control for the
    second library). Two locks keep a test library and the product apart. A tree that build_alt.py has altered
    carries the file ffx-own\ALTERED.txt (what was changed, where): with -TestName the script wants that file and
    refuses a tree without it, so the unaltered source can never come out under a test's name (a control that could
    not fail); and without -TestName it refuses a tree that has it, so an altered tree can never make ffx-own-fg.dll.
    The file is delivered beside the test library as <its name>.txt.

    SINCE THE OWN BUILD'S STAGE 3 (2026-10-10) -Target dlssiv-fg MAKES TWO LIBRARIES IN ONE RUN: THE REFERENCE AND THE
    PRODUCT. Stage 3 makes a generated frame cheaper without changing one bit of it, by CUTS: named, recorded changes
    of ours (ffx-own\cuts\CUTS.txt lists them, ffx-own\cuts\<name>.cut is each one, tools\ffx-own\cuts.py says how
    they are written and applied).
      THE REFERENCE is stage 2's library with no cut: compiled and linked EXACTLY as stage 2 built dlssiv-fg.dll, so its
      bytes are still the ones of stage 2's record (tools\ffx-own\RECORD-dlssiv-fg.txt), and they still call themselves
      dlssiv-fg.dll inside. It is delivered under a reference name of ours, dlssiv-fg-ref.dll, into a folder of its
      own beside the output folder (<output folder>-ref: obj\dlssiv-fg-ref), with the six files stage 2 delivered.
      THE PRODUCT is dlssiv-fg.dll: after the reference is linked, the cuts that are "in" are applied TO THE TEMP COPY
      of AMD's source (the vendored files are never written: tools\ffx-own\ffx_manifest.py holds them), the shader
      headers those cuts change replace the copy's (the committed ones of ffx-own\shaders-cut for the product's set;
      compiled then and there by the pinned dxc for any other set), the eleven files of the second library are
      compiled AGAIN, by the same command line, into a folder of their own, and linked. It has a record of its own
      (tools\ffx-own\RECORD-dlssiv-fg-cut.txt). cuts.txt, delivered beside it, says which cuts are in it, their
      digest, every file of AMD's an edit changed (git blob id before and after) and every header replaced;
      objects.txt names the reference it was built beside (which must be stage 2's record's).
      WITH NO CUT IN, THE PRODUCT IS THE REFERENCE, BYTE FOR BYTE: nothing is compiled twice.
      reads.txt is the FIRST compile's: what the compiler read of AMD's unmodified files, held to the manifest as ever.
    -Cuts <set> BUILDS ANOTHER SET THAN THE PRODUCT'S ("none"; exact names; "+name" and "-name" to add to and take
    from the product's): an experiment or a price of one cut alone. It wants -OutDir, a folder of its own, so that it
    never lies where the product is looked for, and no reference is delivered with it.
    -TestName px (with -Target dlssiv-fg, from the unaltered tree) builds THE JUDGE'S ONE-PIXEL CONTROL: the exact
    library's cuts and the cut "control-one-pixel", as dlssiv-fg-px.dll. It must fail the picture compare.

    SINCE 2026-10-12 THE PRODUCT'S PICTURE IS ITS OWN, AND -Target dlssiv-fg MAKES THREE LIBRARIES IN ONE RUN. The owner
    accepted, in the game, a library with three cuts that change the picture (ffx-own\cuts\CUTS.txt: "accepted"), and
    it is the product. What was the product until then is kept as THE EXACT LIBRARY, so that the proof of the exact
    cuts is neither lost nor weakened:
      THE REFERENCE, as above: no cut.
      THE EXACT LIBRARY: the cuts marked "in" and no other, compiled and linked EXACTLY as the product was until that
      day (the same command line, the same link line, /OUT:dlssiv-fg.dll), so its bytes are the ones the judge proved
      against the reference and they still call themselves dlssiv-fg.dll inside. It is delivered under a name of its
      own that is never the product's file's, dlssiv-fg-exact.dll, into a folder of its own beside the output folder
      (<output folder>-exact: obj\dlssiv-fg-exact), with its seven files, and it has a record of its own
      (tools\ffx-own\RECORD-dlssiv-fg-exact.txt, whose "library" line is that file name: the kit's installer, which
      wants a record of dlssiv-fg.dll, cannot be handed it).
      THE PRODUCT, dlssiv-fg.dll: the accepted picture cuts are then applied TO THE SAME COPY, on top of the exact
      ones, the headers they change replace the copy's (the committed ones of ffx-own\shaders-picture), and the
      eleven files are compiled a third time by the same command line and linked. objects.txt beside it names the
      sibling, the reference AND the exact library it was built on. With no accepted cut the product IS the exact
      library, byte for byte, and nothing is compiled a third time.
    THE TEST LIBRARIES (-TestName alt-fi, alt-of, px) ARE BUILT ON THE EXACT LIBRARY'S CUTS, never on the product's: a
    control with a picture cut in it would fail the compare whatever it was a control of.
    -Cuts exact (with -OutDir) builds the exact library's set alone; "exact,+name" adds to it (a variant library is
    built that way, so that it does not move when the product takes a picture cut).
    The cuts need Python on the PATH (or DLSSIV_PYTHON): tools\ffx-own\cuts.py is the one place that reads a cut.

    THE COMPILER READS A COPY OF THE SOURCE IN A SHORT TEMP FOLDER, and that is not a style choice. AMD's files include
    each other through chains of "../": cl joins them unresolved, the longest joined string is the kit's own path plus
    175 characters, and Windows allows 260. From this repository's folder that left 3 characters to spare on the day
    it was written (2026-10-06), and from a folder a little deeper the build stops with "cannot open include file" on
    a file that is there (AMD's own build script carries a note about the same thing; running the compiler inside the
    kit with relative paths does not help, it puts the folder back in front). So the kit, the 72 headers and the glue
    are copied to %TEMP%\ffxk-<8 hex digits>, compiled and linked there and the copy is removed. The folder's name is
    new on every build, which also means every build shows that the library's bytes do not depend on it. Set
    DLSSIV_FFX_STAGE to another short folder if %TEMP% is itself too deep; the script says so when it is. It also
    says so when the temp folder's path has a character outside ASCII (a Windows user name with an accent): the
    compiler's response files are written in ASCII, the letter became "?" in them, and the compiler then reported a
    source file it "cannot open" in a folder that does not exist (the review of 2026-10-06, R3.5).

    EVERYTHING RUNS AT BelowNormal PRIORITY: this process lowers its own priority class for the length of the build,
    the compiler and the linker inherit it, and it is put back after. Compiling is CPU work only: nothing here creates
    a Direct3D device.

    WHAT IS DELIVERED beside the library: its map (to read a crash address), build.log, toolchain.txt, and reads.txt:
    every file of AMD's the compiler opened, each with the git blob id of the bytes it opened, hashed in the copy it
    was compiled from before that copy is removed. The proof holds those pairs against the manifest, so "the build
    read AMD's files" is said of bytes and not of names (the review of 2026-10-06, R1.3: a script pointed at another
    kit passed, because only the names were compared). For the same reason the build fails if the compiler opened
    any file that is not in its copy, among its own headers or in the Windows SDK. The three logs are the last
    build's: two libraries built into one folder on purpose keep their own .dll and .map and share those three.

.EXAMPLE
    .\build-ffx-own.ps1
    .\build-ffx-own.ps1 -OutDir C:\some\folder
    .\build-ffx-own.ps1 -Target dlssiv-fg
#>
[CmdletBinding()]
param(
    # Where the library, its map and the logs go. Default: obj\ffx-own beside this script (obj\ffx-own-md for the
    # twin), which git ignores.
    [string]$OutDir,
    # /MD instead of /MT, named ffx-own-fg-md.dll. See the description: an instrument for the next stage, never the product.
    [switch]$DynamicCrt,
    # A TEST library from an altered copy of the source: tools\ffx-own\build_alt.py passes it. See the description.
    [ValidateSet('', 'alt-fi', 'alt-of', 'px')]
    [string]$TestName = '',
    # ANOTHER SET OF CUTS THAN THE PRODUCT'S, with -Target dlssiv-fg and -OutDir (the description says how a set is written).
    [string]$Cuts = '',
    # A VARIANT LIBRARY (2026-10-11): a set with a cut that CHANGES THE PICTURE (ffx-own\cuts\CUTS.txt: "picture"), built
    # under a name of its own, dlssiv-fg-v-<name>.dll, for the owner's eye. With -Target dlssiv-fg, -Cuts and -OutDir.
    # NEVER THE PRODUCT: tools\ffx-own\cuts.py refuses such a cut in any build that is not a variant, and a variant
    # with no such cut. tools\ffx-own\build_variants.py builds them all.
    [ValidatePattern('^([a-z][a-z0-9-]{1,30})?$')]
    [string]$Variant = '',
    # WHICH LIBRARY: ffx-own-fg (the reference: AMD's provider layer, AMD's five entry points) or dlssiv-fg (the same
    # objects of AMD's two effects and backend behind the mod's own interface). See the description.
    [ValidateSet('ffx-own-fg', 'dlssiv-fg')]
    [string]$Target = 'ffx-own-fg'
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# git's id of a file's bytes: sha1 of "blob <size>", a zero byte and the bytes. What the manifest lists for each file.
function Get-BlobId([string]$path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $head = [System.Text.Encoding]::ASCII.GetBytes("blob $($bytes.Length)`0")
    $sha = [System.Security.Cryptography.SHA1]::Create()
    try {
        [void]$sha.TransformBlock($head, 0, $head.Length, $null, 0)
        [void]$sha.TransformFinalBlock($bytes, 0, $bytes.Length)
        return ([System.BitConverter]::ToString($sha.Hash)).Replace('-', '').ToLower()
    } finally { $sha.Dispose() }
}

# WHERE VISUAL STUDIO IS: found exactly as build-host.ps1 finds it (its comment says why each step is there).
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

$kit     = Join-Path $root 'external\ffx\ffx_api_2.3.0'
$glue    = Join-Path $root 'ffx-own\glue'
$shaders = Join-Path $root 'ffx-own\shaders'

# AMD's files, relative to the kit, in the order they are linked. The order is part of the build: the linker lays
# code out by it.
$amd = @(
    'api\internal\ffx_api.cpp',
    'api\internal\ffx_assert.cpp',
    'api\internal\ffx_message.cpp',
    'api\internal\ffx_object_management.cpp',
    'api\internal\ffx_query_fallback.cpp',
    'backend\dx12\ffx_dx12.cpp',
    'backend\dx12\ffx_backends_dx12.cpp',
    'framegeneration\fsr3\internal\ffx_frameinterpolation.cpp',
    'framegeneration\fsr3\internal\ffx_frameinterpolation_shaderblobs.cpp',
    'framegeneration\fsr3\internal\ffx_opticalflow.cpp',
    'framegeneration\fsr3\internal\ffx_opticalflow_shaderblobs.cpp',
    'framegeneration\fsr3\internal\ffx_provider_fsr3framegeneration.cpp'
)
$ours = @('own_providers.cpp', 'own_no_swapchain.cpp')
# THE SECOND LIBRARY (-Target dlssiv-fg): one more file of ours is compiled, and its link takes these of the objects.
# AMD's ffx-api core (ffx_api.cpp), its query fallback and its provider layer are not among them, nor is our provider list.
$second = $Target -eq 'dlssiv-fg'
$oursFg = 'dlssiv_fg.cpp'
$amdFg  = @($amd | Where-Object { $_ -notmatch '\\(ffx_api|ffx_query_fallback|ffx_provider_fsr3framegeneration)\.cpp$' })
if ($second -and $amdFg.Count -ne 9) { throw "the second library links nine of AMD's twelve files, and this script found $($amdFg.Count)." }
if ($second -and $DynamicCrt) { throw '-DynamicCrt is the reference library''s instrument: it is not built with -Target dlssiv-fg. Nothing was built.' }
if ($second -and -not (Test-Path (Join-Path $glue $oursFg))) { throw "missing source: $(Join-Path $glue $oursFg)" }
# THE CUTS (stage 3; the description says what each of these is)
$cutsGiven = $PSBoundParameters.ContainsKey('Cuts')
$px = $TestName -eq 'px'
if ($cutsGiven -and -not $second) { throw '-Cuts is for -Target dlssiv-fg: the reference library of stage 1 has no cut. Nothing was built.' }
if ($cutsGiven -and -not $PSBoundParameters.ContainsKey('OutDir')) { throw '-Cuts builds a set that is not the product''s: give it -OutDir, a folder of its own. Nothing was built.' }
if ($px -and (-not $second -or $cutsGiven)) { throw '-TestName px is the one-pixel control of -Target dlssiv-fg, on the product''s cuts: without -Cuts. Nothing was built.' }
if ($Variant -and (-not $second -or -not $cutsGiven -or $TestName)) { throw '-Variant builds a variant library (a set with a cut that changes the picture): with -Target dlssiv-fg, -Cuts and -OutDir, and without -TestName. Nothing was built.' }
# (2026-10-12) THE CONTROLS STAND ON THE EXACT LIBRARY'S CUTS, never on the product's: the one-pixel control and the two altered
# test libraries must differ from AMD's picture BY THEIR OWN CHANGE ALONE, and the product's picture is its own since
# that day (its accepted picture cuts; tools\ffx-own\cuts.py, at STATES). A control with a picture cut in it would
# fail the compare whatever it was a control of.
$cutSpec = if ($px) { 'exact,+control-one-pixel' } elseif ($TestName) { 'exact' } else { $Cuts }
$cutsPy = Join-Path $root 'tools\ffx-own\cuts.py'
$python = $env:DLSSIV_PYTHON
if ($second) {
    if (-not $python) { $python = (Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source }
    if (-not $python) { throw 'the cuts are read and applied by tools\ffx-own\cuts.py, and no python is on the PATH (set DLSSIV_PYTHON to its full path). Nothing was built.' }
    if (-not (Test-Path $cutsPy)) { throw "missing: $cutsPy" }
    if (-not (Test-Path (Join-Path $root 'ffx-own\cuts\CUTS.txt'))) { throw "missing: $(Join-Path $root 'ffx-own\cuts\CUTS.txt'), the list of cuts the build reads" }
}
# A MISSING FILE OF THE KIT HAS ONE CAUSE NOBODY WOULD GUESS (the review of 2026-10-06, R3.7): the kit's longest path is 163
# characters from the repository's root, and under a root longer than about 95 characters git cannot write it unless
# core.longpaths is true. The short temp copy covers the compiler, not git's checkout.
$longRoot = "python tools\ffx-own\ffx_manifest.py lists what is missing. If the working tree's root is longer than about 95 characters, git could not write the kit's longest paths (163 characters from the repository's root; Windows allows 260) unless core.longpaths is true."
foreach ($s in $amd)  { if (-not (Test-Path (Join-Path $kit $s)))  { throw "missing source: $(Join-Path $kit $s). $longRoot" } }
foreach ($s in $ours) { if (-not (Test-Path (Join-Path $glue $s))) { throw "missing source: $(Join-Path $glue $s)" } }
if (@(Get-ChildItem $shaders -Filter '*_permutations.h' -ErrorAction SilentlyContinue).Count -ne 72) {
    throw "ffx-own\shaders does not hold the 72 shader headers. Run: python tools\ffx-own\gen_shaders.py"
}

# THE TWO LOCKS BETWEEN A TEST LIBRARY AND THE PRODUCT (the description says why).
$altered = Join-Path $root 'ffx-own\ALTERED.txt'
if ($px) {
    if (Test-Path $altered) { throw "this tree is marked as an altered copy of the source (ffx-own\ALTERED.txt): the one-pixel control is built from the unaltered tree. Nothing was built." }
} elseif ($TestName) {
    if ($DynamicCrt) { throw '-TestName and -DynamicCrt are two different instruments: one at a time.' }
    if (-not (Test-Path $altered)) {
        throw ("-TestName builds a TEST library from an ALTERED copy of the source, and this tree is not one (no ffx-own\ALTERED.txt): it holds AMD's source as it is, " +
               "and a test library made from it would be the product under a test's name. Nothing was built. Run: python tools\ffx-own\build_alt.py")
    }
} elseif (Test-Path $altered) {
    throw "this tree is marked as an altered copy of the source (ffx-own\ALTERED.txt): it does not build ffx-own-fg.dll. Nothing was built."
}

# $ref: the reference library's name, as the default target makes it. $name: what this run delivers.
# $uncut: the second library as stage 2 linked it, with no cut (the reference, when this run delivers one).
$ref   = if ($TestName -and -not $px) { "ffx-own-fg-$TestName" } elseif ($DynamicCrt) { 'ffx-own-fg-md' } else { 'ffx-own-fg' }
$uncut = if (-not $second) { $ref } elseif ($TestName -and -not $px) { "dlssiv-fg-$TestName" } else { 'dlssiv-fg' }
$name  = if (-not $second) { $ref } elseif ($TestName) { "dlssiv-fg-$TestName" } elseif ($Variant) { "dlssiv-fg-v-$Variant" } else { 'dlssiv-fg' }
$crt  = if ($DynamicCrt) { '/MD' } else { '/MT' }
if (-not $OutDir) {
    $sub = if ($second) { if ($TestName) { 'obj\dlssiv-fg-alt' } else { 'obj\dlssiv-fg' } } elseif ($TestName) { 'obj\ffx-own-alt' } elseif ($DynamicCrt) { 'obj\ffx-own-md' } else { 'obj\ffx-own' }
    $OutDir = Join-Path $root $sub
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$out = (Resolve-Path $OutDir).Path.TrimEnd('\')
# THE REFERENCE'S OWN FOLDER, beside the output folder: only the product's run delivers one (no test, no other set of cuts).
$withRef = $second -and -not $TestName -and -not $cutsGiven
$outRef = "$out-ref"
$refName = 'dlssiv-fg-ref'
# AND THE EXACT LIBRARY'S (2026-10-12): the cuts marked "in" and no picture cut, under a name of its own that is never the
# product's file's, into a folder of its own beside the output folder (<output folder>-exact: obj\dlssiv-fg-exact).
$outExact = "$out-exact"
$exactName = 'dlssiv-fg-exact'
if ($withRef) {
    foreach ($side in $outRef, $outExact) {
        New-Item -ItemType Directory -Force -Path $side | Out-Null
        $amdNamedRef = @(Get-ChildItem $side -File | Where-Object { $_.Name -like 'amd_fidelityfx*' })
        if ($amdNamedRef.Count -gt 0) {
            throw ("the folder of the reference or of the exact library holds a file named like AMD's ($($amdNamedRef[0].Name)): $side. This script does not build into such a folder. " +
                   "Nothing there was deleted and nothing was written.")
        }
    }
}

# FIRST, AND BEFORE ANYTHING IS DELETED: is this a folder with AMD's files in it? Then it is not a build folder of ours.
$amdNamed = @(Get-ChildItem $out -File | Where-Object { $_.Name -like 'amd_fidelityfx*' })
if ($amdNamed.Count -gt 0) {
    throw ("the output folder holds a file named like AMD's ($($amdNamed[0].Name)): $out. This script does not build into such a folder. " +
           "Nothing there was deleted and nothing was written. Give -OutDir a folder of its own.")
}

# THE SHORT COPY (the description says why). k = the kit, s = the 72 headers, g = the glue, o = where it is built.
# Its two refusals come before anything in the output folder is touched.
$stageRoot = if ($env:DLSSIV_FFX_STAGE) { $env:DLSSIV_FFX_STAGE } else { [System.IO.Path]::GetTempPath() }
$stage = Join-Path $stageRoot ('ffxk-' + [System.Guid]::NewGuid().ToString('N').Substring(0, 8))
$longest = 175     # the longest joined include of AMD's files beyond the kit's own path, measured from the compiler's error
if (($stage.Length + 2 + $longest) -ge 260) {
    throw ("the temp folder is too deep for AMD's include chains: $stage\k plus $longest characters is past the 260 Windows allows. " +
           "Set the environment variable DLSSIV_FFX_STAGE to a short folder (for instance C:\t) and build again. Nothing was built and nothing was touched.")
}
foreach ($p in $stage, $vcvars) {
    if ($p -match '[^\x20-\x7E]') {
        throw ("this path has a character outside ASCII: $p. The compiler is given its file names in files written in ASCII, where that character cannot be written, " +
               "and it would look for the source in a folder that does not exist. Set the environment variable DLSSIV_FFX_STAGE to a folder with a plain name " +
               "(for instance C:\t) and build again. Nothing was built and nothing was touched.")
    }
}
$sk = Join-Path $stage 'k'; $ss = Join-Path $stage 's'; $sg = Join-Path $stage 'g'; $so = Join-Path $stage 'o'
$so2 = Join-Path $stage 'p'                                                               # where the second compile is built (the cuts; in the product's run the exact library's)
$so3 = Join-Path $stage 'q'                                                               # where the product's third compile is built (its accepted picture cuts on top)

# THE NAMES THIS SCRIPT OWNS IN ITS OUTPUT FOLDER. What it delivers, and what its older versions left behind
# (they compiled and linked in the output folder itself). Nothing else in the folder is touched, ever.
# FROM HERE ON a failure leaves no library of this script's in the output folder.
$objs = @(($amd + $ours) | ForEach-Object { [System.IO.Path]::GetFileNameWithoutExtension($_) + '.obj' })
# (the second library's own objects: nine of AMD's, the swapchain's two stubs, and its interface)
$objsFg = @(($amdFg + @('own_no_swapchain.cpp', $oursFg)) | ForEach-Object { [System.IO.Path]::GetFileNameWithoutExtension($_) + '.obj' })
$delivered = @("$name.map", 'build.log', 'reads.txt', 'toolchain.txt', "$name.dll")      # in the order they arrive: the library last
if ($second) { $delivered = @('objects.txt', 'cuts.txt') + $delivered }                 # the sibling's and the objects' SHA-256; the cuts that are in
if ($TestName) { $delivered = @("$name.txt") + $delivered }                              # what was altered, beside the test library
$deliveredRef = @('objects.txt', "$refName.map", 'build.log', 'reads.txt', 'toolchain.txt', "$refName.dll")   # the reference's six, the library last
$deliveredExact = @('objects.txt', 'cuts.txt', "$exactName.map", 'build.log', 'reads.txt', 'toolchain.txt', "$exactName.dll")   # the exact library's seven, the library last
$legacy = $objs + @("$name.lib", "$name.exp", 'cl.rsp', 'link.rsp', 'run.cmd', 'toolchain.raw')
if ($second) { $legacy += @('dlssiv_fg.obj', 'link-ref.rsp') }
foreach ($f in ($delivered + $legacy)) {
    $p = Join-Path $out $f
    if (Test-Path -LiteralPath $p -PathType Leaf) { Remove-Item -LiteralPath $p -Force }
}
if ($withRef) {
    foreach ($f in $deliveredRef) {
        $p = Join-Path $outRef $f
        if (Test-Path -LiteralPath $p -PathType Leaf) { Remove-Item -LiteralPath $p -Force }
    }
    foreach ($f in $deliveredExact) {
        $p = Join-Path $outExact $f
        if (Test-Path -LiteralPath $p -PathType Leaf) { Remove-Item -LiteralPath $p -Force }
    }
}

# FFX_FRAMEGENERATION: the effect this library is. FFX_BACKEND_DX12=0: DEFINED selects the DX12 headers, the VALUE 0
# leaves out AMD's driver-side provider, whose header is not published (ffx-own\glue\own_providers.cpp).
# /showIncludes is for reads.txt. One compiler at a time (cl ignores /MP beside /showIncludes): about ten seconds.
$cl = @('/nologo', '/c', '/O2', '/Ob2', '/std:c++20', $crt, '/W3', '/EHsc', '/Gy', '/Gw', '/Brepro', '/showIncludes',
        '/DFFX_FRAMEGENERATION', '/DFFX_BACKEND_DX12=0', '/D_WINDOWS', '/DWIN32', '/DNDEBUG', '/DUNICODE', '/D_UNICODE', '/DNOMINMAX',
        "/I`"$sk`"", "/I`"$ss`"", "/I`"$sg`"") + @($amd | ForEach-Object { "`"$sk\$_`"" }) + @($ours | ForEach-Object { "`"$sg\$_`"" })
if ($second) { $cl += "`"$sg\$oursFg`"" }
# THE REFERENCE LIBRARY'S LINK, the same line whichever target is asked for: with -Target dlssiv-fg it is made first, from
# the same objects, and stays in the temp copy (the description says why).
$linkRef = @('/NOLOGO', '/DLL', '/MACHINE:X64', '/Brepro', '/OPT:REF', '/OPT:ICF', '/INCREMENTAL:NO',
             "/OUT:$ref.dll", "/IMPLIB:$ref.lib", "/MAP:$ref.map") + $objs + @('dxgi.lib', 'dxguid.lib')
$link = $linkRef
if ($second) {
    $link = @('/NOLOGO', '/DLL', '/MACHINE:X64', '/Brepro', '/OPT:REF', '/OPT:ICF', '/INCREMENTAL:NO',
              "/OUT:$uncut.dll", "/IMPLIB:$uncut.lib", "/MAP:$uncut.map") + $objsFg + @('dxgi.lib', 'dxguid.lib')
}
# THE PRODUCT'S SECOND COMPILE (the cuts): every file again, from the copy the cuts were applied to, into a folder of its
# own. THE COMMAND LINE IS THE FIRST COMPILE'S, TOKEN FOR TOKEN, the files it names too: nothing but the cuts differs
# between the two compiles. (An object's own bytes do not say which objects a cut reached: measured 2026-10-10, every
# object of two runs of the SAME compile differs, while the linked library is the same bytes every time. So objects.txt
# lists hashes and marks nothing.) What tells our glue that it is the product's compile is the file
# dlssiv_fg_cuts.h, which tools\ffx-own\cuts.py has by then written into the copy of the glue.
$cl2 = $cl
$link2 = @('/NOLOGO', '/DLL', '/MACHINE:X64', '/Brepro', '/OPT:REF', '/OPT:ICF', '/INCREMENTAL:NO',
           "/OUT:$name.dll", "/IMPLIB:$name.lib", "/MAP:$name.map") + $objsFg + @('dxgi.lib', 'dxguid.lib')
$linkRefStep = if ($second) { 'link @link-ref.rsp >> build.log 2>&1' } else { 'rem one library, one link' }
$ascii = [System.Text.Encoding]::ASCII
$runText = @(
    '@echo off',
    "call `"$vcvars`" >nul 2>&1",
    'if errorlevel 1 exit /b 91',
    'set CL=',
    'set _CL_=',
    'set LINK=',
    'set _LINK_=',
    "cd /d `"$so`"",
    'if errorlevel 1 exit /b 92',
    'echo windows-sdk %WindowsSDKVersion%> toolchain.raw',
    'echo vc-dir %VCToolsInstallDir%>> toolchain.raw',
    'echo sdk-dir %WindowsSdkDir%>> toolchain.raw',
    'cl 2>&1 | findstr /C:"Version" >> toolchain.raw',
    'link 2>&1 | findstr /C:"Version" >> toolchain.raw',
    'cl @cl.rsp > build.log 2>&1',
    'if errorlevel 1 exit /b 93',
    $linkRefStep,
    'if errorlevel 1 exit /b 94',
    'link @link.rsp >> build.log 2>&1',
    'if errorlevel 1 exit /b 94',
    'exit /b 0'
) -join "`r`n"
# THE SECOND COMPILE'S COMMANDS: the first's own head (the compiler's environment, cleared of the four variables by the
# very same lines: they are written once in this script), then its own folder, compiler and linker.
$runHead = @(); foreach ($l in ($runText -split "`r`n")) { if ($l -like 'cd /d*') { break }; $runHead += $l }
$run2Text = ($runHead + @(
    "cd /d `"$so2`"",
    'if errorlevel 1 exit /b 92',
    'cl @cl.rsp > build.log 2>&1',
    'if errorlevel 1 exit /b 93',
    'link @link.rsp >> build.log 2>&1',
    'if errorlevel 1 exit /b 94',
    'exit /b 0'
)) -join "`r`n"

Write-Host "Building $name.dll (x64, $crt) at BelowNormal priority..." -ForegroundColor Cyan
if ($second) { Write-Host "  THE SECOND LIBRARY: the mod's own interface over the objects of AMD's two effects and backend; $ref.dll is linked from the same objects first and is not delivered" -ForegroundColor DarkGray }
if ($px) { Write-Host "  A TEST LIBRARY, NEVER THE PRODUCT: the exact library's cuts and the one-pixel control (ffx-own\cuts\control-one-pixel.cut). It must fail the picture compare." -ForegroundColor Yellow }
elseif ($TestName) { Write-Host "  A TEST LIBRARY, NEVER THE PRODUCT: built from an altered copy of the source (ffx-own\ALTERED.txt). It must fail the picture compare." -ForegroundColor Yellow }
if ($cutsGiven) { Write-Host "  ANOTHER SET OF CUTS THAN THE PRODUCT'S (-Cuts $Cuts): an experiment, never the product" -ForegroundColor Yellow }
if ($Variant) { Write-Host "  A VARIANT LIBRARY, NEVER THE PRODUCT: it CHANGES THE PICTURE (-Variant $Variant), for the owner's eye" -ForegroundColor Yellow }
foreach ($n in 'CL', '_CL_', 'LINK', '_LINK_') {
    $v = [System.Environment]::GetEnvironmentVariable($n)
    if ($v) { Write-Host "  $n was set in the environment ('$v'): cleared for this build's compiler and linker, which take no option from it" -ForegroundColor Yellow }
}
$me = Get-Process -Id $PID
$was = $me.PriorityClass
$global:LASTEXITCODE = 0
try {
    $me.PriorityClass = 'BelowNormal'
    New-Item -ItemType Directory -Path $stage | Out-Null
    New-Item -ItemType Directory -Path $so | Out-Null
    Copy-Item -Path $kit     -Destination $sk -Recurse
    Copy-Item -Path $shaders -Destination $ss -Recurse
    Copy-Item -Path $glue    -Destination $sg -Recurse
    [System.IO.File]::WriteAllText((Join-Path $so 'cl.rsp'),   ($cl   -join "`r`n") + "`r`n", $ascii)
    [System.IO.File]::WriteAllText((Join-Path $so 'link.rsp'), ($link -join "`r`n") + "`r`n", $ascii)
    if ($second) { [System.IO.File]::WriteAllText((Join-Path $so 'link-ref.rsp'), ($linkRef -join "`r`n") + "`r`n", $ascii) }
    $run = Join-Path $so 'run.cmd'
    [System.IO.File]::WriteAllText($run, $runText + "`r`n", $ascii)
    & $run
    $rc = $LASTEXITCODE

    # EVERY CHECK IS MADE HERE, ON THE COPY. Only build.log leaves it before the last one has passed.
    $log = @()
    $logPath = Join-Path $so 'build.log'
    if (Test-Path $logPath) { $log = @(Get-Content $logPath); Copy-Item -LiteralPath $logPath -Destination (Join-Path $out 'build.log') -Force }
    $log | Where-Object { $_ -notmatch '^Note: including file:' } | ForEach-Object { Write-Host $_ }
    if ($rc -ne 0) {
        $why = switch ($rc) { 91 { 'vcvars64.bat failed' } 92 { 'the build folder could not be entered' } 93 { 'the compiler failed' } 94 { 'the linker failed' } default { "exit code $rc" } }
        throw "Build failed: $why. No library was delivered."
    }
    $warn = @($log | Where-Object { $_ -match '\bwarning\s+(C|LNK|D)\d+' })
    if ($warn.Count -gt 0) { throw "Build failed: $($warn.Count) warning(s), and this build takes none. No library was delivered. The first: $($warn[0])" }
    $built = Join-Path $so "$uncut.dll"
    if (-not (Test-Path $built)) { throw "Build failed: the linker left no $uncut.dll." }

    $fs = [System.IO.File]::OpenRead($built)
    try {
        $br = New-Object System.IO.BinaryReader($fs)
        $fs.Position = 0x3C
        $peOff = $br.ReadInt32()
        $fs.Position = $peOff + 4
        $machine = $br.ReadUInt16()
        $fs.Position = $peOff + 4 + 20 + 70
        $dllChar = $br.ReadUInt16()
    } finally { $fs.Dispose() }
    if ($machine -ne 0x8664) { throw ("Build failed: NOT a 64-bit image (machine 0x{0:X4}). No library was delivered." -f $machine) }

    # toolchain.txt: the versions of the compiler, the linker and the Windows SDK that made these bytes, one key a
    # line, for the proof to hold against tools\ffx-own\RECORD.txt. Read from the two programs' own banners (the first
    # thing that looks like a version on the line, so a translated banner reads the same).
    $tool = [ordered]@{ 'cl' = '?'; 'link' = '?'; 'windows-sdk' = '?' }
    $vcDir = ''; $sdkDir = ''      # where the compiler's own headers and the Windows SDK's are: the only other folders it may read
    $rawPath = Join-Path $so 'toolchain.raw'
    if (Test-Path $rawPath) {
        foreach ($line in @(Get-Content $rawPath)) {
            if ($line -match '^vc-dir (.+)$')  { $vcDir  = $Matches[1].Trim(); continue }
            if ($line -match '^sdk-dir (.+)$') { $sdkDir = $Matches[1].Trim(); continue }
            if ($line -notmatch '(\d+\.\d+\.\d+(\.\d+)?)') { continue }
            $ver = $Matches[1]
            if ($line -match '^windows-sdk ') { $tool['windows-sdk'] = $ver }
            elseif ($line -match 'Linker|LINK') { $tool['link'] = $ver }
            elseif ($tool['cl'] -eq '?') { $tool['cl'] = $ver }
        }
    }
    if ($tool.Values -contains '?') { throw "Build failed: the toolchain's versions could not be read (cl $($tool['cl']), link $($tool['link']), Windows SDK $($tool['windows-sdk'])). No library was delivered." }
    [System.IO.File]::WriteAllText((Join-Path $so 'toolchain.txt'), (($tool.Keys | ForEach-Object { "$_ $($tool[$_])" }) -join "`n") + "`n", $ascii)

    # reads.txt: every file of AMD's the compiler opened, relative to the kit, with the git blob id of the bytes it
    # opened ("id path"), for the proof to hold against the manifest. And every OTHER file it opened must be in this
    # build's copy (the 72 headers, the glue), among the compiler's own headers or in the Windows SDK.
    if (-not $vcDir -or -not $sdkDir) { throw "Build failed: the folders of the compiler's headers and of the Windows SDK could not be read, so what the compiler opened cannot be checked. No library was delivered." }
    $kitFull = [System.IO.Path]::GetFullPath($sk).TrimEnd('\') + '\'
    $roots = @($stage, $vcDir, $sdkDir) | ForEach-Object { [System.IO.Path]::GetFullPath($_).TrimEnd('\') + '\' }
    $opened = New-Object 'System.Collections.Generic.List[string]'
    foreach ($s in $amd) { $opened.Add((Join-Path $sk $s)) }
    foreach ($line in $log) {
        if ($line -match '^Note: including file:\s+(.+)$') {
            $p = [System.IO.Path]::GetFullPath($Matches[1].Trim())
            if (@($roots | Where-Object { $p.StartsWith($_, [System.StringComparison]::OrdinalIgnoreCase) }).Count -eq 0) {
                throw ("Build failed: the compiler opened $p, which is neither in this build's copy of the source nor a file of the compiler or of the Windows SDK. " +
                       "Something put another folder on its include path (the INCLUDE variable?). No library was delivered.")
            }
            $opened.Add($p)
        }
    }
    $reads = New-Object 'System.Collections.Generic.SortedDictionary[string,string]' ([System.StringComparer]::Ordinal)
    foreach ($p in $opened) {
        if ($p.StartsWith($kitFull, [System.StringComparison]::OrdinalIgnoreCase)) {
            $rel = $p.Substring($kitFull.Length).Replace('\', '/')
            if (-not $reads.ContainsKey($rel)) { $reads[$rel] = Get-BlobId $p }
        }
    }
    [System.IO.File]::WriteAllText((Join-Path $so 'reads.txt'), (($reads.Keys | ForEach-Object { "$($reads[$_]) $_" }) -join "`n") + "`n", $ascii)

    # THE CUTS (stage 3): applied to this copy NOW, after the uncut library is linked from it and after every file of
    # AMD's it was compiled from has been hashed (reads.txt above is the uncut compile's). Then the second compile.
    $cutCount = 0
    $exactCount = 0
    $sb = $so; $builtName = $uncut            # where the library this run delivers was linked, and under which name
    $sbExact = $so; $exactBuilt = $uncut      # and the exact library of the product's run (with no exact cut it is the uncut library)
    if ($second) {
        Copy-Item -LiteralPath (Join-Path $so 'build.log') -Destination (Join-Path $so 'build-uncut.log')      # the reference's log: the uncut compile and its two links
        # ONE STAGE, OR TWO (2026-10-12). Every run but the product's stages its set once, as before. THE PRODUCT'S RUN STAGES
        # TWICE ON THE SAME COPY: first the exact library's cuts (-Cuts exact: the cuts marked "in"), compiled and linked
        # exactly as the product was until that day, so that its bytes are the ones the judge proved; then the accepted
        # picture cuts on top of them (tools\ffx-own\cuts.py stage --after), compiled and linked again: the product.
        $rounds = @(@{ dir = $so2; spec = $(if ($withRef) { 'exact' } else { $cutSpec }); after = ''; title = 'THE SECOND COMPILE: THE CUTS' })
        if ($withRef) { $rounds += @{ dir = $so3; spec = ''; after = $so2; title = 'THE THIRD COMPILE: THE ACCEPTED PICTURE CUTS ON TOP' } }
        $roundNo = 0
        foreach ($round in $rounds) {
            $roundNo++
            $sd = $round.dir
            New-Item -ItemType Directory -Path $sd | Out-Null
            $stageArgs = @($cutsPy, 'stage', '--root', $root, '--kit', $sk, '--headers', $ss, '--glue', $sg, '--out', $sd)
            if ($round.spec -ne '') { $stageArgs += @('--cuts', $round.spec) }
            if ($round.after) { $stageArgs += @('--after', $round.after) }
            if ($Variant) { $stageArgs += @('--variant', $Variant) }
            $said = @(& $python @stageArgs)
            $rcCuts = $LASTEXITCODE
            $said | ForEach-Object { Write-Host "  $_" }
            $cutsTxt = Join-Path $sd 'cuts.txt'
            if ($rcCuts -ne 0 -or -not (Test-Path $cutsTxt)) { throw "Build failed: the cuts could not be applied (tools\ffx-own\cuts.py stage, exit code $rcCuts): $($said -join ' '). No library was delivered." }
            if (@(Get-Content $cutsTxt)[0] -notmatch '^cuts (\d+)$') { throw "Build failed: cuts.txt does not begin with the count of cuts. No library was delivered." }
            $cutsBefore = $cutCount
            $cutCount = [int]$Matches[1]
            if ($px -and $cutCount -lt 1) { throw "Build failed: the one-pixel control was not applied. No library was delivered." }
            if ($cutCount -gt $cutsBefore) {
                [System.IO.File]::WriteAllText((Join-Path $sd 'cl.rsp'),   ($cl2   -join "`r`n") + "`r`n", $ascii)
                [System.IO.File]::WriteAllText((Join-Path $sd 'link.rsp'), ($link2 -join "`r`n") + "`r`n", $ascii)
                $run2 = Join-Path $sd 'run.cmd'
                [System.IO.File]::WriteAllText($run2, $run2Text.Replace("cd /d `"$so2`"", "cd /d `"$sd`"") + "`r`n", $ascii)
                & $run2
                $rc2 = $LASTEXITCODE
                $log2 = @()
                $log2Path = Join-Path $sd 'build.log'
                if (Test-Path $log2Path) {
                    $log2 = @(Get-Content $log2Path)
                    [System.IO.File]::AppendAllText((Join-Path $so 'build.log'), "`r`n---- $($round.title) ($cutCount) ----`r`n" + ($log2 -join "`r`n") + "`r`n", $ascii)
                    Copy-Item -LiteralPath (Join-Path $so 'build.log') -Destination (Join-Path $out 'build.log') -Force
                }
                $log2 | Where-Object { $_ -notmatch '^Note: including file:' } | ForEach-Object { Write-Host $_ }
                if ($rc2 -ne 0) {
                    $why = switch ($rc2) { 91 { 'vcvars64.bat failed' } 92 { 'the build folder could not be entered' } 93 { 'the compiler failed' } 94 { 'the linker failed' } default { "exit code $rc2" } }
                    throw "Build failed, compile $($roundNo + 1) (the cuts): $why. No library was delivered."
                }
                $warn = @($log2 | Where-Object { $_ -match '\bwarning\s+(C|LNK|D)\d+' })
                if ($warn.Count -gt 0) { throw "Build failed, compile $($roundNo + 1) (the cuts): $($warn.Count) warning(s), and this build takes none. No library was delivered. The first: $($warn[0])" }
                foreach ($line in $log2) {
                    if ($line -match '^Note: including file:\s+(.+)$') {
                        $p = [System.IO.Path]::GetFullPath($Matches[1].Trim())
                        if (@($roots | Where-Object { $p.StartsWith($_, [System.StringComparison]::OrdinalIgnoreCase) }).Count -eq 0) {
                            throw ("Build failed, compile $($roundNo + 1) (the cuts): the compiler opened $p, which is neither in this build's copy of the source nor a file of the compiler or of the Windows SDK. No library was delivered.")
                        }
                    }
                }
                $sb = $sd; $builtName = $name
                $built = Join-Path $sb "$builtName.dll"
                if (-not (Test-Path $built)) { throw "Build failed: link $($roundNo + 1) left no $builtName.dll." }
                $fs = [System.IO.File]::OpenRead($built)
                try {
                    $br = New-Object System.IO.BinaryReader($fs)
                    $fs.Position = 0x3C
                    $peOff = $br.ReadInt32()
                    $fs.Position = $peOff + 4
                    $machine = $br.ReadUInt16()
                    $fs.Position = $peOff + 4 + 20 + 70
                    $dllChar = $br.ReadUInt16()
                } finally { $fs.Dispose() }
                if ($machine -ne 0x8664) { throw ("Build failed: the library with the cuts is NOT a 64-bit image (machine 0x{0:X4}). No library was delivered." -f $machine) }
            }
            if ($withRef -and $roundNo -eq 1) {
                $sbExact = $sb; $exactBuilt = $builtName; $exactCount = $cutCount
                Copy-Item -LiteralPath (Join-Path $so 'build.log') -Destination (Join-Path $so 'build-exact.log')   # the exact library's log: the uncut compile and the exact cuts'
            }
        }
    }

    if ($px) {
        $note = @("$name.dll: A TEST LIBRARY, NEVER THE PRODUCT. It must fail the picture compare.",
                  "Made by build-ffx-own.ps1 -Target dlssiv-fg -TestName px: the exact library's cuts and, on top of them, the cut control-one-pixel",
                  "(ffx-own\cuts\control-one-pixel.cut): the red of the one pixel (1001, 601) of every generated image is changed in the last pass.",
                  "cuts.txt beside it lists every cut that is in it.")
        [System.IO.File]::WriteAllText((Join-Path $so "$name.txt"), ($note -join "`r`n") + "`r`n", $ascii)
    } elseif ($TestName) { Copy-Item -LiteralPath $altered -Destination (Join-Path $so "$name.txt") }
    if ($second) {
        # objects.txt: the reference library these very objects link to (it stays here), and every object the second library
        # was linked from, each with its SHA-256. "sibling <sha256> <bytes> <name>", then "object <sha256> <name>" a line.
        # (That is the UNCUT library's list, as stage 2 wrote it: the reference's folder gets it.)
        $sib = Join-Path $so "$ref.dll"
        if (-not (Test-Path $sib)) { throw "Build failed: the linker left no $ref.dll for the second library's objects to be held to. No library was delivered." }
        $sibLine = "sibling $((Get-FileHash $sib -Algorithm SHA256).Hash.ToLower()) $((Get-Item $sib).Length) $ref.dll"
        $lines = @($sibLine)
        $uncutSha = @{}
        foreach ($o in $objsFg) { $uncutSha[$o] = (Get-FileHash (Join-Path $so $o) -Algorithm SHA256).Hash.ToLower(); $lines += "object $($uncutSha[$o]) $o" }
        [System.IO.File]::WriteAllText((Join-Path $so 'objects.txt'), ($lines -join "`n") + "`n", $ascii)
        # THE DELIVERED LIBRARY'S LIST: the same sibling; "reference <sha256> <bytes> <name>", the uncut second library the
        # same run linked (the reference: stage 2's record holds it); and its own objects.
        $unc = Join-Path $so "$uncut.dll"
        $refLine = "reference $((Get-FileHash $unc -Algorithm SHA256).Hash.ToLower()) $((Get-Item $unc).Length) $uncut.dll"
        $lines = @($sibLine, $refLine)
        # (the product's run: THE EXACT LIBRARY'S LIST first, the same two lines and its own objects; and the product's list names
        # the exact library it was built on top of, "exact <sha256> <bytes> <name>": the third record holds it)
        if ($withRef) {
            $linesExact = @($sibLine, $refLine)
            foreach ($o in $objsFg) { $linesExact += "object $((Get-FileHash (Join-Path $sbExact $o) -Algorithm SHA256).Hash.ToLower()) $o" }
            [System.IO.File]::WriteAllText((Join-Path $so2 'objects.txt'), ($linesExact -join "`n") + "`n", $ascii)
            $ex = Join-Path $sbExact "$exactBuilt.dll"
            $lines += "exact $((Get-FileHash $ex -Algorithm SHA256).Hash.ToLower()) $((Get-Item $ex).Length) $exactName.dll"
        }
        foreach ($o in $objsFg) {
            $h = (Get-FileHash (Join-Path $sb $o) -Algorithm SHA256).Hash.ToLower()
            $lines += "object $h $o"
        }
        $listDir = if ($withRef) { $so3 } else { $so2 }                                        # the folder of this run's last stage: its cuts.txt is the delivered library's
        [System.IO.File]::WriteAllText((Join-Path $listDir 'objects.txt'), ($lines -join "`n") + "`n", $ascii)
    }
    # THE LAST CHECK HAS PASSED: deliver, the library last. First the reference, into its own folder (the product's run only).
    if ($withRef) {
        $fromRef = @{ 'objects.txt' = (Join-Path $so 'objects.txt'); "$refName.map" = (Join-Path $so "$uncut.map"); 'build.log' = (Join-Path $so 'build-uncut.log');
                      'reads.txt' = (Join-Path $so 'reads.txt'); 'toolchain.txt' = (Join-Path $so 'toolchain.txt'); "$refName.dll" = (Join-Path $so "$uncut.dll") }
        foreach ($f in $deliveredRef) { Copy-Item -LiteralPath $fromRef[$f] -Destination (Join-Path $outRef $f) -Force }
        # then the exact library, into its own: the bytes the second link made (they call themselves dlssiv-fg.dll inside, as
        # the product did while it was this library), under the exact library's own FILE name
        $fromExact = @{ 'objects.txt' = (Join-Path $so2 'objects.txt'); 'cuts.txt' = (Join-Path $so2 'cuts.txt'); "$exactName.map" = (Join-Path $sbExact "$exactBuilt.map"); 'build.log' = (Join-Path $so 'build-exact.log');
                        'reads.txt' = (Join-Path $so 'reads.txt'); 'toolchain.txt' = (Join-Path $so 'toolchain.txt'); "$exactName.dll" = (Join-Path $sbExact "$exactBuilt.dll") }
        foreach ($f in $deliveredExact) { Copy-Item -LiteralPath $fromExact[$f] -Destination (Join-Path $outExact $f) -Force }
    }
    $from = @{ "$name.map" = (Join-Path $sb "$builtName.map"); "$name.dll" = $built; 'build.log' = (Join-Path $so 'build.log') }
    if ($second) { $from['objects.txt'] = (Join-Path $listDir 'objects.txt'); $from['cuts.txt'] = (Join-Path $listDir 'cuts.txt') }
    foreach ($f in $delivered) {
        $src = if ($from.ContainsKey($f)) { $from[$f] } else { Join-Path $so $f }
        Copy-Item -LiteralPath $src -Destination (Join-Path $out $f) -Force
    }
} finally {
    $me.PriorityClass = $was
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
}

$dll = Join-Path $out "$name.dll"
$info = Get-Item $dll
Write-Host ("  {0:N0} bytes  {1}" -f $info.Length, $dll) -ForegroundColor Green
Write-Host ("  SHA-256 {0}" -f (Get-FileHash $dll -Algorithm SHA256).Hash.ToLower())
Write-Host ("  toolchain: cl {0}, link {1}, Windows SDK {2} (toolchain.txt)" -f $tool['cl'], $tool['link'], $tool['windows-sdk'])
Write-Host ("  {0} of AMD's files read by the compiler, each with the git blob id of what it read (reads.txt)" -f $reads.Count) -ForegroundColor DarkGray
if ($second) {
    Write-Host ("  the cuts in it: {0} (cuts.txt beside it names them and what each changed){1}" -f $cutCount, $(if ($cutCount -eq 0) { ': NONE, it is the uncut library byte for byte' } else { '' })) -ForegroundColor $(if ($cutCount -gt 0) { 'Green' } else { 'DarkGray' })
}
if ($withRef) {
    $r = Join-Path $outRef "$refName.dll"
    Write-Host ("  THE REFERENCE (no cut, stage 2's library under a reference name): {0:N0} bytes  {1}" -f (Get-Item $r).Length, $r) -ForegroundColor Green
    Write-Host ("  SHA-256 {0}" -f (Get-FileHash $r -Algorithm SHA256).Hash.ToLower())
    $e = Join-Path $outExact "$exactName.dll"
    Write-Host ("  THE EXACT LIBRARY (the {0} exact cut(s) and no picture cut: what the judge holds to the reference): {1:N0} bytes  {2}" -f $exactCount, (Get-Item $e).Length, $e) -ForegroundColor Green
    Write-Host ("  SHA-256 {0}" -f (Get-FileHash $e -Algorithm SHA256).Hash.ToLower())
}
Write-Host "  machine 0x8664 (x64)" -ForegroundColor Green
Write-Host ('  DllCharacteristics 0x{0:X4}: DYNAMICBASE {1}, NX_COMPAT {2}, HIGH_ENTROPY_VA {3}, GUARD_CF {4}' -f $dllChar,
    [bool]($dllChar -band 0x40), [bool]($dllChar -band 0x100), [bool]($dllChar -band 0x20), [bool]($dllChar -band 0x4000))
