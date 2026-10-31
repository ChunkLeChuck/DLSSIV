# DLSS-IV.cfg

Sits next to `GTAIV.exe`, or beside the .asi. `key=value`, one per line, no
sections. ABSENT MEANS NOTHING RUNS: `IvtReadCfg` sets `ivtCfgMissing` and
`InitThread` returns before installing a single hook, so the file's presence
really is the master switch. That is enforced in code rather than implied by a
default, which matters because `transport` now defaults to 1 (a file that
merely OMITS the key still runs).

The column that matters is **SHIP**: whether the key belongs in the file a
player receives, with a comment, or whether it is a development lever that
should be undocumented (it still works if typed, which is what a developer
needs and a player never needs).

## A VALUE THIS FILE CANNOT READ KEEPS THE SHIPPING DEFAULT

Added 2026-09-09, and it changes what a mistyped cfg does.

Every key here is a number, and every one used to go through `atoi` or `atof`.
Both return 0 for text they cannot read, and 0 is a legal value for most of
these keys, so there was no way to tell a mistake from a choice:

    quality=high        was DLAA
    hdr=on              was HDR off
    sharpness=medium    was sharpening off, AND its 33 MB never allocated,
                        for the whole session, because that gate latches once

Each one looks exactly like a setting that has no effect, which is the hardest
kind of fault to report and the hardest to diagnose from a report.

Now a value that is not a number is refused and the key keeps its default. So is
a key the build does not recognise, which is almost always a typo
(`sharpnes=0.5`). **Neither is silent any more:** at `log=1` and above the log
names each one, right under the line that says which cfg was read.

At `log=0` (what shipped until 1.0.7, and what a cfg kept from then still says), each one is said in `DLSS-IV-WARNINGS.txt` beside the game. The BEHAVIOUR is correct
either way: you get the shipping default rather than a silent zero.

**What counts as a number** is what `atoi` and `atof` actually mean by one:
`1`, `-1`, `+2`, `0.25`, `.5`, `1e3`, with a trailing comment allowed. Not
`0x10`, not `inf`, not `1,5` with a comma. **An integer key takes a whole number in any of those spellings** (the final
audit, 2026-09-30): `1e3` is 1000 and `2.0` is 2, while `0.5` or `3e9` on an integer key keeps the default and is said (it
was cut to a different number, sometimes silently). `tools/test-cfg-clamps` proves the rule. `tools/test-cfg-values.ps1` extracts
the real function out of the shipped header and proves all 39 cases.

---

## SHIP THESE. Eighteen keys, in four blocks.

The shipped file is the owner's own rewrite, trimmed again by the owner on
2026-09-19: three headed sections (since 1.0.8 the third is PERFORMANCE: `threaded`,
`unpin` and `cpu_place`, each said in a sentence, with the advice to turn them off one
at a time if the game stutters; the owner, 2026-10-02), and it deliberately does not
explain what it does not have to. THE FILE HAS NO `log` LINE SINCE 1.0.8 (the owner, the
same day): the log is on by default and the file does not say it can be turned off. `hdr_cg_absorb` LEFT THE SHIPPED FILE that day (the owner's
call): it is still a key, its compiled default is still 1, so the absorb still
runs for every HDR player; the file simply no longer offers it.

