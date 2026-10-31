<#
.SYNOPSIS
    Builds ffx-own-probe.exe and its control, ffx-own-probe-control.dll (x64), for tools\prove-ffx-own.py.

.DESCRIPTION
    The probe (probe.cpp) loads one ffx-api library by its full path and asks it for its versions, with no device.
    The control (probe_control.cpp) is a library of ours that answers another version: the probe must fail on it.
    Both files say more at their heads.

    Both are compiled against AMD's public headers in external\ffx\ffx_api_2.3.0, /MT, at BelowNormal priority, into
    -OutDir (default obj\ffx-own beside build-ffx-own.ps1, which git ignores). Neither is ever packaged. Visual Studio
    is found as build-host.ps1 finds it. Compiling is CPU work: nothing here creates a Direct3D device.

.EXAMPLE
    .\tools\ffx-own\build-probe.ps1
#>
[CmdletBinding()]
param([string]$OutDir)

$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$root = Split-Path (Split-Path $here -Parent) -Parent

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

$kit = Join-Path $root 'external\ffx\ffx_api_2.3.0'
if (-not $OutDir) { $OutDir = Join-Path $root 'obj\ffx-own' }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$out = (Resolve-Path $OutDir).Path.TrimEnd('\')
foreach ($f in 'ffx-own-probe.exe', 'ffx-own-probe-control.dll', 'probe.obj', 'probe_control.obj', 'probe.log') {
    $p = Join-Path $out $f
    if (Test-Path $p) { Remove-Item -Force $p }
}

$run = Join-Path $out 'run-probe.cmd'
$text = @(
    '@echo off',
    "call `"$vcvars`" >nul 2>&1",
    'if errorlevel 1 exit /b 91',
    "cd /d `"$out`"",
    'if errorlevel 1 exit /b 92',
    "cl /nologo /O2 /MT /EHsc /W3 /Brepro /I`"$kit`" `"$here\probe.cpp`" /Fe:ffx-own-probe.exe /link /SUBSYSTEM:CONSOLE /MACHINE:X64 /Brepro > probe.log 2>&1",
    'if errorlevel 1 exit /b 93',
    "cl /nologo /O2 /MT /EHsc /W3 /Brepro /LD /I`"$kit`" `"$here\probe_control.cpp`" /Fe:ffx-own-probe-control.dll /link /MACHINE:X64 /Brepro /IMPLIB:ffx-own-probe-control.lib >> probe.log 2>&1",
    'if errorlevel 1 exit /b 94',
    'exit /b 0'
) -join "`r`n"
[System.IO.File]::WriteAllText($run, $text + "`r`n", [System.Text.Encoding]::ASCII)

Write-Host 'Building ffx-own-probe.exe and ffx-own-probe-control.dll (x64, /MT) at BelowNormal priority...' -ForegroundColor Cyan
$me = Get-Process -Id $PID
$was = $me.PriorityClass
$global:LASTEXITCODE = 0
$rc = -1
try {
    $me.PriorityClass = 'BelowNormal'
    & $run
    $rc = $LASTEXITCODE
} finally { $me.PriorityClass = $was }
$log = @()
if (Test-Path (Join-Path $out 'probe.log')) { $log = @(Get-Content (Join-Path $out 'probe.log')) }
$log | ForEach-Object { Write-Host $_ }
if ($rc -ne 0) { throw "Build failed (exit code $rc)." }
$warn = @($log | Where-Object { $_ -match '\bwarning\s+(C|LNK|D)\d+' })
if ($warn.Count -gt 0) { throw "Build failed: $($warn.Count) warning(s). The first: $($warn[0])" }
foreach ($f in 'ffx-own-probe.exe', 'ffx-own-probe-control.dll') {
    $p = Join-Path $out $f
    if (-not (Test-Path $p)) { throw "Build failed: no $f." }
    Write-Host ("  {0:N0} bytes  {1}" -f (Get-Item $p).Length, $p) -ForegroundColor Green
}
