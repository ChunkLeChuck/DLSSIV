<#
.SYNOPSIS
    Compiles frame generation's own shaders (source\fg\shaders) to their checked-in headers, or with -Verify checks that
    every checked-in header is what its .hlsl compiles to today.

.DESCRIPTION
    tools\compile-shaders.ps1 owns source\shaders and asks that every .hlsl in THAT folder is in its list. These live
    in a folder of their own because they belong to frame generation's armed product alone (DLSSIV_FG; written for a build
    that did not ship yet):
    the HUD-less picture for frame generation at hdr=1 (four variants of hdr_encode_ps.hlsl compiled with HDR_FGHUD: the
    plain and the absorbed frame, each sharpening or not) and the check that its copy point is right. Since 2026-10-11
    four more: the real encode itself with that picture as its second output (HDR_FGMRT), the form that runs; the four
    of HDR_FGHUD are the form of before, kept for an A/B. Since 2026-10-12 four more of HDR_FGMRT, with HDR_FGOV: the
    same encode for a frame with a 3D overlay ahead of the interface (the phone), which reads a second copy.

    THE TWELVE VARIANTS INCLUDE source\shaders\hdr_encode_ps.hlsl AND hdr_common.hlsli: an edit of either makes these
    headers stale, and build.ps1's timestamp gate does not look in this folder. -Verify is the proof; tools\test-fg-arms
    runs it before it compiles the armed product.

        .\tools\compile-fg-shaders.ps1            regenerate the thirteen headers
        .\tools\compile-fg-shaders.ps1 -Verify    recompile into a temp folder and compare; exit 1 on any mismatch
#>
param([switch]$Verify)

$ErrorActionPreference = 'Stop'
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
$dir = Join-Path $PSScriptRoot '..\source\fg\shaders'

# All of them with /O3, as the encodes they are made from.
$list = @(
    # (2026-10-11) the real encode with the HUD-less picture as its second output, HDR_FGMRT: the same four variants
    @{ src = 'hdr_fgmrt_ps';         var = 'kHdrFgMrtPS'       },
    @{ src = 'hdr_fgmrt_cga_ps';     var = 'kHdrFgMrtCgaPS'    },
    @{ src = 'hdr_fgmrt_g10_ps';     var = 'kHdrFgMrtG10PS'    },
    @{ src = 'hdr_fgmrt_cga_g10_ps'; var = 'kHdrFgMrtCgaG10PS' },
    # (2026-10-12) the same four for a frame with a 3D overlay ahead of the interface (the phone), HDR_FGMRT with HDR_FGOV
    @{ src = 'hdr_fgmrt_ov_ps';         var = 'kHdrFgMrtOvPS'       },
    @{ src = 'hdr_fgmrt_ov_cga_ps';     var = 'kHdrFgMrtOvCgaPS'    },
    @{ src = 'hdr_fgmrt_ov_g10_ps';     var = 'kHdrFgMrtOvG10PS'    },
    @{ src = 'hdr_fgmrt_ov_cga_g10_ps'; var = 'kHdrFgMrtOvCgaG10PS' }
)
$known = $list | ForEach-Object { $_.src }
$missing = Get-ChildItem "$dir\*.hlsl" | Where-Object { $known -notcontains $_.BaseName } | ForEach-Object { $_.BaseName }
if ($missing) { throw ("these shaders have no entry in compile-fg-shaders.ps1: " + ($missing -join ', ')) }

$tmp = $null
if ($Verify) {
    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("fgshaderverify-" + [System.Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $tmp | Out-Null
}
$bad = 0
foreach ($s in $list) {
    $in  = Join-Path $dir ($s.src + '.hlsl')
    $hdr = Join-Path $dir ($s.src + '.h')
    $out = if ($Verify) { Join-Path $tmp ($s.src + '.h') } else { $hdr }
    $a = @('/nologo', '/T', 'ps_3_0', '/E', 'main', '/O3', '/Fh', $out, '/Vn', $s.var, $in)
    & $fxc @a | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "fxc failed on $($s.src).hlsl" }
    $line = (Select-String -Path $out -Pattern 'approximately' | Select-Object -First 1)
    $note = if ($line) { $line.Line.Trim() } else { '' }
    if ($Verify) {
        if (-not (Test-Path $hdr)) { Write-Host ("  {0,-22} MISSING HEADER" -f $s.src); $bad++; continue }
        $a1 = (Get-FileHash $hdr -Algorithm SHA256).Hash
        $b1 = (Get-FileHash $out -Algorithm SHA256).Hash
        if ($a1 -eq $b1) { Write-Host ("  {0,-22} ok    {1}" -f $s.src, $note) }
        else { Write-Host ("  {0,-22} STALE: the committed header is not what this .hlsl compiles to" -f $s.src); $bad++ }
    } else {
        Write-Host ("  {0,-22} {1}" -f $s.src, $note)
    }
}
if ($Verify) {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    if ($bad) { Write-Host "$bad of $($list.Count) frame generation shader header(s) are stale"; exit 1 }
    Write-Host "all $($list.Count) frame generation shader headers match their source"
}
exit 0