| block | keys |
|---|---|
| anti-aliasing and upscaling | `quality` `custom` (since 1.0.6) `responsiveness` (since 1.0.6) `upscaler` `sharpness` `preset` `fsr_version` (since 1.0.4) |
| HDR output | `hdr` `hdr_peak` `hdr_paper_white` `hdr_ui` `hdr_rolloff` (and `cfg_live`, commented out) |
| performance | `threaded` (a live line since 1.0.8; commented out from 2026-09-20 until then) `unpin` `cpu_place` (both since 1.0.8: five players with a frame cap reported stutter that `unpin=0` ended, and until then the switch was in no file they had) `rename_sdr` (in the file since 1.1) |
| frame generation (since 1.1, the shape `build.ps1 -Ship` builds) | `fg` (a live line at `0`: frame generation is off by default, the owner's decision of 2026-10-25, and a player turns it on by changing that line to `1`) `fg_min_fps` (a live line at `0`, the floor by display) |

### `quality` = 0..4   **the main one**

The default is `1` (Quality) since 2026-09-25 (the owner), from the performance card's G series: with the 4090 locked
at 900 MHz so the graphics card is the limit, Quality cost 2 percent of the frame rate against the game without the mod,
DLAA 28 percent, DLAA with HDR 38 percent (DECISIONS, "THE G SERIES", on branch perf-card). A player report of 170 to
100 fps at DLAA with HDR was exactly that price, on a default that was DLAA until then.

Was `render_quality` until 2026-09-06, and that name is still accepted silently
so no existing cfg breaks. The struct member was renamed with it, so one name
means one thing. (This paragraph named the same key on both sides until
2026-09-09, which read as a key that was renamed to itself.)

### `custom` = 0, or 34..99   **your own render size**

Your own render size instead of the `quality` presets, as a percent of the screen in each axis (2026-09-23, a
mod-page request; the key was `scale` until 2026-09-24, when it moved under `quality` as `custom`). `0`, the default,
leaves `quality` in charge. `34` to `99` renders the scene at that percent of the
screen size in each axis and takes precedence over `quality`; the log says so
("cfg: custom=N" and "render scale: custom=N percent (quality=Q not used)").
`100` and above is treated as `0`, so the `quality` preset decides (that is DLAA only at `quality=0`); `1`
to `33` is under DLSS's floor of a third and is raised to `34`, and
`IvtReportClamps` says so. The client is size-agnostic: the LOD bias, the jitter
phase count and the target redirection derive from the resulting sizes. The
helper's DLSS picks the nearest mode for the input, and its optimal-size line then
says the mismatch is expected instead of calling it out; FSR takes any size. The
helper learns the value on its command line (`--scale=`), for that line only.
Static: needs a restart.

### `upscaler` = -1, 0, 1 or 2

`-1` automatic, `0` DLSS, `1` FSR, `2` XeSS (Intel XeSS Super Resolution). Automatic picks DLSS on an NVIDIA RTX card,
XeSS on Intel graphics (an Arc card or Intel's integrated graphics: vendor 0x8086) and FSR on every other card. On NVIDIA and AMD cards XeSS runs Intel's slower general path,
so automatic never picks it there. `fsr_version` and `preset` do not apply to XeSS. THE CFG KEY ONLY: the struct member and the
wire term stay `backend`, because that word is in the IPC and on the host's
command line (`FEED_ACK_BACKEND_FSR`, `--backend=`), and renaming a protocol
term across two binaries to improve a player-facing label would trade a small
confusion for a bigger one. `backend` is still accepted as a key.

### `sharpness` = 0.0 .. 1.0

RCAS after the resolve. The shipped cfg documents what it looks like and what
it costs; the two things worth saying here are structural.

**It ships at 0, and 0 is a HARD GATE rather than a strength of zero.** At 0
the shader is never compiled, the scratch target is never created, and the pass
is never reached. That gate is decided ONCE, at startup, from the cfg. The
shipped file and the built-in default now agree at 0; the file said 0.25 while
the code said 0 until 2026-09-09, which was the only place where the file a
user reads and the code disagreed about behaviour rather than wording.

**UNDER `hdr=1` THE BRIGHT PARTS OF THE PICTURE ARE SHARPENED TOO (2026-09-19, audit SHADER-L44).** RCAS measures how
much headroom a neighbourhood has before it clips, and AMD's ceiling is the literal 1: white. Under HDR the back buffer
is open above white, so every neighbourhood holding a street lamp, a neon sign or the sun had NEGATIVE headroom and the
filter switched itself off there: the brightest parts were the only unsharpened ones. The ceiling is now a constant the
mod passes in. It is exactly 1 in SDR, and under HDR whenever values above white cannot reach the display: an 8-bit back
buffer, `hdr_restore=0`, or the rebuilt-composite source, which is the automatic one WHILE FUSIONFIX'S TONE MAPPING IS
ON (there the frame is decoded with a clamp, and a raised ceiling would sharpen into the clip). Otherwise it is six
times paper white in the buffer's own encoding (2.258 at the default `hdr_eotf`), the top of the night emitters'
measured range (4 to 6 times white). NOT above everything this game produces, which this said until 2026-09-19:
headlight interiors reach 25 times white in the project's own night capture, well under 0.2 percent of pixels, and
they and the pixels touching them stay unsharpened. Not the display's peak, on purpose: at 800 and 203 nits the
peak is 3.94 times white, below those emitters, and sharpness would then move with a display setting. So `hdr_restore`
and `hdr_source` now move the sharpener as well, one frame late; the log says `sharpen: headroom measured against N`
once a change. `tools/check-rcas-ceiling.py` is the arithmetic, with the wrong generalisation as its control.

**UNDER `hdr=1` THE SHARPENING RUNS INSIDE THE HDR ENCODE (2026-09-30, G10; step 5).** The same RCAS and the same ceiling
rule, applied where the encode reads the picture, so the separate pass and its full-frame copy are gone; the HUD is sharpened
with the scene there. The separate pass comes back only if that encode could not be made (the log says so), and the
`sharpen: headroom` line above belongs to the pass, so an HDR session with the encode does not write it.

**HDR CHANGES NOTHING ELSE ABOUT SHARPENING** (owner's call, 2026-09-14). From
2026-09-09 to 2026-09-14, `hdr=1` treated a sharpness of 0 as 0.25, because
under HDR the RCAS scratch and the encode's scratch are the same target and the
pass costs no memory there. It went for two reasons: 0.25 was measured
invisible in this game (2026-09-04), and since the R8 fix any client pass
switches FSR's own RCAS off, so under HDR an FSR player got less sharpening
than in SDR. Now 0 means the same thing at `hdr=0` and `hdr=1`, and since
2026-09-17 on both upscalers: none. Whoever wants sharpening sets the key.

**The strength is LIVE within a session that built the pass (2026-09-09).**
It is one pixel-shader constant, so saving the file moves it from the next
frame. Going from 0 to anything still needs a relaunch, because the gate is
not live, and it says so. Setting 0 live stops the pass but does NOT give the
33 MB scratch back; the log says that too, because "sharpening off" and "the
memory is back" are different claims and only one of them is true.

**0 MEANS NONE, ON BOTH UPSCALERS (owner's decision, 2026-09-17).** From
2026-09-14 to 2026-09-17, after field report R8 ("blurry, just removes the
jaggies" of default-cfg FSR at native), FSR ran its own RCAS stage at 0.5
whenever the client's pass was not built. Sitting 2 showed the cost: a session
STARTED at 0 was sharpened at 0.5 while a session set to 0 live was not
sharpened at all, so one cfg value meant two pictures, and the owner preferred
the unsharpened one (and judged FSR's 0.5 a touch sharper than the mod's RCAS
at 1.0). FSR's own stage is off always now; the mod's RCAS is the one
sharpener, driven by this key on both upscalers. The answer to R8 is in the
text a player reads instead: the cfg and the README say FSR's output is softer
than DLSS's and that 0.5 is AMD's own default strength. `FEED_BUILD_CLIENT_RCAS`
is retired (its value is left unused). The cfg's old "Try with 0.25 first"
contradicted the project's own measurement (0.25 invisible in this game); it
says 0.5.

### `preset` = 0, or 10..13

The DLSS model hint. Ignored by FSR and XeSS. MOVED HERE from a second cfg file beside the host exe,
which no longer exists. It reaches the host as `--preset=`, the same way
`--backend=` always did.

### `fsr_version` = 0, 3 or 4

SHIPPED SINCE 1.0.4, in the owner's words (2026-09-21); a development lever
for the FSR 4 tests of that morning before that. Which FSR runs when the
upscaler is FSR. Ignored by DLSS and XeSS.

`0` (the default) is the version AMD's library picks, which was the newest
one offered in every layout measured (3.1.4, 3.1.5, 4.1.1b, 4.1.1r11): the
cfg's "the newest FSR your graphics card supports". `4` or `3` (any single
digit is read): the newest offered version with that major number. One that
is not offered is a log line and the library's own pick, never a refusal,
which is the cfg's "Setting 4 on a card without FSR 4 gives you FSR 3.1
instead". `-1` is read as `0`, and anything else out of range is clamped
to `0`.

AUTOMATIC TESTS FSR 4 FIRST WHERE DIRECT3D 12 MAY BE TOO OLD FOR IT (the
start-up test, 2026-10-10, for 1.1; `host/fsr4_test.h`). Where the helper's
D3D12 offers shader model 6.5 or less, and the library offers an FSR 4 and an
FSR 3, the helper starts a throwaway copy of itself that creates the FSR
context automatic would create and runs its first dispatch. FSR 4 ran there:
`0` runs FSR 4. It failed, raised, or the copy died: `0` runs the newest FSR 3
and FSR 4 is never created in the helper itself (a library that raised is
never called again in its process, so the raise has to happen in another).
NOTHING IS KEPT (the owner, 2026-10-10): the test runs once a session at every
launch that meets the rule, and costs a quarter of a second where it was
measured (the copy's start, its device, the context and one dispatch). A
copy that does not end in 30 s is ended, FSR 3 runs that session, and one
line in `DLSS-IV-WARNINGS.txt` says how to skip the test (`fsr_version` 3 or
4). Under Wine there is no test and the
rule below decides alone, as in 1.0.9. The log says it in one line
(`FSR: FSR 4 test: ...`), and the copy adds one more only to say where FSR 4
raised.

THE RULE BEHIND IT (the owner, 2026-10-03, after 1.0.8; alone until 1.0.9):
where the test says no or gives no answer, `0` runs the newest FSR 3 and
FSR 4 is never created. `DLSS-IV-WARNINGS.txt` says so once a session and
says what raises the shader model: the 64-bit `D3D12Core.dll` of a RELEASE
version of Microsoft's Agility SDK (the `Microsoft.Direct3D.D3D12` package,
`build\native\bin\x64`) in `DLSSIV\D3D12\`; a preview version is refused by
Windows and the text then says that the file there could not be used; under
Wine it names a newer Proton instead. The reason is one player's Radeon RX
7800 XT on Windows 10, twice: AMD's own FSR 4.1.1 crashed in its first frame
there, which ends FSR for the session, while FSR 3.1 ran and FSR 4 ran once
a release Agility runtime was in place. The test exists because the next
report was a Radeon RX 9070 XT on Windows 10 with the same shader model 6.5,
where FSR 4 runs: the shader model alone does not say. `4` still asks for
FSR 4 whatever the shader model and whatever the test said, and its crash
note names the same remedy. Shader models 6.6 and 6.7 are not measured with
AMD's FSR 4 and nothing changes there: no test, FSR 4 as before.
The rule is `FsrBelowFsr4` in the helper and the test is `host/fsr4_test.h`,
both proved by `tools/test-fsr-pick` (the reading
of the copy's exit code on a bench with prove-all; its `--live` part runs the
built development helper against a community FSR 4 library with the line
moved by `--fsr4_min_sm` and a fault planted in the copy by
`--fsr4_test_fault`, development helper arguments that no cfg key reaches).

What is offered depends on the files in `DLSSIV\` and on the GPU. AMD's SDK
2.3.0 `amd_fidelityfx_loader_dx12.dll` with its
`amd_fidelityfx_upscaler_dx12.dll`, what ships since 1.0.4, offers 3.1.5 and
2.3.4 on an RTX 4090 and adds FSR 4 on AMD's RDNA 3 and 4. SDK 1.1.4's
`amd_fidelityfx_dx12.dll` (shipped until 1.0.3, and still loaded when no
loader is present) offers 3.1.4 and 2.3.3. The log at `log=1` lists what is
offered and names the version that ran (`FSR provider in use`). If the
chosen version fails to create, the next offered FSR 3 or newer is tried and
logged. A community FSR 4 build replaces the upscaler DLL and is offered like
AMD's own; its first dispatch is warmed up inside the build (see
`fsr_warmup` below). Needs a relaunch.

### `log` = 0..3

`1` IS THE COMPILED DEFAULT SINCE 1.0.8 (the owner, 2026-10-02: six players reported stutter on 1.0.7 and not one
could send a log, because none had one), AND THE KEY IS NOT IN THE SHIPPED FILE ANY MORE (the owner, the same day: ship
with the log on and do not mention that it can be turned off). It is the record a bug report needs. `0` writes NOTHING:
no file is created at all (the default until 1.0.7; a cfg kept from then still says it, and is obeyed). `2` adds periodic health counters, `3` adds
everything. WHAT `1` COSTS: its telemetry counters were measured at 0.06 to 0.1 ms a frame on the render thread at `log=2`
on 2026-09-18, before the release was made lean, and every figure this project publishes was measured at `log=1`; `0`
against `1` on the current release is not measured.
The 64-bit helper writes into the SAME file, prefixed `host:`, via
`--logpath=`. Levels 2 and 3 are not in the shipped file.

**What each level means, tightened 2026-09-14** (the owner: a player's log had
become our lab notebook, most of it the device thread's report every 600
frames). The rule now:

- **Two files exist at ANY log level, `log=0` included, and only when there is
  something to say.** `DLSS-IV-DID-NOT-START.txt` next to `GTAIV.exe` when the
  mod could not run at all (or stopped part way through), with the reason and
  the three installation faults that cause it. `DLSS-IV-WARNINGS.txt` (owner's
  decision, 2026-09-14) when something the cfg asked for is not running: HDR
  refused (Windows HDR off, exclusive fullscreen, a non-DXVK runtime, the back
  buffer not FP16, the colour space never set), the device thread refused, a
  cfg line the parser could not use or a value it clamped, a DXVK that is not
  3.1, a second copy of the .asi loaded, the sharpening pass refused, the log
  file refused by the game's folder. One line each, appended as they happen;
  both files are deleted at the next start. If the game's folder refuses
  writes (Program Files without permission), both land under `%TEMP%` instead,
  and so does the log. Nothing forces a setting on the player; this is how they
  learn within one launch what did not take.
- **`log=1` is a product's log.** The header is the build, the clock, the
  level and the pid. Then: the game version, FusionFix and its version (or that
  it is not loaded), one `cfg:` line with the keys that decide the picture
  (quality, upscaler, hdr, sharpness, preset, threaded), the card the game
  renders on with its driver version in readable form, the back end, the device
  hooks arming, `armed (+ms)`, the host started and connected, `FIRST INJECTION`,
  and after that only EVENTS and FAULTS: a Reset (`reset #N`), the device thread
  arming and disarming, HDR engaging (once) or refusing, `resolve on/off` edges,
  a `FRAME GAP` or a per-frame `SLOW` call, every one-shot fault line, each
  saying what it costs the picture, a `health` heartbeat after the first 300
  injections and every thirtieth period after it, the warnings that also went
  to `DLSS-IV-WARNINGS.txt`, and a closing `session over:` line with the frame,
  injection, reset and camera-cut counts. No per-period report, no hex handles,
  no audit or run references, no explanation of our process.
  (The budget of 2026-09-19, `docs/reports/instrument-audit-2026-09-18/04-log-design.txt`:
  what moved to `log=2` was ours and not a player's. The per-cut camera lines,
  `jitter on/off`, the scene camera's re-election, the Reset's pointer dump, the
  adapter LUIDs, D3D9's spoofed adapter line, the shared set's DXGI enums, a
  successful CreateDevice's raw parameters, the repeats of lines a normal start
  printed two and three times, and a `SLOW` build exchange, which is expected.)
  (The rewrite of 2026-09-30, the owner's rule from the SISCO log work: every
  `log=1` and warnings line says figures, identities, decisions and events, and
  no longer the reasons, what the mod does, or "please report". The GPU is named
  once; an adapter LUID only where two GPUs are in play. The warnings file keeps
  what is wrong and one thing to do about it. Every line a tool reads kept the
  words that tool matches; the readers are listed in the log rewrite's record.)
- **`log=2` adds the periodic health**: the transport's 300-injection line,
  the device thread's 600-frame report, the memory census, the scene-camera
  capture line, the seam line, the camera-cut lines (each cut, the first 120),
  the scene camera's re-elections, `jitter on/off`, the probes, the full cfg
  dump, the tracker's report at the verdict, and the one-shot developer detail
  (which slot was hooked, which VkImage, which depth path).
- **`log=3` adds everything** the instruments can say.

`tools/log_inventory.py` lists every call site by level with its gating, so
the next review of what a player sees is a read of one file, not a session.

**`log` controls what is WRITTEN, not what is MEASURED.** `hdr_view` costs
real time and stays a separate key, so `log=3` can
never silently cost frame rate. A measurement not taken cannot be written.

---

## THE FORMER SHIP LIST, now development levers

Three of these were documented as shipped and are not in the file any more: `transport`, `async_home` and
`jitter_phases`. THE FIRST ENTRY, `quality`, IS SHIPPED (it is the first key of the file and heads the SHIP list
above); its full ladder has simply always lived in this section, and this paragraph said "these four ... are not in
the file any more" from 2026-09-06 to 2026-09-19, which told a reader the mod's main key was a development lever.

### THE `quality` LADDER IN FULL (a SHIPPED key: its short entry is under SHIP THESE, above)

    0  DLAA           the scene at the screen size, no upscale
    1  Quality        the scene at two thirds       (2560x1440 at 4K)
    2  Balanced       the scene at 0.58             (2227x1253 at 4K)
    3  Performance    the scene at a half           (1920x1080 at 4K)
    4  Ultra Perf     the scene at a third          (1280x720 at 4K)

Sizes land exactly on NGX's own optimal input for each mode; the host verifies
and logs "optimal size off by 0 pixel(s)". Measured on this machine at 4K,
Quality is 10 to 15% faster than DLAA.

**FusionFix's own Antialiasing under a scale (known, 2026-09-14).** The scale
shrinks every screen-size 8-bit render target the game creates, and FusionFix's
SMAA/FXAA targets (edges, blend, temp2) are created through the game's own
texture factory at exactly that size, so with FusionFix Antialiasing on its post
AA runs at the render size and its final blend is stretched to the back buffer.
DLSS-IV already supplies the anti-aliasing; FusionFix's runs on top of the
resolve and, under a scale, costs sharpness. Leave it Off with `quality>=1`.
There is no clean way to exempt those targets from this side: the creation
comes from GTAIV.exe, not from FusionFix's module.

### `transport` = 0 or 1

DEFAULTS TO 1 since 2026-09-06 and is no longer in the shipped file: a cfg that
omits it should still run. The mod is turned off by deleting the cfg, not by
this key. **0 spawns no host and creates no velocity target, jitter or render
scale** (this line said until 2026-09-21 that 0 gives motion vectors and jitter;
the code has given neither since the velocity target moved behind the transport,
audit 2026-09-21, TR-M1). It is the frametime BASELINE since 2026-09-20, with one
caution: the device thread and the mover tracker still run at transport=0 and
the default threaded=1, and hdr=1 still engages, so a baseline of the mod idle
is `transport=0` with `threaded=0` and `hdr=0`. The mean/max frametime line it
is compared on is a development build's log=2 line; a release build keeps only
the FRAME GAP line over 500 ms.

### `async_home` = 0 or 1

**1 is what ships and this key is deliberately not in the player's file.** It
is documented here, on the development-lever list, because it still works if
typed and because roadmap A10 may want to measure it.

`1` pipelines the resolve, so the frame carried home is one frame old. `0`
waits for the current frame's result before continuing: lower latency, and the
round trip is inside the frame instead of across two, so it costs frame rate.

**`0` WAS BROKEN FROM 2026-09-04 TO 2026-09-09 AND IS NOW FIXED.** Design A
moved the resolve's home out of the back buffer and into the game's scene
buffer; the pipelined branch was changed and the same-frame branch was not, so
at `0` the resolve was computed, waited for, and then discarded when the post
chain read the unresolved scene buffer and overwrote the back buffer copy. Any
measurement of `async_home=0` taken in that window is worthless. Both branches
now home into the same place.

### `responsiveness` = 0, 1, 2, 3 or 4   **the latency setting**

THE LATENCY MODES (2026-09-23; `docs/LATENCY-PLAN-2026-09-23.md`). `4` is the shipped default (2026-09-24: the owner's card ran it three
times, on DLSS and on FSR 4, with no frame rate lost and about a frame of picture age won on a machine that is mostly
CPU-bound; more where the queue is in use). `0` turns the latency modes off (1.0.5 had none; the release's other changes have their
own switches: `unpin`, `cpu_place`, `rename_sdr`, `threaded`, and the DXVK key in dxvk.conf, see `d3d9.deviceLocalConstantBuffers`
below; the final audit 2026-09-30, cfgAndList-readme-wayout). `1` has the mod set DXVK's frame queue itself: DXVK otherwise lets the CPU run up
to min(3, BackBufferCount + 1) frames ahead of the GPU, and once the mod makes the GPU the bottleneck each of those
frames is input delay. The mod writes `d3d9.maxFrameLatency` into the `DXVK_CONFIG` environment variable before any
DXVK instance exists, so the player edits nothing; if the player's own dxvk.conf (read from the game's folder, where
DXVK reads it) or DXVK_CONFIG already sets that key at the same depth or shallower, the mod leaves it alone and says so; a
deeper value (the older advice's `2`) is overridden and said, because yielding to it undid the setting (2026-09-24); `frame_queue=0` keeps yours. A line at device time says
whether DXVK's instance received the key, read from DXVK's own log. `2` adds the mod's pacer (`pacer`). `3` adds the
same-frame upscale, `async_home=0` in effect, unless the file names `async_home` itself, which then wins, and never on the
Linux bridge, where the relay's round trip would sit inside every frame (3 and 4 run pipelined there); the stored
cfg keeps what the file said, so a live edit never warns about it. `4` (2026-09-24) is `1` always, plus as much of `3` as the player's machine gives for little frame rate, MEASURED rather
than guessed (the same-frame upscale for 3 percent or less; the pacer for 3 percent plus 3 for every frame of picture age it
wins, never above 9: the owner's decision after the clock-locked legs read the pacer at 4 to 5 percent for a frame and a half,
a coin flip at a flat 3): with a limiter or vsync on the same frame is simply on (the GPU has slack by
construction, and the pacer stands down on its own rule); otherwise the mod measures the same-frame upscale against the
queue alone, interleaved A A A B B B then B B B A A A, pair after pair in alternating order (the first window of each half
settles and is not counted; the
verdict is the median of the pair ratios, so a drifting scene or one hitch cannot decide it; three pairs, two more when
the verdict lands within two points of the line), and keeps it if it fits. The pacer can only remove queue wait, so it
is measured only where either half of that measurement averaged a quarter of a frame or more in flight, and kept only if the two together fit and it won
at least a quarter of a frame of picture age; while kept it is audited every 90 windows (held for four, compared with
the three before) and dropped when it no longer fits, no longer wins, or has nothing left to convert. The ladder rests
on the rung it chose for 300 windows (five minutes at 120 fps), or until three consecutive windows leave the band of
0.75 to 1.33 of its period, and measures again; a rung that did not fit is tried again after a minute; a rung the
ladder sits on is dropped only when a re-measure reads one point over its line (hysteresis, so a rung near the line
does not flap). Each verdict is
one log line with the numbers, the flips inside a measurement print at log=2; the same-frame flag travels per frame, so
a flip never splits a frame; the pacer's hold restarts its estimator on release; never on the Linux bridge. Every mode runs the instrument; at log=2 it runs even at `0`, so a baseline can be measured (the `latency_log` key was folded into `log` on 2026-09-24). Static: needs a restart.

THE DXVK CONSTANT PATH (2026-09-30, step 5's sittings 23 and 24; no key). Every build also writes
`d3d9.deviceLocalConstantBuffers = False` into `DXVK_CONFIG` before DXVK starts: DXVK 2.x's way of passing shader constants,
without copying them into video memory each draw, about 0.40 ms a frame faster where the CPU sets the pace and nothing seen
where the GPU does. The player's own value wins: naming the key in `dxvk.conf` beside `GTAIV.exe`, or in `DXVK_CONFIG`
(`d3d9.deviceLocalConstantBuffers = True` for DXVK's other path), turns the mod's off, and the log says which. DXVK reads at
most 259 characters of `DXVK_CONFIG`; if the key would not fit, the mod leaves it out and says so. At device time the log
says whether DXVK's instance received it, from DXVK's own log.

### `frame_queue` = -1, 0, 1, 2 or 3

The depth `responsiveness` asks DXVK for. `-1`, the default, takes it from the mode (`kLatQueue`, 1, decided by the card). Above `3` is `3`.
`0` touches nothing even at `responsiveness>0`. `1` to `3` is that depth by hand, and BY HAND OVERRIDES the player's own
dxvk.conf value (DXVK reads the environment after the file), which the log says. For A/B runs and for Linux players.
Read by release builds; not in the shipped file.

### `pacer` = -1, 0 or 1

The pacer on its own switch: `-1`, the default, follows the mode (on at `responsiveness` 2 and above; at 4 the adaptive step decides when it runs); `0` off and `1` on
whatever the mode. The pacer stands down under a limiter read from FusionFix's cfg in FusionFix's own five folders and choice rule (audit F11;
the final audit, 2026-09-30): FusionFix's own .asi is found first (loaded, or in plugins\, the game folder or scripts\), and
its folder and name decide the first place and the file's name, `<its name>.cfg`, then the exe folder, LocalAppData twice
and Documents; the first existing writable file is FusionFix's, whether or not it carries the key (FusionFix 5.1 writes only
the keys that differ from its defaults, and a missing FpsLimitPreset is Off); Custom's FpsLimit comes from the .ini beside
FusionFix's own .asi first. When FusionFix had not loaded when the mod read it (it creates a missing cfg as it loads), the cfg
is read again at the game's first device, and the log says so only if the answer changed. The pacer holds the render thread just after Present, by LatencyFleX's estimate of when the GPU will
be free, so a frame queue of one costs no GPU idle; it stands down under vsync, under a frame limiter (FusionFix's FPS
limit read from its cfg, or DXVK's d3d9.maxFrameRate; read at start, and again at the game's device as above), for 16 frames after a Reset, and while the
instrument sees no live frames; its sleep is capped at 20 ms and at twice the GPU period. Its frame end is the frame's own
GPU end, signalled after each Present (2026-09-24), not the injection's retire. The estimator is
LatencyFleX (Apache-2.0, `licenses\LICENSE-LatencyFleX.txt`). Read by release builds; not in the shipped file.

### `unpin` = 0 or 1

GTA IV pins its main thread to CPU 0 at start-up and keeps it there. `1`, the default, gives that thread back every
processor the game is allowed, once, when the game creates its device; `0` leaves the pin. On the owner's 7900X3D it was
worth 14 percent of the frame rate and about 2 ms at p99.9 in a CPU-bound scene (DECISIONS, U1, on branch perf-card).
It changes only the thread's affinity mask: a player's own affinity or CPU Sets still apply, a thread whose mask is not
exactly CPU 0 is left alone, and Wine and Proton apply the pin and its undoing the same way Windows does. The log says
which it did, in a `cpu:` line.
SINCE 2026-09-30 THE SAME KEY COVERS THE PIN (source/cpu_place.h; step 5, sittings 22 to 24): once the game world has run
a few seconds, on a CPU with two or more last-level caches the game and the helper are given one of them (the larger on an
X3D part, the lower-numbered otherwise, and only one holding six cores or more). It stands down under Wine and Proton, on
Windows before 10, on more than 32 logical processors, and when another program (XOS Toolkit, Process Lasso, a rule of your
own) already set the game's or the helper's CPU set or affinity; if one does later, the mod takes back what it wrote and
leaves the rest to that program. It needs the transport (`transport=1`, the default): the game world is counted by its
injections. `0` leaves the game's CPU handling entirely alone: no unpin, no pin, and no wall whatever `cpu_place` says.
Needs a relaunch. In the shipped file since 1.0.8, in its PERFORMANCE section.

### `cpu_place` = 0 or 1

THE WALL (source/cpu_place.h). `1` gives the threads that set the frame rate (the render thread, DLSS-IV's device thread,
DXVK's command thread and the game's main thread) one logical processor each that nothing else of the game uses, once the
game world has run a few seconds. Two cores of the fastest kind are always kept for the rest (so a four-core CPU walls
two, and a laptop with two fast cores beside its efficient ones walls none), and nothing is walled where the game has
fewer than six logical processors. It needs `unpin=1` and stands down wherever the pin does. On the owner's 7900X3D it was
worth 0.3 to 1 ms a frame where the processor set the pace (DECISIONS, Z5 to Z13).
`1` IS THE DEFAULT, AS 1.0.7 RAN IT; THE KEY IS NEW IN 1.0.8. In 1.0.7 the wall ran whenever `unpin` was 1 and could not be
turned off alone. Five players with a frame cap their machine reaches reported stutter that `unpin=0` ended; the two whose
logs were read had the wall and not the pin. A thread held to one processor cannot move when something else is put
there, and the wall's gain was measured on one machine, above the cap. Whether the stutter came from the wall or from the
unpin itself is not measured (`unpin=0` ends both). The owner drove both caps on 2026-10-02 with the wall on, off and at
`unpin=0`: six legs, all clean, so it is not reproduced here. For a day the default was `0`; the owner then decided to
ship both on, as 1.0.7 did, and to let the shipped file's PERFORMANCE section name `threaded`, `unpin` and `cpu_place` for
a player to turn off one at a time. `0` gives no thread a processor of its own; the unpin and the pin stay.
`0` IS THE DEFAULT SINCE 1.1 (the owner, 2026-10-10: "we will put cpu place 0 as a default insted of 1"). Every stutter
report whose CPU is known is one cache with threads on processors of their own (eight reports by 2026-10-05), and the
gain was measured on the owner's two-cache CPU alone. The shipped file keeps the line, now `cpu_place=0`, and says that
a dual-CCD Ryzen (two core groups) may gain from `1` (the owner's wording of 2026-10-11: "rewording it to dual CCD would be better"; it said "12 or more cores" until then, and the gain was only ever measured on his two-group CPU). A value out of range reads as 0. The readme's list of lines to try
against stutter no longer names it.
Needs a relaunch. In the shipped file.

### `rename_sdr` = 0, 1 or 2

NVIDIA's Windows driver presents a Vulkan window whose engine is named exactly "DXVK" through its composited path: in
borderless there is no G-Sync, and vsync holds near 119 at 120 Hz. `1`, the default, gives DXVK's instance the
`hdr_engine_name` string in SDR sessions too, when an NVIDIA GPU drives the primary display, so the window is presented
with independent flip and gets G-Sync. ON A HYBRID (2026-09-30: a laptop without a MUX switch, where the NVIDIA GPU renders and
the integrated one drives the panel) the same when NVIDIA's driver is installed, since DXVK picks a discrete GPU before an
integrated one. A DXVK device filter keeps "DXVK". The log and the warnings file say if the game then renders elsewhere; NVIDIA's G-Sync setting is not read there. Unmeasured on a hybrid:
whether a renamed window tears below the refresh. NOT when G-Sync is switched off for GTA IV alone in NVIDIA's per-program
settings: there a renamed window tears and one named "DXVK" keeps vsync, so the mod reads that setting (read-only, once,
at DXVK's first instance) and keeps "DXVK"; with HDR the rename stays and the warnings file says what to change. `0`
keeps "DXVK" in SDR. `hdr_engine_name=0` turns the rename off in both.
NVIDIA'S OWN "VERTICAL SYNC: ON" (1.0.8, 2026-10-02; a 1.0.7 player's report: SDR, a fixed-refresh screen, "Vertical
sync" On in NVIDIA's settings, the frame counter flipping between 60 and 61 with stutter, ended by `rename_sdr=0`). The
driver's forcing does not pace a window that DXVK presents without vsync. Recorded on the owner's machine (legs NV1 and
NV2, 60 Hz, G-Sync unticked): renamed, the frames ended 33, 5 and 12 ms apart in runs of many seconds; named "DXVK", 12
to 21 ms apart at a steady 60, felt as micro-stutter. So the same read now takes NVIDIA's vertical sync setting for GTA IV
(`VSYNCMODE`) and, where it is "force on", THE MOD ASKS DXVK FOR VSYNC ITSELF: it passes the present interval ONE in the
game's place at CreateDevice and at each Reset, in SDR and with HDR, with G-Sync or without, renamed or not (with
`rename_sdr=0` or `hdr_engine_name=0` the setting is read at the game's first device instead). The rename stays. That is
the state every vsync leg of the performance card ran in. Two cases are left where DXVK cannot be asked, and there SDR keeps
"DXVK" when no variable refresh is in play (G-Sync unticked for the display, or the driver says the display cannot do it),
as `rename_sdr=0` did for that player: `d3d9.presentInterval` set in dxvk.conf or DXVK_CONFIG (DXVK puts it above the
application; with 0 there the warnings file says to remove the line), and vsync forced at half, a third or a quarter of
the refresh (what DXVK does with those intervals in a window is not measured). Fast Sync is not counted. `2` renames in
those two cases too. NOT VERIFIED: what the driver answers about variable refresh for a monitor that has none.
Never under Wine. With the name changed, the driver's vsync stops waiting for the refresh, so the refresh guard holds a
game that runs faster than the refresh to NVIDIA Reflex's cap for it (116 fps at 120 Hz); it only engages when frames
outrun the refresh, and says so once in the log. Why: docs/reports/engine-rename-2026-09-26.md. Needs a relaunch.
Read by release builds. IN THE SHIPPED FILE SINCE 1.1 (the owner, 2026-10-10: "add rename sdr to the shipped cfg - but
mention maybe that it's for SDR only", and with his rearranged file the same day: "add rename SDR there", the
PERFORMANCE section): `rename_sdr=1` at the section's end, with the text that it is for NVIDIA cards in SDR and that
0 ends G-Sync in a borderless window. With HDR on it changes nothing (there `hdr_engine_name` decides).

### `sync` = -1, 0, 1 or 2

THE LINUX SYNC PATH (2026-09-23; `docs/LINUX-VERDICT-2026-09-23.md` has the evidence, `DlssIvTransport.h` above
`ivtWine` the design). `-1`, the default, decides by platform: `0` on Windows, `1` under Wine, which the mod detects
from ntdll's own `wine_get_version` export. Needs a relaunch; the helper is told on its command line (`--sync=`).

- `0` the native path: the helper creates two D3D12 shared fences and the game imports them as timeline semaphores.
  What Windows has always run. Under Wine it is how "does this Proton's Wine still share a D3D12 fence between
  processes" is tested: stable Proton 11.0 and GE-Proton11-6 should pass, Proton Experimental and GE-Proton11-7
  cannot (their Wine of 2026-09-11 emulates D3D12 fence values per process and returns success for an import
  that never connects).
- `1` the game creates the two timeline semaphores, exported as plain Vulkan (OPAQUE_WIN32) handles that Proton's
  emulation does not intercept, and the helper opens them as fences. A PROBE then proves the link both ways before
  any GPU wait depends on it. Both directions seen: FAST, both sides wait and signal on the GPU exactly as on
  Windows, no CPU hop per frame. Otherwise the BRIDGE.
- `2` the bridge, forced: the same two semaphores, but a relay thread of the game's writes each frame message only
  after the game's own copies retired, the helper answers each frame after its own fence, and the relay signals the
  game's GPU wait from the CPU. One pipe round trip a frame. It needs no cross-process GPU object at all, so it is
  the path that works on every Wine that shares the textures, and `2` on Windows is how it is exercised where the
  shared fences would otherwise hide it.

Under Wine, whatever `sync` says, the shared textures are imported as OPAQUE_WIN32 (what vkd3d-proton exported
them as, and the one type Proton 9 and 10 accept) and the memory-type narrowing is skipped. The log at `log=1`
carries one `platform:` line naming Wine and the mode asked, and one `sync probe:` line naming what was seen and
which mode holds; the helper's lines say the same from its side.

### Thirteen development keys and their switches (DELETED 2026-10-25, the deep strip)

`stage`, `reset_on_cut`, `jitter_hold`, `jitter_phases`, `hdr_lut_pivot`, `quad_uv`, `corona_hold`, `rain_layer`,
`steady_depth`, `dedither`, `fsr_debug_view`, `fsr_warmup` and `hdr_cs_test` were development builds' keys only (a release
never read one) and no build reads them now: every build runs the shipped value a release always ran (stage 3, the cut's
reset and the jitter hold on, NVIDIA's jitter rule, the LUT's own pivot, the quad fix, the corona hold, the rain layer, the
steady depth and the dither pass all on, FSR's debug view off, FSR 4's warm-up by version, the surface asked for scRGB). With
them went `dedither` 11 and 12 (the pass's view) and the dither pass's and the steady depth's development counters and
checks. A cfg that still carries one gets the unknown-key line. docs/INSTRUMENTS-FG.md says where each lived; the tag
`fg-instruments` has all of it.

### The stage dump and its key (DELETED 2026-09-19)

The stage dump, which wrote the next N frames' buffers to disk as BMPs, and its key were deleted with the other
development-only instruments (the owner's decision). A dump= line in the file is no longer read. Git has it at `a9accc1c` and earlier.

### The per-hook profiler and its key (DELETED 2026-09-19)

The rdtsc brackets in every hook and the `profile=` line that switched them on were deleted with the other
development-only instruments (the owner's decision). CpuProbe (`tools/cpuprobe`), a sampling profiler with no code in the
mod, does that job. A profile= line in the file is no longer read.

### The call census and its key (DELETED 2026-09-19)

The census counted every D3D9 call a frame by method, caller and thread, through a counting thunk it generated for
every device slot at start-up. Its question, whether D3D9 could leave the render thread, was answered when the device
thread shipped, and it was deleted with the other development-only instruments (the owner's decision). The line that
switched it on is no longer read. Git has it at `744c3f98` and earlier.

### The transport's debug views and their two keys (DELETED 2026-09-19)

Six views drawn over the finished frame (the depth guide, the vectors as colour, the raw depth snapshot, the resolve's
own input, the object-vector rejection with its heat ramp), their pixel shader, and the two lines that chose the view
and set the ramp's top. Deleted with the other development-only instruments (the owner's decision); neither line is
read any more. The HDR views that were kept are `hdr_view` 6, 14 and 17. Git has the rest at `c6747cea` and earlier.

### The motion vector self-test and its key (DELETED 2026-09-19)

With the key set and `log=3`, the helper warped the previous colour frame by the motion vectors and scored the result
against the current frame (0 means the vectors explain the motion, 1 means they explain nothing). It is how the
vectors' sign was settled (8,480 frame pairs) and how they were last measured, on 2026-09-17: 0.22 to 0.32 for
motion above three pixels, and near 1 below that, where there is almost no motion to explain (`docs/evidence` has
the run). Deleted with
the other development-only instruments (the owner's decision); the line is no longer read. It is the instrument to
restore if the motion vectors are ever in doubt again: git has it at `0732172c`, both halves.

### The automatic A/B and its key (DELETED 2026-09-19)

With the key set to N the transport switched the whole resolve on and off every N frames and reported the two
frame-time means. It is how "what the mod costs" was measured (DECISIONS has the four runs). Deleted with the other
development-only instruments; the line is no longer read. Git has it at `0732172c`.

### The HDR luminance meter and its three keys (DELETED 2026-09-19)

The meter measured the PRESENTED frame every N frames (max, p99 and mean nits, the share above paper white and at the
ceiling, and how well the encode's rebuild of the SDR picture matched the game's own), reported every so many frames,
and could write one sample's per-block table as a CSV. The calibration it was built for is done and its results are the
shipped defaults, so it was deleted with the other development-only instruments (the owner's decision), and with it
the one-shot dump of the composite's shader constants that its first sample triggered. None of its three lines is
read any more. Git has all of it at `44fa7df0` and earlier.

### Step 5's test keys: queue_ab, census, cpu_limit, dxvk_cb, hybrid_test (DELETED 2026-09-30, Stage F)

Step 5's A/B and measurement keys, taken out with their code once their questions were answered: `queue_ab` (DXVK's frame
latency switched 3 against 1 in ABBA blocks inside one session, the latency card of 2026-09-24), `census` (instrument B4, the
drain census: every wait of the render thread named by its caller), `cpu_limit` (the game confined to a smaller CPU for the
few-core legs), `dxvk_cb` (`2` left DXVK's own constant path, the A arm of the constant key's launch against launch) and
`hybrid_test` (the hybrid laptop's SDR rename path run on an NVIDIA display for Stage E's card). `split_pipelined` is no longer
parsed either (the owner dropped L3); its field went at the final audit (2026-09-30).

### The diag key and its two censuses (DELETED 2026-09-30)

Two censuses that only spoke at `log=3` (the camera tally and the scene-camera pick). They went in edac3fa1 (2026-09-16) and
the key switched nothing after that; the final audit took the key out. A cfg that still carries it gets the unknown-key line. None of these lines is
read any more; an old development cfg that carries one gets the "not a key this build knows" note. `opt_ab` kept its engine
and its calibration switches 1, 2, 3 and 6 until the deep strip of 2026-10-25, which took it, `opt_ab_block`, `opt_ab_pairs`,
`opt_ab_pad`, `opt_ab_hold`, `opt_ab_holds`, `gpu_probe` (instrument B1) and `frames_csv` (the frame recorder) out too; its
candidate switches 4, 5 and 7 to 25 went with their code at Stage F. Every piece, where it lived and how to bring it back, is
in docs/INSTRUMENTS-STEP5.md and docs/INSTRUMENTS-FG.md; the tags `step5-instruments` and `fg-instruments` have all of it.

---

## DO NOT SHIP THESE. Development levers, working but undocumented.

Each of these has exactly one correct value for a shipping build. They exist so
a specific failure can be isolated, and every one of them can produce a broken
pipeline if changed without knowing why.

**THE DEFAULTS ARE NOW THE SHIPPING VALUES**, so the shipped cfg omits every
key below (one exception since 2026-09-20: `threaded`, offered commented out until 1.0.7 and as a live line of the PERFORMANCE section since 1.0.8, with the advice to set 0 if the game runs worse) and still behaves correctly. One was wrong until 2026-09-04, found
by writing the shipping file and checking each key we meant to LEAVE OUT
against what it would fall back to:

  - `mode` defaulted to **1**, the transport test, in which the host copies
    Colour to Output with no NGX at all. A shipped cfg without an explicit
    `mode=` would have run perfectly and done no DLSS whatsoever. Now 2.
  - (`watchdog` was the second. THE KEY IS DELETED, 2026-09-19, with the per-draw
    stamp it gated, which nobody had ever switched on. The 50 ms blocking-call
    report, `IvtWatch`, was never gated on it and still runs for every player.)

`transport` DEFAULTS TO 1, which is right for a cfg that simply omits the key,
and is NOT what makes the file's presence a master switch. Defaulting on would
break that switch on its own: with no file at all the compiled defaults would
run the entire mod. What actually enforces it is `ivtCfgMissing`, set in
`IvtReadCfg` and tested by `InitThread` before a single hook is installed. Both
halves are needed and neither is sufficient. **This paragraph said "transport
stays 0 by default" until 2026-09-09**, which contradicted the code and the
transport's own comment block twenty lines apart; audit M19.

| key | ship value | what it does |
|---|---|---|
| `mode` | `2` | DEVELOPMENT BUILDS ONLY since 2026-10-31: a release parses it, clamps a 1 to 2 and has no transport-only path. `1` is the transport test: the host copies Colour to Output with no NGX at all, to prove the plumbing without the resolve. `2` is DLSS. |
| `flags` | `-1` | DEVELOPMENT BUILDS ONLY (the owner, 2026-09-30, the final audit's hidden-key review): a release build does not read it. NGX feature-flag override. `-1` means the host's default (`MVLowRes \| AutoExposure`, plus `DepthInverted`, plus `IsHDR` when the scene buffer is FP16). Any other value replaces ALL of them, including the ones that are facts about our data rather than preferences. |
| `depth_mode` | `-1` | `-1` auto, `0` copy, `1` direct, `2` substitute. Auto has picked DIRECT in every log ever produced. The copy path exists as a fallback and carries the extent check that has now refused a 1024x1024 shadow map in three runs out of three. |
| `rs_ds` | `1` | DEVELOPMENT BUILDS ONLY (the owner, 2026-09-30, the final audit's hidden-key review): a release build does not read it. The render-scale gate shrinks the game's screen-size depth-stencil textures along with its colour targets, as it always has. `0` leaves them at the output size. DIAGNOSTIC ONLY: the scene then rasterises into a depth larger than its colour target and depth-dependent effects will be wrong. It exists to name the dimension of a fault, not to ship. Needs a relaunch. |
| `inject_worker` | `2` | Which thread the injection runs on. NOT A PLAYER KEY and not in the shipped cfg; the default is `2` since 2026-09-19 (the owner's decision, on five paired A/Bs of +0.29 to +0.47 ms a frame at `hdr=1`, development and release builds alike; `docs/OPTIMISATION-LEDGER.md` has each). `2` is `1` with the velocity drain inside the body on the worker too, and no wait at the seam. `0` runs it inline on the render thread, which is the way out. `1` no longer exists (it drained the velocity batch at the post, measured at nothing, and came out on 2026-09-19); a cfg that says `1` gets `2`. As it was first written: `1` runs it on the **device thread**: the render thread captures what the injection needs, posts it as a note in the worker's own stream and carries on, so the roughly 277 us it otherwise spends blocked inside DXVK's `FlushRenderingCommands` is spent on a thread that can absorb it. Measured with the seam pad: a 300 us block costs 0.277 ms of frame time on the render thread and 0.057 ms on the worker, so the predicted saving is about 0.20 ms a frame. `0` runs it inline exactly as before, which is the A/B and the way out. (`2` WAS a diagnostic when this row was written: it posted as `1` does and then waited for the worker. The wait came out on 2026-09-19 and `2` became the mode that wins.) Does nothing at `threaded=0`, because there is no worker to post to, and falls back to inline automatically whenever the layer is disarmed. Live. See `docs/DESIGN-INJECTION-ON-THE-WORKER.md`. |
| `threaded` | `1` | `2` (every shadow answer checked) is a development build's since 2026-10-31: a release clamps it to 1. ON BY DEFAULT since 2026-09-17 (the owner's decision: it is the largest CPU gain the mod has, measured as a gain even with the game held to four logical processors, and both of its shadows ran clean at `2` in sitting 4). `0` turns it off and is the first thing to try if the game misbehaves with the mod and not without it; it has not yet been run on an AMD card by this project. Below four logical processors it refuses by itself and says so. `1` moves DXVK off the game's render thread: every D3D9 call the render thread makes is recorded and run by DXVK on a second thread, in the same order, a moment later. The mod's own hooks still run on the render thread exactly as before, except the mover tracker, the pose store and the velocity batch, which run on the second thread while it is armed (stage 2a, 2026-09-18) and come back to the render thread whenever it disarms. Gets the game makes every frame are answered from a copy of the state the recorded calls set; anything else that needs an answer waits for the second thread to catch up. `2` does the same and checks every answer from the copy against DXVK, logging any difference: slower, and the way the copy is proved exact. The log says when it arms (300 frames after start and after each device reset), and at `log=2` every 600 frames how many calls were recorded and how long the render thread waited; at `log=1` it says only when that wait exceeds 2 ms a frame, which is more than the layer saves and the sign to set `threaded=0` on that machine. It refuses to arm, and says why, if another module has hooked DXVK's functions underneath this one, or if the process may run on fewer than four logical processors. `docs/THREADED-DEVICE.md` has the design. Needs a relaunch. |
| `mv_glass` | `3` | DEVELOPMENT BUILDS ONLY (the owner, 2026-09-30, the final audit's hidden-key review): a release build does not read it and runs `3`. **THE DEFAULT IS `3` SINCE 1.1** (the owner, 2026-10-11, live from the chase camera and in first person, `2` against `3`: "B is better that solves it"): a mover the game draws blended, or with depth writes off, is not redrawn into the velocity target, so what MOVES behind a vehicle's glass (its own cabin, traffic through a windshield) keeps its own motion vector. **CORRECTED THE SAME EVENING:** only a blended mover whose depth goes somewhere else than the depth the guide pass reads is left out (a pane: the game draws it into the scene's depth). A blended mover that writes the guides' own depth is redrawn: a ped's hair is drawn blended into the G-buffer, and the first form of `3` left it with the camera's vector wherever the head has nothing under it (the top of the owner's driver's head, torn off in a generated frame). **`4`, a development build's alone:** that first form, every blended mover left out, to see the difference. `2` was the default of 1.0.8 and 1.0.9 (the owner, 2026-10-02: `1` clearly wrong through glass, `0` and `2` both clean); it left out only movers drawn with depth writes off, in the belief that a pane writes none. A pane does, so `2` never left one out, and everything moving behind glass got the camera's vector. What follows is the history of `1`, the default from 2026-09-09 to 1.0.7. **THE ONLY DEV LEVER HERE THAT CHANGES THE PICTURE.** The guide pass takes an object velocity only when its stored view distance agrees with the scene depth to within two percent. That test has a SIGN and the two signs mean opposite things. If the stored surface is BEHIND what you can see, something opaque was drawn in front of it after the velocity pass ran, which is ordinary occlusion (a pole in front of a pedestrian) and the camera-vector fallback is the correct answer. If it is NEARER, that can only happen because it wrote no depth, which means a TRANSPARENT MOVER: vehicle glass, and wind-blown litter. Its own motion is the right answer there, and on a vehicle it is the same rigid body as everything behind it, so falling back to the camera told that band of pixels the car was not moving. `1` keeps the vector in that case. No threshold, the sign is categorical. Established by looking, with `debug=6`: the affected set is exactly glass and litter, the A/B moved exactly those pixels and nothing in the occlusion families, and an ordinary drive found no regression. `0` is the pre-2026-09-09 behaviour and exists for the same reason `reset_on_cut=0` does, to see what it looks like without. **`2` (2026-10-01, the windshield report; the default since 1.0.8):** "the same rigid body as everything behind it" holds for a car seen from OUTSIDE and for nothing else seen through glass. Players (a first-person mod, DLSS and FSR) and then the owner saw everything inside a vehicle's glass smear and lose its anti-aliasing when the camera moves: the world behind a windshield was given the windshield's motion. `2` leaves every mover the game draws with depth writes off OUT of the velocity pass (VelRecordObject), so the target keeps what is behind the glass: the cabin's own vector, whose depth agrees and needs no exception, another vehicle's, or none, which is the camera's vector for the standing world. Wind-blown litter is left out with it and falls back to the camera vector, as before 2026-09-09. REWORKED 2026-10-02 after the owner's Y legs: `0` was clean through glass and the first `2` was not, because it left out only movers that were blended AND rigid (about four depth-off movers a frame stayed in) and kept the rule of `1` on for them. Now `2` leaves out every such mover, blended or not, rigid or skinned, and turns the rule of `1` off for whatever still gets in: the picture of `0` through glass, plus the cabin's own motion where glass used to overwrite it. A development build counts such movers at every setting, by kind, and says so at log=2. Live. Roadmap B5. |
| `notice` | (none) | NOT IN THE SHIPPED CFG, ON PURPOSE (an override, not a setting; the owner, 2026-10-10). `notice=0` turns the on-screen notices off for the session, for someone recording; anything else leaves them on. The notices are the few warnings that are also said on the screen, top centre, for ten seconds, only when something is wrong (the mod could not start or stopped, HDR asked for and not on, DLSS missing for a cause that can be mended, a picture that is wrong, another mod in the way at start, two copies of the .asi, a cfg edit that needs a restart): `source/notice.h` has the rule and the list. The warnings file and the did-not-start note are written whatever this says. It holds no value in the cfg's table: the parser sets a switch of the notice's own. Off under Wine whatever it says. |
| `lod_bias` | `1` | DEVELOPMENT BUILDS ONLY (the owner, 2026-09-30, the final audit's hidden-key review): a release build does not read it. Texture mip bias. `1` (and any positive) AUTO: `log2(render / output)`, derived from the size the render scale actually achieved. `0` writes nothing at all, which is what every build before 2026-09-07 did and is the baseline half of the comparison. A negative value pins that bias in every mode, DLAA included. Live: save the file and the next frame carries it. |
| `mv_wheel` | `1` | A DEVELOPMENT BUILD'S KEY (2026-10-11): a release build neither parses it nor needs it, IT ALWAYS LEAVES THE SPIN OUT. **THE DEFAULT IS `1` SINCE THE EVENING OF THAT DAY** (the owner: "yes on the wheel default", after the frame generation sitting: "the wheels are much better, the ghosting from the wheels is almost completely gone"). `1` gives a tyre the car's motion and leaves its spin out of the motion vectors: the tyre as it is turned this frame, placed where its hub and its axle were last frame (`VelWheelNoSpin`, at the end of `source/DLSS-IV.cpp`). `0` keeps the spin, the picture until then, to compare: a tyre's vectors turn round its hub, up to 120 render pixels a frame at speed, which the interpolated frame cannot step along (a torn wheel in a generated frame) and which leaves the upscaler's real frames with speckled spokes. Live. The development log's `velocity: tyres that moved` line counts the tyres and says which of the mesh's axes was the axle (x, in the owner's game). |
| `fg` | `0` | BUILDS WITH FRAME GENERATION ONLY: development builds, the release with frame generation (`build.ps1 -Release -Fg`) and THE SHAPE 1.1 SHIPS (`build.ps1 -Ship`, package.ps1's default since round 8), whose shipped file carries the line `fg=0` (OFF BY DEFAULT, the owner's decision of 2026-10-25). The plain release (`package.ps1 -Plain`) does not know it: a file that names it there gets the unknown-key line. `1` asks for frame generation; `0` is frame generation off and the log says nothing of it; a value out of range is `0`, said in one line (`fg=N is out of range (0 or 1): 0 stands`). With `threaded=1`, `inject_worker=2`, `responsiveness=4` and the pacer on, on Windows native or under Wine and Proton on the game's own semaphores (`sync` -1 or 1), the helper is started with `--fg=1` and the log says once whether the host offers it; then the conditions decide at every window whether frame generation runs (source/fg/fg_cond.h, one line each, and the one that keeps it off goes into the warnings file). The video memory a set takes is said in the log; docs/reports/fg-5/opt-2026-10-25/MEMORY-AND-PRESENTER.md has the figures. Read once at load: a later edit needs a relaunch, and the log says so. A static condition that fails is said in one line (`frame generation: off for this session: ...`). With `fg_test=1` (development builds) the rig has the session and this key stands aside. |
| `fg_min_fps` | `0` | EVERY BUILD WHOSE FRAME GENERATION ARMS, THE SHIPPED ONE AMONG THEM (frame generation, phase 5; shipped since round 6, 2026-10-24, the owner's key of 2026-10-17): the law's floor in whole frames a second, `30` to `240`, on every display; `0` is THE FLOOR BY DISPLAY, 30 real on a 60 Hz screen (59.94 is one) and 38 on every other (40 until the owner's word of 2026-10-27: "stay on down to about 35 fps in heavy stretches"). Generation starts at it and stands down 3 under it. THE FLOOR FOLLOWS THE PANEL: the law holds it under its own top for the display (the top at 120 Hz, 67, scaled with the refresh, less its 3 of hysteresis) and says on its start line when it did, so a 240 Hz player may ask for 80. A player may lower it to `30` or raise it; its note is docs\reports\fg-5\SHIP-fg_min_fps.txt (whose `fg_min_fps=40` would hold 40 on a 60 Hz screen too). THE SHIPPED FILE CARRIES `fg_min_fps=0` since round 8 (the floor by display, under fg11-text's player text). Read once at load. |

### `lod_bias`, and why it replaced a line in `dxvk.conf`

**NOT USER FACING, and deliberately absent from the shipped `DLSS-IV.cfg`**
(owner, 2026-09-10). It ships at `1`, which is the built-in default and the right
answer in every mode, so naming it in the file a player reads would only invite
them to change it. It is documented here because this is the developer reference;
it appears in no shipped file, no `README.txt` and no public `README.md`, and it
should stay that way.

When the scene renders below display resolution every sampler needs
`log2(render / output)` of negative bias, or the texture detail the upscaler
exists to reconstruct was never sampled into its input. Too negative and it
samples detail below what the render resolution can represent: aliasing and
shimmer in the INPUT, which the temporal filter then has to fight.

    DLAA         3840 -> 3840    0.0000
    Quality      2560 -> 3840   -0.5850
    Balanced     2227 -> 3840   -0.7860
    Performance  1920 -> 3840   -1.0000
    Ultra Perf   1280 -> 3840   -1.5850

Filed HIGH by the 2026-09-03 audit and open until 2026-09-07. What stood in for
it was a hand-written `d3d9.samplerLodBias = -0.585` in `dxvk.conf`: correct for
Quality, wrong for the other four modes, and **absent entirely from the `GTAIV`
folder**, which is the one closest to a shipping install. Every player was in
that state. Requiring a player to hand-edit a DXVK config, correctly, per
quality mode, was never shippable.

**Do not set both.** DXVK ADDS its config bias to what the application sets, so
the two stack. The `dxvk.conf` line is commented out in both folders that had it.

Three things are measured and reported every 600 frames at `log=2`, in development builds:

  - the bias in force, the derived value, and what the cfg asked for
  - how many times the GAME set a bias of its own (composed with ours, not
    replaced: a bias the game sets is one it wants at the resolution it thinks
    it is rendering, and ours corrects for the one it actually is)
  - how many of a sample of DRAWS carried our value, read back through
    `GetSamplerState`, which is not hooked. Draws that do not are the ones
    where the GAME set a bias of its own on that sampler and ours was composed
    on top: the read-back holds the sum, not ours alone. The 2026-09-08 log
    showed exactly that (`-4.5850` against `-0.5850`, a difference of exactly
    `-4.0`), so a small mismatch count is the composition working, not a state
    block undoing it. The check comes out in the frame-generation audit's
    Phase 2, its question answered.

---

## FRAME GENERATION: REMOVED 2026-09-09

The mod had an FSR frame generation mode. It was removed by the owner's
decision under tenet 15, and `docs/POSTMORTEM-FRAME-GENERATION.md` is its
record, including the user-facing half of the cap and the pacer that used to be
documented here.

**Seven keys are gone from the parser**: `fg`, `fg_cap`, `fg_mv`, `fg_debug`,
`fg_flush`, `fg_guides_prev`, and `fg_mvtest` under that name. A cfg line
naming any of them is ignored, as it is for any key the parser does not know.
(Since 2026-10-05 `fg` is a key again in builds that carry the frame generation
revival; its row is in the development table above. No shipped release reads it.)

**One instrument survived the removal and was renamed with it, until 2026-09-19.** `fg_mvtest=1`
became `mv_selftest=1` (deleted since: see the note on the self-test above). The name changed
on purpose: the instrument is the motion vector self-test, it scores the
buffer both upscalers are handed, and carrying an `fg_` prefix would have
invited exactly the sweep that nearly deleted it. Its value `2`, the
presented-frame lag test, read the presented-frame shared slot and could not
survive; there is no replacement.

**One key under this heading was never frame generation** and has moved up into
the development lever table where it belongs: `rs_ds`, the render-scale
depth-stencil gate. (`chv_intz` was the other. It stopped being a key at all
when B13 closed on 2026-09-09: the overlay depth rule it switched is now
unconditional at any render scale, for DLSS and FSR alike, and there is no
setting because with it off the phone does not render.)

**Keys that no longer exist.** A cfg line naming any of these is ignored, which
is what the parser does with every key it does not know, so an old file keeps
working and simply stops doing the thing that is gone.

Frame generation's, all removed 2026-09-09: `fg`, `fg_cap`, `fg_mv`,
`fg_debug`, `fg_flush`, `fg_guides_prev`. Also `fg_mvtest`, which became
`mv_selftest` for its one surviving mode and was deleted with it on 2026-09-19.

Earlier development keys, removed with the code they switched on: `fg_probe`
(the flash detector, the brightness comparison, the sweep, the stamp, the BMP
dumps and real-frame-in-both-slots), `fg_test` (the double-present experiment),
and `subs_rule` (the design A read rule, settled: reading follows the latest
write, and there is no other rule).

Frame generation's development keys removed in round 8's deep strip (2026-10-25,
docs\INSTRUMENTS-FG.md, which says where each lived): `fg_test` and the
thirty-six `fg_test_*` keys of phase 1C's rig; the product's development keys
`fg_shadow`, `fg_shadow_pipelined`, `fg_shadow_every`, `fg_plant`, `fg_q_at_hand`,
`fg_host_args`, the twelve helper levers `fgtool_mv`, `fgtool_lag`, `fgtool_every`,
`fgmv_old`, `fghold_q`, `fgraise`, `fgraise_at`, `fgfail_at`, `fgvram_refuse`,
`fgslow_g`, `fgdie_g`, `fgslow_d`, and the armed development build's `fg_force`,
`fg_force_after`, `fg_slot`, `fg_lift`, `fg_hudless`, `fg_pace`, `fg_law`,
`fg_law_cut_ms`, `fg_law_fast`, `fg_fixed` (the key; the fixed mode itself ships),
`fg_test_fill` and `fg_test_fill_alt_ms`.

---

## The host has no file of its own

`host64/dlssiv-host.cfg` was deleted on 2026-09-06. It held two keys, `preset`
and `log`, and both now live in `DLSS-IV.cfg` and travel to the host on its
command line as `--preset=` and `--log=`. The host also no longer writes
`dlssiv-host.log` or `dlssiv-host.prev.log`: it appends into the client's log
through `--logpath=`, serialised by a named mutex, with each line prefixed
`host:`. One config, one log.

### The preset, and what NVIDIA actually says

Straight from `nvsdk_ngx_defs.h`, which is the authority rather than anyone's
recollection:

    J = 10   Similar to preset K. Slightly less ghosting at the cost of extra
             flickering. K is generally recommended over J.
    K = 11   Default preset for DLAA / Balanced / Quality modes, transformer
             based. Best image quality preset at a higher performance cost.
    L = 12   Default for Ultra Perf mode.
    M = 13   Default for Perf mode.
    A-D      removed, use J or K.   E, F deprecated.   G, H, I, N, O do not use.

So NVIDIA's own per-mode default is:

    DLAA         K        Quality      K        Balanced     K
    Performance  M        Ultra Perf   L

We currently set K on all six. That is CORRECT for DLAA, Quality and Balanced,
and WRONG for Performance and Ultra Performance.

**AND IT CORRECTS OUR OWN RECORD.** A note from 2026-09-02 said "M belongs in
an upscaled mode, which needs an internal render scale this chain does not
have", implying M for Quality/Balanced. The header says Balanced is K and M is
Performance only. The 09-02 note was half right, about M wanting a lower input
resolution, and wrong about which modes.

**THE HARDWARE CAVEAT: CONFIRMED, and it is not in the headers.** Researched
2026-09-04 after the owner raised it.

L and M are the SECOND-generation transformer models introduced with DLSS 4.5,
and they use FP8 precision. Ada Lovelace (RTX 40) and Blackwell (RTX 50) have
native hardware FP8 in their tensor cores, which roughly doubles inference
throughput and is what lets a heavier model run at a minor cost. Turing (RTX
20) and Ampere (RTX 30) tensor cores do NOT support FP8 inference, so both new
presets "greatly underperform" there. NVIDIA's own tuning targets are M for
Performance and L for 4K Ultra Performance.

AND THERE ARE NO OFFICIAL UI GUIDELINES FOR THIS YET. An NVIDIA developer
forum thread asking exactly this question ("how best to expose the transformer
model preset selection to the user") is answered with: presets L and M perform
poorly on older hardware, some games (Cyberpunk 2077, Escape from Tarkov)
already ship their own hardware-based preset defaults, and "there aren't any
official guidelines for this as of yet".

So every title doing this is inventing its own policy, and so are we. HOW those
games implement it is not public and should not be guessed at.

It means the per-mode table cannot simply be hardcoded:

    RTX 40 / 50      DLAA K, Quality K, Balanced K, Perf M, Ultra Perf L
    RTX 30 and below DLAA K, Quality K, Balanced K, Perf ?, Ultra Perf ?

The `?` entries are the open question. K is the safe answer (it works
everywhere, it is just slower); J is the other candidate. That needs measuring
on hardware we do not have, so the shipped default for pre-40-series should be
whatever is SAFE rather than whatever is best, with `preset` in the cfg as the
override, and NVIDIA's own app-level override available on top of that.

## HDR

Proven 2026-09-05 on an LG C1 with HGIG, on stock DXVK 3.0.2; the history and
every withdrawn conclusion are in `HDR-PLAN.md`. Requirements: Windows HDR on,
the game borderless (not exclusive fullscreen: under DXVK that drops the HDMI
link to SDR), stock DXVK. No fork, no extra DLL, no `dxvk.conf` entry
(`dxvk.allowFse` was tested off: still HDR). DLSS-IV requests an FP16 back buffer (unlocked through DXVK's own
`ID3D9VkExtInterface`), sets the scRGB colour space on DXVK's swapchain
interface, and renames DXVK's Vulkan engine string while HDR is on, because the
NVIDIA driver presents windows as SDR for the exact string "DXVK".

**FusionFix's own Antialiasing (FXAA/SMAA) under `hdr=1`** (2026-09-14, field
report R4, owner-verified as "HDR looks darker with SMAA"). Two separate
effects, both handled from this side now. The darkening: SMAA's final pass
reads its 8-bit target with an sRGB sampler and writes the back buffer with
sRGB write enabled; DXVK honours the read and drops the write on a float format
(A16B16G16R16F has no sRGB view), so under the FP16 back buffer the frame landed
already linearised and the encode decoded it a second time (mid grey at 15% of
its luminance, the HUD, drawn after SMAA, untouched). The mod suppresses the
sRGB read for exactly that draw, so the pair cancels as it does in SDR; the log
says so once. The second effect, the highlights: with any FusionFix post AA on,
the composite draws into FusionFix's off-screen 8-bit target first, so the
frame's values above white are clipped before the back buffer sees them, and
the frame's first back-buffer draw is the AA pass rather than the composite.
Since the owner's word of 2026-09-14 the mod notices that (the AA pass's
sampler 2 is not the pre-tonemap image), looks for the composite among the
draws before the first back-buffer draw (a screen-size 8-bit target with a
screen-size float texture on sampler 2), captures it there, and restores
highlights from the rebuilt composite (`hdr_source` auto resolves to 1), the
same path the LUT-on case uses. The log names each step. Unrun in the game:
TEST-PLAN item 3, including what a halo along SMAA's blended edges would look
like and the one-line fallback. DLSS-IV already supplies the anti-aliasing;
FusionFix's runs on top of the resolve, and under a render scale the off-screen
search is untested.

The shipped cfg carries FIVE HDR keys, and `cfg_live` only as a commented
line: `hdr=0` (HDR is opt-in), `hdr_peak=0` and `hdr_paper_white=203`,
`hdr_ui=0` and `hdr_rolloff=80`. Decided
2026-09-06 after the measured calibration (`HDR-CALIBRATION-GUIDE.md`,
`HDR-CALIBRATION-DESIGN.md` section 7). Everything else below is auto with an
expert override, an expert lever, or a diagnostic, and none of it is in the
shipped file. Removed the same day: the phase-2 model (`hdr_source=2`,
`hdr_scene_white`), the `hdr_knee` key (now a compiled constant, 0.90, in
`hdr_encode.h`) and `hdr_colorspace` (the fork era); a cfg line naming them
is ignored. `cfg_live` makes every `hdr_*` key apply within a second of
saving the file, logged as `cfg live: key old -> new`; keys that cannot
change while running are named as "needs a relaunch".

| key | default | what it does |
|---|---|---|
| `hdr` | `0` | `1` engages HDR: FP16 back buffer, linear scRGB out, borderless required. Not live. While HDR is engaged, a Reset that asks for exclusive fullscreen is held borderless (2026-09-17: the switch froze the game inside the graphics driver; not under Wine since the review of 2026-09-25, where the freeze does not exist and fullscreen is a borderless surface); a session that starts in exclusive fullscreen is left alone and runs without HDR until it goes borderless. UNDER WINE (Proton; 2026-09-25, `hdr_encode.h`, the Linux rule) fullscreen is allowed, and HDR engages when the session's HDR switch is on, read from DXGI's answer (which DXVK gives from that switch, not from the screen) or from `DXVK_HDR=1` / `dxgi.enableHDR` in `DXVK_CONFIG` directly; the display surface then decides at the first frame, and if it cannot show HDR the picture stays SDR and the warnings file says so. A Steam Deck needs Game Mode with HDR switched on; a desktop needs `DXVK_HDR=1` and either gamescope with `--hdr-enabled` or Wine's Wayland driver in an HDR session (`PROTON_ENABLE_WAYLAND=1`, Mesa 25.1 or newer): X11 and XWayland cannot show HDR, and when the display refuses, the warnings file names the Wine display driver the game ran on. `dxgi.enableHDR` named in `DXVK_CONFIG` wins over `DXVK_HDR`, as it does in DXVK. AFTER THE DISPLAY REFUSES HDR AT THE FIRST FRAME, the back buffer stays FP16 until the next resolution change, or until a launch with `hdr=0`; the picture is the normal SDR one, and since 2026-09-30 (the final audit's G15) the mod's per-draw HDR work stops with the stand-down instead of running for the session. After a Reset that FAILS, the old back buffer is kept (8-bit if HDR was off before it) and HDR stays off until a Reset succeeds. |
| `hdr_peak` | `0` | The display's peak in nits (HGiG MaxTML). `0` = what Windows reports for the display (DXGI MaxLuminance, which the Windows HDR Calibration app writes; this machine read 1499 before calibration and 800 after), 1000 if Windows reports nothing. Range 100..10000. Verify with `hdr_view=17`: the zebra stripes mark what reaches 0.98 of the peak. (The nit staircases that used to verify this, views 4 and 8, were deleted on 2026-09-19; the Windows HDR Calibration app is the way to set the peak itself.) `-1`, the automatic of five neighbouring keys, is taken as `0` (the final audit, 2026-09-30). |
| `hdr_paper_white` | `0` | Nits that SDR white becomes (and the interface, unless `hdr_ui` sets it separately). `0` = the OS SDR white level (Microsoft's documented form; the brightness the game already had in SDR on this desktop). 203 is the BT.2408 reference; 160 to 180 is the usual OLED comfort range. `-1` is taken as `0` (the final audit, 2026-09-30). |
| `hdr_eotf` | `0` | How the game's output is decoded: `0` gamma 2.2 (ships; RenoDX's Rockstar mods, RTX HDR and Special K all default to it), `1` sRGB piecewise (lifts shadows), `2` gamma 2.4 (BT.1886). Out of range is `0` (the final audit, 2026-09-30). |
| `hdr_restore` | `100` | Highlight restoration strength, 0 off .. 100 full: the luminance the game's tonemap clipped, put back from the pre-tonemap image. Strictly additive, hue-preserving. |
| `hdr_source` | `-1` | Where the luminance the game lost comes from. `-1` auto: `0` while FusionFix's tone mapping is off, `1` while it is on. `0` = the frame itself: under `hdr=1` the composite draws into the FP16 back buffer the mod requested and a float target does not clip, so the frame's own values above white ARE the game's unclipped output, blur, grain and bloom included. `1` = the game's composite (rage_postfx) rebuilt per pixel from its own inputs, captured at the composite draw (the pre-tonemap scene, the 1x1 adapted luminance, the bloom, FusionFix's LUT and the constants Exposure / ToneMapParams / deSatContrastGamma / ColorCorrect / ColorShift); needed when FusionFix's LUT compressed the frame before the mod saw it. |
| `hdr_console_gamma` | `-1` | WHICH console gamma FusionFix applies to the composite's output: `0` none, `1` the Xbox 360 curve (its `xbox_gamma.csv`), `2` the PS3 curve (added 2026-09-24: FusionFix's 2026-09 rewrite offers it as `ConsoleGamma = 2`, a plain 1.2 power with no clamp, which climbs faster than the Xbox 360 curve above white). The frame the mod reads already contains the curve, but the PRE-HUD COPY DOES NOT, so `hdr_ui` needs this answer on every path (the copy is taken on the wrong side of that pass, and `HdrUiCoverage` puts the curve back on the copy before differencing; 2026-09-10; the wrong curve of the two leaves a residual above white that makes the mask partial on the brightest highlights). The rebuilt composite (`hdr_source=1`, the LUT-on path) and the LUT profile apply the same curve to match. `-1` = detect it, from two sources: the absorb's PROOF of FusionFix's shader when it stands (`hdr_cg_absorb` on, no `hdr_view`), which is exact; otherwise a 256-pixel probe that compares the frame against the rebuild with no curve and with each curve, every 600 frames once it has an answer and every 120 before that (no stall), which stands down for 1200 frames (ten seconds at 120 fps) after the proof last spoke, and at once when the absorb sees FusionFix draw with a shader other than the proved one or its pass stop for sixty frames (a curve switched in its menu, console gamma switched off). The curves are close, so the probe separates them only on clean frames; where it can rule out "none" but not tell the curves apart it keeps the curve it has. Logged as `console gamma in the HDR model: <curve> (<source>)` on every change, at log=1. `0`, `1`, `2` force it. (The HDR meter, whose reproduction check used to report both assumptions, was deleted on 2026-09-19.) |
| `hdr_ui` | `0` | UI BRIGHTNESS: nits for the interface, meaning the HUD, the map, subtitles and the menus (BT.2408 graphics white). `0` = the same as paper white, where graphics belong; a number sets them independently of the scene; `-1` disables the whole mechanism. Whatever the level, this is what stops the HUD RIDING THE SCENE: the game draws its 2D pass into the same back buffer after the composite, so a HUD pixel blended over a clipped sky used to carry the sky's above-white values and be restored with it. The mod copies the back buffer once, at the first draw after the composite, and withdraws the scene's restoration wherever that copy and the final frame differ. The copy is compared THROUGH FusionFix's console gamma, because that curve is its own full-screen pass and the copy is taken before it; without that, the difference over the scene is the curve itself and the whole picture reads as interface (fixed 2026-09-10, and `hdr_console_gamma` is what the correction is keyed on). The tolerance that decides "these differ" scales with the pixel's own level above white, because the residual it rejects is a fraction of that level, and a fixed one masked everything brighter than about 1.5x white: headlight interiors, and nothing else. Cost: one full-res copy per frame, and only while the 2D pass is actually drawing. With the HUD turned off in the game's own settings nothing draws after the composite, no copy is taken, and it costs one integer test; it re-arms by itself when the HUD returns. WHILE A MENU OF THE GAME'S IS UP (the pause menu with its map, the front end) the whole frame is treated as interface, off the game's own menu flag: a translucent full-screen layer is what a difference cannot mask, and the scene's outlines used to print through it (2026-09-19). On a build where that flag is not found the log says so and the menu is masked by difference as before. `hdr_view=14` is the mask, and paints a menu frame green all over. A value from 1 to 49 is raised to 50 and one above 1000 lowered to 1000, paper white's range (the final audit, 2026-09-30: `hdr_ui=5` was a near-black HUD). |
| `hdr_cg_absorb` | `1` | THE ABSORB (2026-09-18, ROADMAP 9; DEFAULT 1 after a clean field session, 66,287 frames withheld with every compare exact and no remedy; `0` is the way out). FusionFix's console gamma pass is a copy of the whole back buffer into a texture of its own and one full-screen draw that samples it through the curve; with `hdr=1` the owner measured it at about 0.37 ms a frame at 4K, and the HDR encode then copies and draws the frame again. `1`: once FusionFix's pixel shader has been PROVED, its draw is withheld, the encode samples FusionFix's copy instead of making its own, and applies the identical curve and dither at that one sample: one copy and one draw a frame where there were two and two. The proof assumes nothing about FusionFix's version: on frames where its pass runs untouched, a 256-pixel pass computes the Xbox 360 curve and the PS3 curve, each with and without FusionFix's dither, from FusionFix's input and compares them with its real output to half-float rounding; exactly one fitting on every pixel, three checks running, proves that shader object, one frame in 600 is compared again for as long as the session lasts, and a failed compare withdraws the proof at once. An unknown shader never proves and FusionFix's pass runs as it always did. If anything writes or reads the back buffer between the withheld draw and the encode, the curve is first drawn into it by the mod (the log counts why), and four such frames in a row hold the absorb off for 600. `2`: recognise and compare only; nothing is withheld, the log carries every compare. (A `3` stood here, the mod's own A/B of the absorb behind `DLSSIV_CGA_DEV`. It came out of the source with its answer, 0.190 ms a frame, and 3 is out of range in every build now.) Does nothing with `hdr=0` or under `hdr_view`. (Until 2026-09-19 it also stood down whenever `sharpness` was above 0; it no longer does, `hdr_cga.h` says why.) Live. |
| `hdr_rolloff` | `80` | Percent of the ceiling (peak / paper white) above which restored highlights follow an exponential max-channel shoulder toward the peak; below it they are sent at their true brightness. `0` = hard clip at the ceiling. 80 ships because the measured content sits under it: with the LUT off, daylight reaches 1.0 to 1.6x white and night emitters 4 to 6x, against a 3.94x ceiling on an 800-nit panel at 203 nits paper white, so 50 (RenoDX's reference) dimmed a 700-nit sun to 611 for no gain. The shoulder only ever removes brightness; lower is softer. |
| `hdr_view` | `0` | DEVELOPMENT BUILDS ONLY since 2026-10-31: a release parses it, clamps it to 0 and carries no debug shader. THREE VIEWS (debug shader, created only while set; every other number reads as 0 and the log says it was out of range). The other twelve, the staircases, the patterns, the capture checks, the gain, reproduction and headroom maps, were DELETED on 2026-09-19 with the rest of the development-only instruments (the owner's decision) and are in git at `b3f6a492` and earlier. `6` false colour by presented nits, `14` the UI mask (green where the mask is full, and a whole green screen while a menu of the game's is up; a dark red to orange ramp where it is partial, which is a fault; magenta when no mask was made), `17` THE HIGHLIGHT MAP (2026-09-17, for timecycle authoring): the real picture below paper white, exactly as `hdr_view=0` presents it; above it one continuous green, yellow, orange, red, magenta scale spaced in stops between paper white and THIS display's peak, emitted at 1.25x to 2.5x paper white so highlights stand out from the picture, dark contour lines at 400, 600 and 1000 nits, zebra stripes at or above 0.98 of the peak, and the top 3 percent of the screen is the legend (paper white at the left edge, the peak at the right). |
| `cfg_live` | `0` | Re-read `DLSS-IV.cfg` about once a second when it changes and apply the live keys, so most settings can be tuned without a relaunch. One file-attribute query a second. **Default changed from 1 to 0 on 2026-09-09** (owner's call): most people set the file once, so the check should not be on for them. **Turn it on while calibrating HDR:** `hdr_paper_white`, `hdr_peak`, `hdr_rolloff` and `hdr_ui` all apply live, and the calibration guide's procedure depends on it. `hdr` itself and `quality` still need a relaunch and say so in the log. The shipped `DLSS-IV.cfg` carries a commented `#cfg_live=1` with that explanation. |
| `hdr_engine_name` | `3` | The engine string DXVK gives the driver while HDR is on, and in SDR on NVIDIA (see `rename_sdr`): `3` NVIDIA's own RTX Remix name "DXVK_NvRemix" where an NVIDIA GPU drives the display, or renders on a hybrid in SDR (see `rename_sdr`; a hybrid with HDR on gets "DXVK-HDR"), else "DXVK-HDR" (the default since 2026-09-29: it keeps NVIDIA's DXVK speed paths, about 9 percent of GPU time the other names lose, and still gets borderless G-Sync; sitting 18, docs/DECISIONS.md), `2` "DXVK-HDR", `1` "D3D9 on Vulkan (HDR)", `0` leave "DXVK" (the window is then presented as SDR). Windows only: under Wine the name always stays "DXVK", because Mesa's drivers and gamescope key on it (2026-09-26). Why the name matters on NVIDIA: docs/reports/engine-rename-2026-09-26.md. Out of range (`-1`, or above 3) is `3`, the shipping default (the final audit, 2026-09-30: `-1` silently left HDR on NVIDIA presented as SDR). |
### Old names (DELETED 2026-09-30)

render_quality (renamed quality on 2026-09-06), hdr_ui_white (renamed hdr_ui on 2026-09-08) and backend as a cfg key (the
file has said upscaler since 2026-09-06) were accepted as old names until the final audit; all three predate the first public
release, and the owner took them out. A cfg that carries one gets the unknown-key line. upscaler is the key; backend stays the
field's name and the wire term (`--backend=`, FEED_ACK_BACKEND_FSR).
