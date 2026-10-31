<#
    A REAL DATE IN THE PE, WITHOUT GIVING UP A REPRODUCIBLE BUILD (2026-09-21).

    /Brepro makes one commit give one binary by writing a hash of the output where the linker would write the clock.
    Read as a date, that hash is anywhere in 1970-2106: the 1.0.4 helper's said 2058-10-03, and VirusTotal showed it as
    the compilation time the day Microsoft's machine-learning engine flagged the file (Trojan:Win32/Wacatac.B!ml, 2 of
    71 engines). A date decades in the future is one of the anomalies such models weigh. This writes the COMMIT'S time
    instead, which is as deterministic as the hash was (the same commit gives the same bytes), into every place the
    linker put one: the COFF header, the debug directory entries, the export directory, and any non-zero resource
    directory. Nothing reads these at run time; they are metadata.

    Called by build.ps1 and build-host.ps1 after a successful link, with -Stamp from `git log -1 --format=%ct`.
#>
param([Parameter(Mandatory = $true)][string]$Path,
      [Parameter(Mandatory = $true)][uint32]$Stamp)
$ErrorActionPreference = 'Stop'

$b = [IO.File]::ReadAllBytes($Path)
function U16([int]$o) { [BitConverter]::ToUInt16($b, $o) }
function U32([int]$o) { [BitConverter]::ToUInt32($b, $o) }
function Put32([int]$o, [uint32]$v) { [Array]::Copy([BitConverter]::GetBytes($v), 0, $b, $o, 4) }

$pe = [BitConverter]::ToInt32($b, 0x3C)
if ((U32 $pe) -ne 0x4550) { throw "not a PE file: $Path" }
$nSec = U16 ($pe + 6); $optSize = U16 ($pe + 20); $opt = $pe + 24
$magic = U16 $opt
$dd = if ($magic -eq 0x20B) { $opt + 112 } elseif ($magic -eq 0x10B) { $opt + 96 } else { throw "unknown optional header 0x$('{0:X}' -f $magic): $Path" }
$secTab = $opt + $optSize

function Off([uint32]$rva) {
    for ($i = 0; $i -lt $nSec; $i++) {
        $s = $secTab + 40 * $i
        $va = U32 ($s + 12); $size = [Math]::Max((U32 ($s + 8)), (U32 ($s + 16)))
        if ($rva -ge $va -and $rva -lt $va + $size) { return [int]((U32 ($s + 20)) + ($rva - $va)) }
    }
    throw ("RVA 0x{0:X} is in no section: {1}" -f $rva, $Path)
}

$said = @()
Put32 ($pe + 8) $Stamp; $said += 'COFF header'

$expRva = U32 ($dd + 0 * 8)
if ($expRva) { Put32 ((Off $expRva) + 4) $Stamp; $said += 'export directory' }

$resRva = U32 ($dd + 2 * 8)
if ($resRva) {
    $base = Off $resRva; $n = 0
    $todo = New-Object System.Collections.Generic.Queue[int]; $todo.Enqueue(0)
    while ($todo.Count) {
        $d = $base + $todo.Dequeue()
        if ((U32 ($d + 4)) -ne 0) { Put32 ($d + 4) $Stamp; $n++ }
        $count = (U16 ($d + 12)) + (U16 ($d + 14))
        for ($k = 0; $k -lt $count; $k++) {
            $to = U32 ($d + 16 + 8 * $k + 4)
            if ($to -band 0x80000000) { $todo.Enqueue([int]($to -band 0x7FFFFFFF)) }
        }
    }
    if ($n) { $said += "$n resource director$(if ($n -eq 1) { 'y' } else { 'ies' })" }
}

$dbgRva = U32 ($dd + 6 * 8); $dbgSize = U32 ($dd + 6 * 8 + 4)
if ($dbgRva) {
    $o = Off $dbgRva; $n = [int]($dbgSize / 28)
    for ($k = 0; $k -lt $n; $k++) { Put32 ($o + 28 * $k + 4) $Stamp }
    $said += "$n debug directory entries"
}

[IO.File]::WriteAllBytes($Path, $b)
Write-Host ("  PE date: {0:yyyy-MM-dd HH:mm} UTC, the commit's (in the {1})" -f ([DateTime]'1970-01-01').AddSeconds($Stamp), ($said -join ', ')) -ForegroundColor DarkGray
