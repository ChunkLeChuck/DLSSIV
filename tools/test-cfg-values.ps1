<#
.SYNOPSIS
    Proves IvtValueIsNumber accepts exactly the cfg values the mod can use.

.DESCRIPTION
    The cfg parser refuses a value it cannot read as a number and keeps the
    shipping default instead, because atoi and atof both return 0 for text and
    give no way to tell that from a real 0. `quality=high` used to mean DLAA and
    `sharpness=medium` used to mean sharpening off with its 33 MB never
    allocated, for the whole session. That guard is now the only thing standing
    between a typo and a silently different mod, so it gets a test.

    THE FUNCTION IS EXTRACTED FROM THE SHIPPED HEADER, never copied into this
    file. A test against a copy proves the copy. If the extraction fails this
    script fails loudly rather than testing nothing, which is the failure mode
    this project has hit more than any other.

    Three of these cases are the reason the first implementation was thrown
    away: strtod accepts "0x10" as hexadecimal and accepts "inf" and "nan",
    while atoi turns all three into 0. A guard built on strtod would have passed
    them through to the exact silent zero it exists to prevent.

.EXAMPLE
    .\tools\test-cfg-values.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root   = Split-Path $PSScriptRoot -Parent
$header = Join-Path $root 'source\transport\DlssIvTransport.h'
if (-not (Test-Path $header)) { throw "header not found: $header" }

$src = Get-Content $header -Raw
$m = [regex]::Match($src, '(?s)static bool IvtValueIsNumber\(const char\* v\)\s*\{.*?\r?\n\}')
if (-not $m.Success) {
    throw ("IvtValueIsNumber was not found in $header. It was renamed, removed, " +
           "or reformatted. This script tests the SHIPPED function and refuses " +
           "to pass by testing nothing.")
}
$fn = $m.Value
Write-Host ("  extracted {0} bytes of the real IvtValueIsNumber" -f $fn.Length)

# text after '=' , must the parser accept it?
$cases = @(
    @('1', $true), @('0', $true), @('-1', $true), @('+2', $true),
    @('0.25', $true), @('.5', $true), @('1.', $true), @('1e3', $true), @('2E-1', $true),
    @('1 ', $true), @("1`t", $true),
    # THE MOST IMPORTANT CASES IN THIS LIST. fgets hands back the line ending
    # and IvtParseCfg trims it, but this guard must accept one regardless: if it
    # ever rejected a trailing newline, EVERY key in EVERY cfg would fall back
    # to its default and the whole mod would silently run on compiled defaults.
    # That is the largest silent failure this code can produce, so it is tested
    # rather than reasoned about.
    @("1`n", $true), @("1`r`n", $true), @("0.25`r`n", $true), @("-1`r`n", $true),
    @('1 # a trailing comment', $true), @('1 ; ini style', $true), @('1 // c style', $true),
    @('', $false), @(' ', $false),
    @('high', $false), @('on', $false), @('true', $false), @('yes', $false), @('off', $false),
    @('1abc', $false), @('abc1', $false), @('1.2.3', $false), @('--1', $false),
    @('0x10', $false),          # strtod reads 16; atoi reads 0
    @('inf', $false), @('nan', $false), @('INF', $false),
    @('1e', $false),            # a bare exponent: the value is 1 and 'e' is junk
    @('.', $false), @('-', $false), @('+', $false), @('e5', $false),
    @('1,5', $false)            # a comma decimal, which a European editor writes
)

$body = New-Object System.Text.StringBuilder
[void]$body.AppendLine('#include <stdio.h>')
[void]$body.AppendLine('#include <string.h>')
[void]$body.AppendLine('')
[void]$body.AppendLine($fn)
[void]$body.AppendLine('')
[void]$body.AppendLine('int main() {')
[void]$body.AppendLine('  int bad = 0;')
foreach ($c in $cases) {
    $esc = $c[0].Replace('\', '\\').Replace('"', '\"').Replace("`t", '\t').Replace("`r", '\r').Replace("`n", '\n')
    $want = if ($c[1]) { 'true' } else { 'false' }
    # Built by concatenation, NOT by -f: the C body is full of braces and the
    # format operator reads every one of them as a placeholder.
    [void]$body.AppendLine('  { bool got = IvtValueIsNumber("' + $esc + '"); if (got != ' + $want +
        ') { bad++; printf("  FAIL  \"' + $esc + '\"  got %d want %d\n", (int)got, (int)' + $want + '); } }')
}
[void]$body.AppendLine('  if (bad) { printf("\n%d FAILURE(S)\n", bad); return 1; }')
[void]$body.AppendLine('  printf("  all ' + $cases.Count + ' cases pass\n"); return 0;')
[void]$body.AppendLine('}')

$tmp = Join-Path $env:TEMP 'dlssiv-cfgnum'
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
$cpp = Join-Path $tmp 'cfgnum_test.cpp'
$exe = Join-Path $tmp 'cfgnum_test.exe'
[System.IO.File]::WriteAllText($cpp, $body.ToString())

$vsRoot = 'C:\Program Files\Microsoft Visual Studio\2022\Community'
$vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found at: $vcvars" }
$cmd = "`"$vcvars`" >nul 2>&1 && cl /nologo /EHsc /W3 `"$cpp`" /Fe:`"$exe`" /Fo:`"$tmp\t.obj`""
$out = & cmd.exe /c $cmd 2>&1
if ($LASTEXITCODE -ne 0) { $out | ForEach-Object { Write-Host $_ }; throw "the test program did not compile." }

& $exe
if ($LASTEXITCODE -ne 0) { throw "IvtValueIsNumber does not accept what it should, or accepts what it must not." }
Write-Host "  cfg value parsing: OK" -ForegroundColor Green
