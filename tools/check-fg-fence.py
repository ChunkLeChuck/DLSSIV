r"""FRAME GENERATION'S FENCE, READ FROM THE SOURCE (2026-10-05, frame generation phase 2, stage 1; docs/DECISIONS.md "PHASE 2,
STAGE 0", A2; docs/reports/fg-2/map-2026-10-05/check-machine.md, O8).

Proof B (tools\prove-fg-off.py --ship since round 8's deep strip retired the third build's --fg) compares the shape that
carries frame generation with the pinned base and prints what DIFFERS. It cannot see a
line that is MISSING: a release-compiled function that lacks its release of a wait reads as the base, and passes. Phase 2
adds three waits across the process boundary, each the shape of this project's one permanent freeze, and each is safe only
with its release. So two things are read from the source itself:

  1. MUST-FIND. Every row of SITES names a file, a function and a piece of text that must be in that function's body, in
     PRODUCT code: inside an `#if DLSSIV_FG` block, or on a line inside an `FGX(` wrapper. The rule the rows enforce: every
     guard of a wait sits behind the same switch as the wait. THE TABLE IS EMPTY AT STAGE 1: nothing is behind the switch
     yet. Each stage that wires a wait adds the rows of its guards in the commit that wires it (stage 10, stage 11), and
     MIN_SITES is raised with them, so the table cannot be emptied without the gate seeing it.
  2. NEVER RETURNS. The names the plan lists as never returning (docs/FG-REVIVAL-2026-09-24.md, section 4: the
     second-thread borrow and its writer guard, the shelved keys, the unreset client frame id, FG_ACTIVE at bit 8) are in
     no source or helper file outside comments.

The scan drops comments (string literals stay: a shelved cfg key would come back as one), follows #if / #elif / #else /
#endif for the switch, and finds a function's body by its name and its braces. CONTROLS, every run, on text in memory: a
site that is there is found; the same site with its line deleted FAILS; the same line outside the switch FAILS; a
never-returning name planted in code FAILS, and the same name in a comment does not.

    python tools\check-fg-fence.py
"""
import os, re, sys
import fg_machine_text   # (fg_machine.h is in parts included in place: it is read with them pasted back, tools/fg_machine_text.py)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# (file relative to src\DLSS-IV, the function's name, the text that must be in its body in product code, why,
#  and since the review of stages 5 to 7 a dict of what else is held:
#    "count": how many times the text must be there (IvtTick stands the set down at two places: one row, two wanted);
#    "before": a text of the same body that the site must come BEFORE (a drop below the release it guards is no guard);
#    "product_file": the file is included only behind the switch, so its lines carry no #if of their own; main() holds
#                    that include to the switch instead)
SITES = [
    # STAGE 7 (2026-10-05): the set's life around the build. A set of frame generation's is helper-made memory the game
    # holds through imports, so each of these is a place where the imports must go, or must stop being used, and Proof B
    # cannot see one that is missing.
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtBuild", "FgxBeforeRelease();",
     "the game's imports of the set go at the build's head, before the main set's release and the build message that frees the helper's side",
     {"before": "IvtReleaseImages();"}),
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtBuild", "if (!FgxAfterBuild(w, h, (int)bbf, rw, rh)) { IvtReleaseImages(); return false; }",
     "an exchange that left the pipe out of step stops the build and gives the main set back, as every failure arm of the build does",
     {"before": "ivtBuilt = true;"}),   # (the rig's install, FgRigInstall, held this place until round 8's deep strip took the rig out)
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtFail", "FgxStandDown();",
     "a stopped transport's set is not one to use", {}),
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtOnReset", "FgxStandDown();",
     "a device Reset makes the set stale until the next build asks again", {}),
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtTick", "FgxStandDown();",
     "a rebuild owed for the scene buffer or the render scale makes the set stale at once, not at the build that may wait: BOTH places",
     {"count": 2}),
    # THE PLATFORM TEST'S CALLERS (the review of stages 5 to 7): tools/test-fg-eligible proves the two pure functions over
    # every case, which says nothing unless the product ASKS them with the three facts, and every later reader takes the
    # verdict that was latched with the key.
    (os.path.join("source", "fg", "fg_client.h"), "FgxWhyNotStaticNow", "FgxPlatformOk(ivtWine, ivtSyncMode, ivtBridge)",
     "the static verdict asks the platform test with the session's three facts", {"product_file": True}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxHelperArg", "fgx.staticWhy = FgxWhyNotStaticNow();",
     "the helper's flag is decided by the static verdict, latched there", {"product_file": True}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxMarkerWanted", "FgxWhyNotStatic() == nullptr",
     "the marker under a limiter is wanted only under the static conditions", {"product_file": True}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxAfterBuild", "if (FgxWhyNotStatic() ||",
     "no set is asked for outside the static conditions", {"product_file": True, "before": "FgxAcquire<FgxB>("}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxOnCaps", "FgxPlatformOk(ivtWine, ivtSyncMode, ivtBridge)",
     "a session elsewhere than Windows native gets its one line and nothing else", {"product_file": True}),
    # STAGE 10 (2026-10-06): THE RELEASE OF THE GAME'S QUEUED WAIT ON THE HELPER'S PREPARE, written and held here BEFORE the
    # wait is wired (stage 11). A wait-only batch behind fgPrep is the shape of the project's one permanent freeze, and
    # each of these is a place where a stuck one must be let go. Proof B cannot see one that is missing.
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtFail", "FgxReleasePrepWait();",
     "a stopped transport releases frame generation's queued wait beside its own, BEFORE the pipe closes",
     {"before": "if (ivtPipe) { CloseHandle(ivtPipe); ivtPipe = nullptr; }"}),
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtHostWatch", "FgxReleasePrepWait();",
     "the watch releases it when it condemns the helper for a frame, and keeps releasing until it is stopped: BOTH places",
     {"count": 2}),
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtHostWatch", "if (FgxWatchLook()) { FgxWatchFire(want); fired = true; continue; }",
     "the watch has its term for a prepare that stands still: without it a helper hung in a prepare is never condemned",
     {"before": "const UINT64 done = FeedVkTimelineValue(&ivtVk, ivtSemOut);"}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxWatchFire", "TerminateProcess(ivtWatchHost, 0)",
     "the helper is ended BEFORE the fence is signalled: the kernel will not move a fence shared with a frozen process",
     {"product_file": True, "before": "FgxReleasePrepWait();"}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxWatchFire", "IvtSignalOutTo(want + 1, &did);",
     "a helper condemned for a prepare has the frame's wait released too", {"product_file": True}),
    # STAGE 11 (2026-10-06): THE PRODUCT IN THE MOD. The wait is wired now: the worker queues it before an injection's body
    # (the machine's rule, the client's lines), so every guard of the machine's life in the game is held here.
    # (source/fg/fg_product.h is included by fg_client.h alone, a file that is itself included only behind the switch.)
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtFail", 'FgpStop("the transport stopped", false);',
     "a stopped transport closes frame generation's machine (its writes are refused, no new wait is queued) BEFORE the waits are released",
     {"before": "IvtReleaseQueuedWait();"}),
    (os.path.join("source", "DLSS-IV.cpp"), "hkReset", "FgpOnResetBarrier(rsHow);",
     "before the game's Reset reaches DXVK the machine is dead if the Reset goes ahead of the queue, and its copies have retired",
     {"before": "HRESULT hr = origReset ? origReset(dev, pp) : D3DERR_INVALIDCALL;"}),
    # (2026-10-28, the core audit's finding 1; tools/perfproof/dt_fg_corefix.h, HU14) AND BEFORE THE MOD'S OWN TEXTURES GO: the encode's second
    # target (FgHudTargetGive(true), inside HdrEncodeRelease) is named by the hand's raw copy into the helper's HUDLESS, which DXVK does not
    # track, and the layer is disarmed there, so the target's own wait (inside a direct scope) does not run. FAILS on a19c680b.
    (os.path.join("source", "DLSS-IV.cpp"), "hkReset", "FgpOnResetBarrier(rsHow);",
     "the machine's retire wait runs before HdrEncodeRelease gives back the encode's second target, which the hand's raw copy reads",
     {"before": "HdrEncodeRelease();"}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxWatchFire", "TerminateProcess(ivtWatchHost, 0)",
     "and BEFORE the frame's own wait is released too: that signal is the one the transport measured as not returning for a frozen helper",
     {"product_file": True, "before": "IvtSignalOutTo(want + 1, &did);"}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxBeforeRelease", "const bool retired = FgpBeforeRelease();",
     "the machine's quiesce and its retire wait run before the set's images go", {"product_file": True, "before": "FgxDrop<FgxB>(&fgx.c->set);"}),
    (os.path.join("source", "fg", "fg_client.h"), "FgxBeforeRelease", "if (!retired) fgx.c->inFlight = true;",
     "images whose copies did not retire are forgotten, never destroyed", {"product_file": True, "before": "FgxDrop<FgxB>(&fgx.c->set);"}),
    (os.path.join("source", "fg", "fg_product.h"), "MayEngage", "ivtCfg.inject_worker == 2",
     "no present is engaged with the injection off the worker: the sent count would run ahead of a hand that runs late", {"product_file": True}),
    (os.path.join("source", "fg", "fg_product.h"), "PrepGuard", "FgxPrepWait<FgxB>(",
     "the one GPU wait is queued by the client's own lines, which store its top where the release reads it", {"product_file": True}),
    (os.path.join("source", "fg", "fg_product.h"), "FgpBeforeRelease", "FgPM::BeforeRelease();",
     "no pair stays open across the build message, and the build generation moves", {"product_file": True, "before": "FgPA::RetireWait(2000)"}),
    # THE GATE BEFORE DXVK'S PRESENT (2026-10-06, the freeze of phase 2's first sitting; docs/DECISIONS.md): both of the
    # machine's forwarded presents call the binding's gate, and the product's gate is the client's own line
    (os.path.join("source", "fg", "fg_machine.h"), "WorkerPresent", "B::PresentGate();",
     "no present is handed to DXVK before the gate has looked: at most one present stands behind an open wait on the helper", {"product_file": True, "count": 2}),
    # SINCE THE FOLD OF THE RELEASE LINE'S GATE (2026-10-10; docs/DECISIONS.md): THERE IS ONE GATE. The product's is a call of
    # the release's core on the release's one state (the row that named FgxPresentGate<FgxB>( until then), with the prepare
    # fence and its top as the second need and the going-down flag as the core's pointer; both needs are put back with
    # vsync off (the release's wrapper puts back one: it never sets the other); after the transport's stop no prepare need
    # is handed on; and the machine's install takes DXVK's bare pointer from behind the release's thunk, so that no present
    # passes two gates and none passes a gate under the fire's lock
    (os.path.join("source", "fg", "fg_product.h"), "PresentGate", "IvtGateCore(&ivtGate, &ivtVk, ivtSemOut, sentOut, c->set.semPrep, prepTop, FgxB::GateDown());",
     "the product's gate is the release's core on the release's ONE state, the core the fake client proves on real fences: a state of its own would let the machine's first present out unchecked",
     {"product_file": True}),
    (os.path.join("source", "fg", "fg_product.h"), "PresentGate", "if (ivtGate.prevPrep) ivtGate.prevPrep = 0;",
     "with vsync off the prepare fence's need is put back too: nothing else resets it, and a later switch to vsync would hold a present on a stale one",
     {"product_file": True, "before": "IvtGateCore("}),
    (os.path.join("source", "fg", "fg_product.h"), "PresentGate", "const uint64_t prepTop = stopped ? 0 : (uint64_t)(ULONG)c->waits.prepTop;",
     "after the transport's stop the gate is handed no prepare top: the next present has no need on a fence nobody will signal again",
     {"product_file": True, "before": "IvtGateCore("}),
    (os.path.join("source", "fg", "fg_product.h"), "DxvkBare", "(p == (void*)&IvtGatedPresent && ivtGateDxvkPresent) ? ivtGateDxvkPresent : p",
     "the product's binding hands the machine DXVK's own pointer from behind the release's thunk", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "Install", "s.ptr.dxvkPresent = B::DxvkBare(dtScPresentO);",
     "the machine calls DXVK's bare Present: with the thunk left in its chain a present would pass two gates, and a fire's would wait under the fire's lock",
     {"product_file": True, "before": "dtScPresentO      = (void*)&WorkerPresent;"}),
    (os.path.join("source", "devthread", "dt_install.h"), "DtReport", "&& !fgEngaged",
     "the device thread's threaded=0 advice is not given for a window in which a present was engaged", {}),
    # PHASE 3, LEVER 3 (2026-10-06; docs/reports/fg-3/wire-2026-10-06/specification.md, part 1): the hand's one span carries
    # the signal of fgIn in its one submit, and the guard before an injection's body is CALLED, before the saved observer,
    # for the last 'Q' written. (Until these rows the PrepGuard row above passed with the guard's call deleted.)
    # (round 9's strip: the shadow shape's span, ShadowSpan, went with that shape; the merged hand's, ArmedSpan, is held below. The
    # guard is called inside ArmedBeforeInject, which InjectObs calls before the saved observer: held below too)
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowBeforeInject", "B::PrepGuard(x.w.lastQN)",
     "the guard names the LAST 'Q' written, an earlier injection's: one that named this frame's own would wait for its own input", {"product_file": True}),
    # PHASE 3, THE WIRE'S ONE REVISION (2026-10-06; specification.md, part 2): a helper of the wire before this one is
    # refused in one line and is sent nothing ('Q' is 81 bytes now, and it would read 73)
    (os.path.join("source", "fg", "fg_client.h"), "FgxOnCaps", "(caps & FEED_CAP_FG_WIRE1)",
     "a host that speaks frame generation's wire 1 gets one line, not a message it would read by the wrong layout", {"product_file": True}),
    # PHASE 3, LEVER 2 (2026-10-10; docs/reports/fg-3/lever2-2026-10-10/DESIGN.md): 'Q' IS WRITTEN BEHIND THE INJECTION'S
    # BODY. The order that makes that safe is held here line by line: the guard, then the saved observer (the frame
    # message and the frame's submits), then the new writer; inside the writer the settle and the two tests before the
    # write; and every place that must close a 'Q' whose frame never reached a hand.
    (os.path.join("source", "fg", "fg_machine.h"), "InjectObs", "s.ptr.prevInject(kind, payload, bytes);",
     "'Q' is written BEHIND the saved observer: the frame message is on the pipe ahead of it and nothing of it is on the frame's path", {"product_file": True, "before": "ShadowAfterInject();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "InjectObs", "ArmedBeforeInject(payload, bytes);",
     "the guard (inside ArmedBeforeInject) runs before this frame's 'Q' exists, so it can only name the injection before: one that named its own would wait for its own input", {"product_file": True, "before": "ShadowAfterInject();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowAfterInject", "ShadowCloseEarly();",
     "a 'Q' with no hand is closed CLOSED before the next 'Q' is written: the helper counts a 'Q' read with one open as DRIFT", {"product_file": True, "before": "B::WriteQ(sn, qf);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowAfterInject", "if (!sent || !x.e.engaged) return;",
     "no 'Q' for an injection that was not sent, and none unless the last present was engaged: standby spends nothing", {"product_file": True, "before": "B::WriteQ(sn, qf);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowAfterInject", "x.sh.stopped || s.sh.state != FG_LIVE",
     "nothing is written behind an injection once the transport stopped or the machine is not live", {"product_file": True, "before": "B::WriteQ(sn, qf);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowHand", "if (!early) ShadowCloseEarly();",
     "a 'Q' of another frame is closed before the hand writes or adopts one", {"product_file": True, "before": "B::WriteQ(sn, qf);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowHand", "x.e.engaged = 0; ShadowCloseEarly();",
     "a hand that is refused, or that finds the set ended, closes the 'Q' its frame's injection wrote: BOTH places", {"product_file": True, "count": 2}),
    # (the shadow shape's close note, NoteCloseOpen, went with it in round 9's strip; the merged path closes by its fire note's kind)
    (os.path.join("source", "fg", "fg_machine.h"), "BeforeRelease", "ShadowCloseEarly(true); x.e.engaged = 0;",
     "no 'Q' stays open across the build message", {"product_file": True, "before": "_InterlockedIncrement(&x.sh.bgen);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "OnDisarm", "ShadowCloseEarly(); B::X().e.engaged = 0;",
     "a disarm closes a 'Q' with no hand (owed off the worker), and the first frame after the re-arm writes its 'Q' at its hand", {"product_file": True}),
    # PHASE 3, STAGE 7 (2026-10-10; docs/reports/fg-3/merge-2026-10-10): THE MERGED PATH, a binding that arms on the wire.
    # Compiled by a harness binding alone today; the day the product says it can arm, each of these is a line whose
    # absence no compare can see: an order, a guard, or a read that must come before what it protects.
    (os.path.join("source", "fg", "fg_machine.h"), "InjectObs", "if constexpr (B::kWire) ArmedBeforeInject(payload, bytes);",
     "the merged path's order before an injection's body runs BEFORE the saved observer", {"product_file": True, "before": "s.ptr.prevInject(kind, payload, bytes);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedBeforeInject", "ArmedFire(FG_PT_C, (DWORD)((payload - 2) - dtRing), wait);",
     "a pair still armed is brought to its real frame BEFORE the guard queues its wait: behind it the real frame would stand behind the helper's prepare", {"product_file": True, "before": "ShadowBeforeInject();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedBeforeInject", "if (wait && a.w.blitOut) { ArmedFlush(); DEVSTAT(a.w.guardFlushes++;) }",
     "DXVK's stream is flushed between a present and the wait: a Present returns before its blit has reached the queue, and the wait's batch would land ahead of it", {"product_file": True, "before": "ShadowBeforeInject();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedSpan", "B::RecSignal(FG_TL_IN",
     "the merged hand's one submit signals fgIn: a DISPATCH 'G' with no signal leaves the helper's queue waiting", {"product_file": True, "before": "took = B::RecSubmit();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "if (isI && B::TlValue(FG_TL_OUT) != 2ull * (ULONGLONG)(ULONG)nI) return FG_AF_NOANSWER;",
     "the answer is read BY EQUALITY, on the worker, before anything of the generated frame's fire: another pair's image, no frame, no answer and a helper that is gone show nothing", {"product_file": True, "before": "B::RecCopyFromImg(srcImg, bbNow);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "B::PresentGate();",
     "a fire's present passes the gate WITH NO LOCK HELD: the call stands before the fire takes its lock", {"product_file": True, "before": "Lock();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "if (!wrote)",
     "a write the queue did not take ends the fire BEFORE its Present: nothing is presented and nothing restored", {"product_file": True, "before": "const HRESULT hr = dxvk(w.sc"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedPresent", "if (FG_PRESENT_GATE_ON(1)) B::PresentGate();",
     "the merged path's inline present passes the gate before DXVK's Present, as the shadow shape's does", {"product_file": True, "before": "const HRESULT hr = dxvk(sc, src, dst, wnd, dirty, flags);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowHand", "ArmedFire(FG_PT_HANDOPEN, own, false);",
     "a pair found armed is fired BEFORE this hand's copy into HOLD, which is where that fire reads its real frame from", {"product_file": True, "before": "if (!ArmedSpan(own, !nosource, pass))"}),
    (os.path.join("source", "fg", "fg_machine.h"), "BeforeRelease", "if (w.openPhase == FG_PAIR_ARMED) ArmedDrop(FG_AD_BUILD);",
     "no pair stays armed across the build message: its fire would read the new set's images", {"product_file": True, "before": "_InterlockedIncrement(&x.sh.bgen);"}),
    # STAGE 7B, the merged path's placer: two orders inside one function, each of which is a ruling (R6)
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedPlacerStep", "L.tN = L.tI + (LONGLONG)(h * (double)B::Qpf() / 1000.0);",
     "the real frame's time is fixed from the handoff BEFORE the answer's fence is read: its due does not move with the answer", {"product_file": True, "before": "const ULONGLONG out = B::TlValue(FG_TL_OUT), n2 = 2ull * (ULONGLONG)(ULONG)L.n;"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedPlacerStep", "if (out == n2) why = -1;",
     "the placer says SHOW for exactly its pair's 'a frame' value, read from the fence itself: no frame, a later pair's answer and the top show nothing", {"product_file": True, "before": "DueWrite(s.dueI, L.p, L.gen, FG_DUE_SHOW, 0, tShow,"}),
    # STAGE 7C (2026-10-10), THE LIFE: what a fire reads AGAIN behind the gate, and in which order; a dead machine's pair at
    # the hand; the stand-down at a lost restore; the first pair behind a re-arm and behind a build
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "_InterlockedExchange(&s.sh.fireBusy, 1);",
     "behind the gate the fire says it is busy BEFORE it reads the machine's state again: a Reset that kills the machine waits out a busy fire, so one of the two sees the other",
     {"product_file": True, "before": "if (FG_ARMED_GUARD_ON(0) && !ArmedMayFire()) { _InterlockedExchange(&s.sh.fireBusy, 0); return FG_AF_DEAD; }"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "if (FG_ARMED_GUARD_ON(0) && !ArmedMayFire()) { _InterlockedExchange(&s.sh.fireBusy, 0); return FG_AF_DEAD; }",
     "a machine that died while a present was held in the gate fires nothing: the state is read again BEHIND the gate and BEFORE the fire's lock and its writes", {"product_file": True, "before": "Lock();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "if (FG_ARMED_GUARD_ON(1) && isI && B::TlValue(FG_TL_OUT) != 2ull * (ULONGLONG)(ULONG)nI) { _InterlockedExchange(&s.sh.fireBusy, 0); DEVSTAT(a.w.gateRead++;) return FG_AF_NOANSWER; }",
     "the answer is read by equality AGAIN behind the gate, before the copy out of GEN: a helper that went while the present was held shows nothing", {"product_file": True, "before": "B::RecCopyFromImg(srcImg, bbNow);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "B::PresentGate();",
     "the two reads stand BEHIND the gate, which is what can hold for seconds", {"product_file": True, "before": "if (FG_ARMED_GUARD_ON(0) && !ArmedMayFire())"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "if (lost && FG_ARMED_GUARD_ON(2)) Kill(",
     "a restore the queue did not take stands the machine down for the session", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowHand", "if (w.openPhase == FG_PAIR_ARMED) ArmedDrop(FG_AD_DEAD);",
     "a hand leaves no pair armed on a machine that fires nothing: dropped, counted, BEFORE the hand's own publication", {"product_file": True, "before": "ShadowWriteOwed();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "SwapPresent", "if constexpr (B::kWire) { if (r.wasEngaged && r.lastGen != gen) r.wasEngaged = false; }",
     "the first present behind a re-arm is an engage's first (F34): decided BEFORE the want and the facts are", {"product_file": True, "before": "LONG want = s.wt.want;"}),
    (os.path.join("source", "fg", "fg_machine.h"), "BeforeRelease", "s.r.wasEngaged = false;",
     "the new set's first pair is an engage's first where no unengaged present lies between (F34's other edge)", {"product_file": True}),
    # STAGE 7D (2026-10-10), THE WANTS, THE TAIL, THE LEAD: a line whose absence shows a frame the want did not ask for, or
    # sleeps the game where nobody may, or lets a refused key act
    (os.path.join("source", "fg", "fg_machine.h"), "SwapPresent", "const bool engaged = WantsWire(want);",
     "a present records a hand only for a want that speaks the wire: CAP paces and writes nothing", {"product_file": True, "before": "if (DtNote(DT_NOTE_FG_HAND, &h, sizeof(h)))"}),
    (os.path.join("source", "fg", "fg_machine.h"), "SwapPresent", "if (!(WantsPace(want, engaged) && s.sh.state == FG_LIVE))",
     "the lean merged shape's present that is not paced is the original present and nothing else: the test stands BEFORE the sleeper count and the clock are read", {"product_file": True, "before": "const LONG sleptBefore = B::PaceSleptN();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowAfterInject", "!WantsWire(ReadAcquire(&s.wt.want))",
     "no 'Q' is written behind an injection's body for a want that does not speak the wire", {"product_file": True, "before": "B::WriteQ(sn, qf);"}),
    # (2026-10-14, the helper's answer on a card that is the limit: the row read "nosource = true" until then. A binding that
    # arms still never asks lever 4's measuring key; what it asks instead is ArmedOverMay, and the two rows behind this one
    # hold the two tests of it that stand between a copy and a dispatch that still reads)
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowHand", "if (left) nosource = !ArmedOverMay(h, sn, left, early, realCut);",
     "a pair behind a dispatch in flight has no source unless ArmedOverMay says the card's order holds its copy behind that dispatch (lever 4's measuring key went with the shadow shape in round 9's strip)", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedOverMay", "if (B::Pipelined() || !B::HomeWaited(sn)) return false;",
     "a pair copies over a dispatch in flight only where THIS frame's home was submitted behind its wait for the helper: the fact the card's order rests on", {"product_file": True, "before": "aw.overPrev = x.w.pend[0];"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedOverMay", "if (!a.k.over || left != 1 || x.w.pend[0] != sn - 1) return false;",
     "one dispatch in flight, the pair before's, and never two: the copy stands behind exactly the dispatch the home waited behind", {"product_file": True, "before": "aw.overPrev = x.w.pend[0];"}),
    # (2026-10-14, THE ORDER WAIT: a fourth wait on the game's queue, on a fence of this side's own. Every row below is a
    # release of it or the order that makes one enough; a build that lost one of them would read as the base to Proof B)
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "ArmedOrdBeforePresent();",
     "a fire's present never goes out as the second present behind a standing order wait: the worker releases first, ahead of the gate", {"product_file": True, "before": "if (FG_PRESENT_GATE_ON(isI ? 2 : 3))"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedPresent", "ArmedOrdBeforePresent();",
     "an inline present of the merged path never goes out as the second present behind a standing order wait", {"product_file": True, "before": "const HRESULT hr = dxvk(sc, src, dst, wnd, dirty, flags);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "WorkerPresent", "if constexpr (B::kWire) { if (OnWorker()) ArmedOrdBeforePresent(); }",
     "a present the worker passes through never goes out as the second present behind a standing order wait", {"product_file": True, "before": "if (FG_PRESENT_GATE_ON(0)) B::PresentGate();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedOrdWait", "FGC_IF(top && ArmedOrdRelease(top), aw.ordRelHand++);",
     "two order waits never stand: the one before is released before the next is queued, and the top is stored before the submit", {"product_file": True, "before": "if (!B::OrdWait(n))"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedOrdWait", "WriteRelease(&aw.ordTop, n);",
     "the top of the order waits is stored BEFORE the submit, so a release that runs between the two signals past it all the same", {"product_file": True, "before": "if (!B::OrdWait(n))"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedPlacerStep", "const LONGLONG ordNext = ArmedOrdWatch(now);",
     "the placer releases a standing order wait at its answer or its deadline, every step, before any pair's verdict", {"product_file": True, "before": "want(ordNext);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "BeforeReset", "if constexpr (B::kWire) ArmedStopped();",
     "no order wait stands when a Reset's barrier waits for the binding's submits to retire", {"product_file": True, "before": "if (!B::RetireWait(2000))"}),
    (os.path.join("source", "fg", "fg_machine.h"), "Stop", "if constexpr (B::kWire) ArmedStopped();",
     "a stopped machine leaves no order wait standing and gives the helper's place on the card back", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "Kill", "if constexpr (B::kWire) ArmedStopped();",
     "a dead machine leaves no order wait standing and gives the helper's place on the card back", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "OnDisarm", "if constexpr (B::kWire) ArmedStopped();",
     "a disarm leaves no order wait standing, before it waits for a fire in flight", {"product_file": True, "before": "while (InterlockedCompareExchange(&s.sh.fireBusy, 0, 0)"}),
    # (2026-10-19, round 3's merge of fg6-g3: the grid fill's second generated frame. The rule gained "|| hasI2": the first generated frame
    # is also too late where the pair's second is already due. It only withholds more, never shows more: the old guarantee stands.)
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFire", "const bool tooLate = hasN || (hasI && a.k.minLeadUs > 0 && di.tDue + UsToTicks(di.eUs) - reached < UsToTicks(a.k.minLeadUs));",
     "a generated frame is not shown at a fire point that comes too close to the real frame's raise: decided BEFORE its fire", {"product_file": True, "before": "ArmedFireI(point, own, di, reached);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedRoomPer", "return per < room ? room : per;",
     "with the room asked for the pacer's period is never under the room floor", {"product_file": True}),
    # STAGE 15 (2026-10-10), THE HUD-LESS PICTURE IN THE BACK BUFFER: each of these is a line whose absence puts the frame with
    # the interface gone on the screen, or hands the helper a pair before its HUD-less picture is in
    (os.path.join("source", "fg", "fg_machine.h"), "SwapPresent", "if (hand && B::A().r.hud == FG_HUD_PASS) { B::HudDraw(); B::A().r.hudDrawn = 1; }",
     "the HUD-less picture is drawn only behind a hand that was RECORDED, and before the note that takes it back", {"product_file": True, "before": "if (!DtNote(DT_NOTE_FG_HUDLESS, &pp, sizeof(pp)))"}),
    (os.path.join("source", "fg", "fg_machine.h"), "SwapPresent", "if (B::A().r.hudDrawn) { B::A().r.hudDrawn = 0; B::HudUndo(); }",
     "a HUD-less note that was refused with the picture drawn: the render thread puts the real picture back itself", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowHudless", "ArmedHudBack(pp, own);",
     "the picture is taken back FIRST OF ALL at the HUD-less note, before the note refuses a pair that was closed, never opened or not sent",
     {"product_file": True, "before": "if (w.openPhase != FG_PAIR_OPEN || pp.p != w.openP || s.sh.state != FG_LIVE || s.sh.armQuiet) { DEVSTAT(s.c.noteRefused++;) return; }"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ShadowHand", "ArmedHudStore(h, own);",
     "a hand that opens no pair (closed, not sent, a set that ended) still stores its frame where a HUD-less picture follows it", {"product_file": True, "count": 3}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedSpan", "else if (!pass && B::HasHudless()) B::RecCopyToImg(bb, inH);",
     "a hand with a HUD-less picture to follow does not hand the helper the presented picture as the HUD-less one", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedSpan", "if (!pass) B::RecSignal(FG_TL_IN, x.w.openN);",
     "a hand with a HUD-less picture to follow does not signal fgIn: the pair is handed over when its picture is in", {"product_file": True, "before": "took = B::RecSubmit();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedHudSpan", "B::RecCopyToImg(bb, inH);",   # (round 5, H: the HUD-less image of the pair's own input set)
    
     "the HUD-less picture is copied out BEFORE the real frame is put back over it", {"product_file": True, "before": "B::RecCopyFromImg(FG_IMG_HOLD, bb);"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedHudSpan", "B::RecCopyFromImg(FG_IMG_HOLD, bb);",
     "the restore reads HOLD, the mod's own store of the real frame, and stands in the submit that answers for it", {"product_file": True, "before": "if (B::RecSubmit()) r |= 2;"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedInlineK", "pl.kHist[bin]++;",
     "the inline pairs' K samples: pl here is B::X().pl, and FgpPlanStep reads the same array as fgp.x->pl.kHist for the plan", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedHudBack", "else { aw.wrongK = pp.k; DEVSTAT(aw.hudBackLost++;) }",
     "a picture that could not be taken back marks its ordinal: that present is not made", {"product_file": True}),
    (os.path.join("source", "fg", "fg_machine.h"), "WorkerPresent", "if (FG_ARMED_GUARD_ON(3) && OnWorker() && ArmedHudDropped(k)) return S_OK;",
     "the present of an ordinal whose picture could not be taken back is not made, asked FIRST OF ALL: before a stray caller passes through", {"product_file": True, "before": "s.c.presentsPass++;"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedG", "(w.openFlags & FG_HF_NOHUD) ? FG_AW_NOHUD",
     "a pair whose frame had an interface and no HUD-less picture is fed and never shown", {"product_file": True}),
    # THE FIRES' SAVES (2026-10-12; source/devthread/dt_bbs.h): since that day a fire leaves its save out where the device thread's
    # tracker says nothing has written the back buffer since the pair was armed. Each of these is a line whose absence
    # lets a fire overwrite the game's half-drawn frame, and no compare of binaries can see a line that is missing. (The
    # tracker's own sites in the device thread's wrappers are held by the harness, which compiles them: dt_fg_saves.h.)
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedPresent", "B::BackMark();",
     "the mark is taken where the pair is armed, BEFORE its fire points go in: a fire that came first would ask against the mark of the pair before",
     {"product_file": True, "before": "CheckpointsInstall();"}),
    (os.path.join("source", "fg", "fg_machine.h"), "ArmedFirePresent", "const bool save = B::BackWritten();",
     "every fire asks, and asks BEFORE it takes its buffers: the answer decides whether the restore's buffer is taken",
     {"product_file": True, "before": "if (!B::CbTake(save ? 2 : 1)) { DEVSTAT(w.slotWaits++;) return FG_AF_NOBUFFER; }"}),
    (os.path.join("source", "fg", "fg_arm.h"), "BackMark", "DtBbsMark();",
     "the product's mark is the device thread's: without it every question is asked against no mark (which answers written, and saves nothing)", {"product_file": True}),
    (os.path.join("source", "fg", "fg_arm.h"), "BackWritten", "DtBbsWritten(backbufferSurf, w.fireKind, w.hasSrc || w.hasDst)",
     "the product's answer is the device thread's, asked with the surface the fire will name and with the pair's rectangle: a Present with one may go through GDI, which nothing sees",
     {"product_file": True}),
    (os.path.join("source", "transport", "DlssIvTransport.h"), "IvtHome", "DtBbsRawWrite();",
     "the transport's one raw write into the back buffer tells the tracker, AHEAD of the copy: no D3D9 call shows it", {"before": "IvtCopyHome(cb, bbImg, dstL, ivtBbLiveFmt);"}),
    # (2026-10-13) THE GAME'S WEB PAGE (source/fg/fg_web.h): the harness runs the search and the rule (WP1, tools/test-fg-web); these
    # two rows hold the product's own lines that use them: the stand-by where the menu's is, and the search at the install
    (os.path.join("source", "fg", "fg_arm.h"), "MayEngage", "if (FgWebUp()) { if (!m.menuSeen) m.menuSeen = 1; return false; }",
     "a present with a web page of the game's up is not engaged: the stand-by of a menu, in the build that could ship", {"product_file": True}),
    (os.path.join("source", "fg", "fg_arm_life.h"), "FgpArmInstall", "FgWebFind();",
     "the web page's state is looked for at the install: without the call the pointer stays null and no present ever stands by for a page", {"product_file": True}),
]
MIN_SITES = 98   # raised in the commit that adds rows; never lowered without a DECISIONS entry saying why (98 since 2026-10-28: hkReset's barrier ahead of HdrEncodeRelease, the core audit's finding 1) (97 since round 9's S8, 2026-10-26: ArmedPresent's SHADOW pair row went with the want, which no build of the product asks; docs\INSTRUMENTS-FG.md, wave 8) (98 since round 9's strip, 2026-10-26: the shadow shape's three rows, ShadowSpan's signal, InjectObs' direct guard call and NoteCloseOpen, went with that shape; docs\INSTRUMENTS-FG.md, wave 5)
PRODUCT_INCLUDES = [(os.path.join("source", "DLSS-IV.cpp"), '#include "fg/fg_client.h"'), (os.path.join("source", "DLSS-IV.cpp"), '#include "fg/fg_xport.h"')]
NEVER = ["fgPresGuard", "fgBbWriters", "FgBbWriteBegin", "FgBbWriteEnd", "FgWaitForPresenter", "fgBbDraws", "fgBbClears",
         "fgBbAtHandoff", "fg_flush", "fg_guides_prev", "fg_mvtest", "ivtFgFrameId", "FEED_ACK_FG_ACTIVE"]
SCAN_DIRS = [("source", False), (os.path.join("source", "transport"), False), (os.path.join("source", "fg"), False),
             (os.path.join("source", "devthread"), False), ("host", False)]


def no_comments(text):
    """The text with every comment blanked (newlines kept, so line numbers hold). String and character literals stay."""
    out = []; i = 0; n = len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i); j = n if j < 0 else j
            out.append(' ' * (j - i)); i = j
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2); j = n if j < 0 else j + 2
            out.append(''.join(ch if ch == '\n' else ' ' for ch in text[i:j])); i = j
        elif c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == '\\' else 1
            out.append(text[i:j + 1]); i = j + 1
        else:
            out.append(c); i += 1
    return ''.join(out)


def arm_kind(d):
    """What an #if or #elif line's arm is: 'fg' (the product's switch), 'dev' (a development switch: the code is not in a
    build without it), 'off' (#if 0), or 'other'."""
    if re.match(r'(el)?if\s+DLSSIV_FG\b', d): return 'fg'
    if re.match(r'(el)?if\s+(DLSSIV_DEV|DLSSIV_DEVLOG)\b', d) or re.match(r'ifdef\s+(DLSSIV_DEV|DLSSIV_DEVLOG)\b', d): return 'dev'
    if re.match(r'(el)?if\s+0\b', d): return 'off'
    return 'other'


def arm_lines(text):
    """For each line of the comment-free text, (in the product's switch, in an arm a build without the development switch
    does not compile). EVERY enclosing #if is tracked (the review of phase 2: one boolean a level read '#if DLSSIV_FG' with
    '#if DLSSIV_DEVLOG' or '#if 0' nested inside as product code). The #else of the product's switch is not product code;
    the #else of a development switch is ordinary code."""
    stack = []; out = []
    for line in text.split('\n'):
        s = line.strip()
        if s.startswith('#'):
            d = s[1:].strip()
            if re.match(r'if(def|ndef)?\b', d): stack.append(arm_kind(d))
            elif re.match(r'elif\b', d) and stack: stack[-1] = arm_kind(d)
            elif re.match(r'else\b', d) and stack: stack[-1] = 'off' if stack[-1] == 'fg' else 'other'
            elif re.match(r'endif\b', d) and stack: stack.pop()
            out.append((False, True))
        else:
            out.append(('fg' in stack, 'dev' in stack or 'off' in stack))
    return out


def fg_lines(text):
    """For each line of the comment-free text: is it in the product's switch and in no development or dead arm?"""
    return [fg and not dev for fg, dev in arm_lines(text)]


def body_of(text, fn):
    """(first line index, last line index) of the body of the function named fn in the comment-free text, or None."""
    for m in re.finditer(r'\b%s\s*\(' % re.escape(fn), text):
        i = m.end(); depth = 1
        while i < len(text) and depth:
            depth += {'(': 1, ')': -1}.get(text[i], 0); i += 1
        j = i
        while j < len(text) and text[j] in ' \t\r\n': j += 1
        if text.startswith('const', j): j += 5
        while j < len(text) and text[j] in ' \t\r\n': j += 1
        if j >= len(text) or text[j] != '{': continue          # a call or a declaration, not the definition
        k = j + 1; depth = 1
        while k < len(text) and depth:
            c = text[k]
            if c == '"' or c == "'":
                k += 1
                while k < len(text) and text[k] != c: k += 2 if text[k] == '\\' else 1
            elif c == '{': depth += 1
            elif c == '}': depth -= 1
            k += 1
        return text.count('\n', 0, j), text.count('\n', 0, k)
    return None


def site_problem(text, fn, needle, count=1, before=None, product_file=False):
    """None when the needle is in fn's body in product code, as many times as wanted and before the text it must precede;
    else why not."""
    t = no_comments(text)
    b = body_of(t, fn)
    if b is None: return "the function %s was not found" % fn
    lines = t.split('\n'); arms = arm_lines(t); flags = [fg and not dev for fg, dev in arms]
    there = [i for i in range(b[0], b[1] + 1) if needle in lines[i]]
    if not there: return "\"%s\" is not in %s" % (needle, fn)
    # NOT IN A DEVELOPMENT ARM, in a product file too, and NOT INSIDE DEVSTAT(...): such a guard is in the development build,
    # the harness and the fake client, and in no build a player would have
    live = [i for i in there if not arms[i][1] and not ('DEVSTAT(' in lines[i] and lines[i].index('DEVSTAT(') < lines[i].index(needle))]
    if not live:
        return "\"%s\" is in %s, but only in development code (a development switch, #if 0, or DEVSTAT(...)): no build without the development switch has it" % (needle, fn)
    # (FGA(...), since 2026-10-12: a call site of the ARMED path, which is product code of a build whose product can arm:
    # DLSS-IV.cpp defines it beside FGX, behind DLSSIV_FG, which arms in every build since round 9's strip)
    inside = [i for i in live if product_file or flags[i] or any(w in lines[i] and lines[i].index(w) < lines[i].index(needle) for w in ('FGX(', 'FGA('))]
    if not inside:
        return "\"%s\" is in %s, but outside the switch: not in an #if DLSSIV_FG block and not inside FGX(...) or FGA(...)" % (needle, fn)
    if len(inside) < count:
        return "\"%s\" is in %s %d time(s) in product code, and %d are wanted" % (needle, fn, len(inside), count)
    if before is not None:
        # the site's own line may hold the anchor's text too (a failure arm that releases): the anchor is looked for on
        # the OTHER lines of the body
        anchors = [i for i in range(b[0], b[1] + 1) if before in lines[i] and i not in inside]
        if not anchors: return "\"%s\", which \"%s\" must come before, is not in %s" % (before, needle, fn)
        if inside[0] > anchors[0]: return "\"%s\" comes AFTER \"%s\" in %s (line %d against %d): a guard below what it guards" % (needle, before, fn, inside[0] + 1, anchors[0] + 1)
    return None


def include_problem(text, inc):
    """None when the #include line is inside an #if that names DLSSIV_FG; else why not."""
    t = no_comments(text); stack = []; seen = 0
    for line in t.split('\n'):
        s = line.strip()
        if s.startswith('#'):
            d = s[1:].strip()
            if s.replace(' ', '') == inc.replace(' ', ''):
                seen += 1
                if not any(stack): return "%s is not behind #if DLSSIV_FG" % inc
            elif re.match(r'if(def|ndef)?\b', d): stack.append(bool(re.match(r'if\s+DLSSIV_FG\b', d)))
            elif re.match(r'elif\b', d) and stack: stack[-1] = bool(re.match(r'elif\s+DLSSIV_FG\b', d))
            elif re.match(r'else\b', d) and stack: stack[-1] = False
            elif re.match(r'endif\b', d) and stack: stack.pop()
    return None if seen == 1 else "%s was found %d time(s), not once" % (inc, seen)


def never_hits(text):
    t = no_comments(text)
    return [(nm, t.count('\n', 0, m.start()) + 1) for nm in NEVER for m in re.finditer(r'\b%s\b' % re.escape(nm), t)]


def main():
    bad = 0
    # THE CONTROLS, on text in memory
    good = ("static void Guarded()\n{\n    int a = 1;\n#if DLSSIV_FG\n    FgReleaseQueuedWait();   // the release\n#endif\n}\n"
            "static void Other()\n{\n    FGX(FgStandDown();)\n}\n")
    c1 = site_problem(good, "Guarded", "FgReleaseQueuedWait(") is None and site_problem(good, "Other", "FgStandDown(") is None
    c2 = site_problem(good.replace("    FgReleaseQueuedWait();   // the release\n", ""), "Guarded", "FgReleaseQueuedWait(") is not None
    c3 = site_problem(good.replace("#if DLSSIV_FG\n", "#if DLSSIV_DEV\n"), "Guarded", "FgReleaseQueuedWait(") is not None
    c4 = site_problem(good.replace("FgReleaseQueuedWait();   // the release", "// FgReleaseQueuedWait();"), "Guarded", "FgReleaseQueuedWait(") is not None
    n5 = never_hits("static int fgBbWriters = 0;\n"); n6 = never_hits("// fgBbWriters went with the borrow\nstatic int x;\n")
    print("  control 1: a site that is there, in an #if block and in FGX(...): %s" % ("found" if c1 else "NOT PASSED"))
    print("  control 2: the same site with its line deleted: %s" % ("FAILS as it must" if c2 else "NOT SEEN"))
    print("  control 3: the same line behind another switch: %s" % ("FAILS as it must" if c3 else "NOT SEEN"))
    print("  control 4: the same line turned into a comment: %s" % ("FAILS as it must" if c4 else "NOT SEEN"))
    print("  control 5: a never-returning name in code %s, and in a comment %s" % (
        "FAILS as it must" if len(n5) == 1 else "NOT SEEN", "does not" if not n6 else "NOT PASSED"))
    # THE COUNT AND THE ORDER (the review of stages 5 to 7): one of two wanted sites deleted; a guard moved below what it guards
    two = "static void Twice()\n{\n    FGX(FgStandDown();)\n    int a = 1;\n    FGX(FgStandDown();)\n}\n"
    c6 = site_problem(two, "Twice", "FgStandDown();", count=2) is None and site_problem(two.replace("    int a = 1;\n    FGX(FgStandDown();)\n", "    int a = 1;\n"), "Twice", "FgStandDown();", count=2) is not None
    order = "static void Ordered()\n{\n    FGX(FgBeforeRelease();)\n    ReleaseImages();\n}\n"
    late = "static void Ordered()\n{\n    ReleaseImages();\n    FGX(FgBeforeRelease();)\n}\n"
    c7 = site_problem(order, "Ordered", "FgBeforeRelease();", before="ReleaseImages();") is None and site_problem(late, "Ordered", "FgBeforeRelease();", before="ReleaseImages();") is not None
    inc = "#if DLSSIV_FG\n#include \"fg/fg_client.h\"\n#endif\n"
    c8 = include_problem(inc, '#include "fg/fg_client.h"') is None and include_problem(inc.replace("#if DLSSIV_FG", "#if DLSSIV_DEV"), '#include "fg/fg_client.h"') is not None
    print("  control 6: two sites wanted and one deleted: %s" % ("FAILS as it must" if c6 else "NOT SEEN"))
    print("  control 7: a guard moved below what it guards: %s" % ("FAILS as it must" if c7 else "NOT SEEN"))
    print("  control 8: the product header's include behind another switch: %s" % ("FAILS as it must" if c8 else "NOT SEEN"))
    # A GUARD THAT ONLY A DEVELOPMENT BUILD HAS (the review of phase 2): nested in a development switch inside the product's,
    # and inside DEVSTAT(FGX(...)); in a product file too
    nested = good.replace("    FgReleaseQueuedWait();   // the release\n", "#if DLSSIV_DEVLOG\n    FgReleaseQueuedWait();   // the release\n#endif\n")
    c9 = site_problem(nested, "Guarded", "FgReleaseQueuedWait(") is not None and site_problem(nested, "Guarded", "FgReleaseQueuedWait(", product_file=True) is not None
    devstat = good.replace("    FGX(FgStandDown();)\n", "    DEVSTAT(FGX(FgStandDown();))\n")
    c10 = site_problem(devstat, "Other", "FgStandDown(") is not None
    print("  control 9: the same site nested in a development switch, in a product file too: %s" % ("FAILS as it must" if c9 else "NOT SEEN"))
    print("  control 10: a site inside DEVSTAT(FGX(...)): %s" % ("FAILS as it must" if c10 else "NOT SEEN"))
    if not (c1 and c2 and c3 and c4 and len(n5) == 1 and not n6 and c6 and c7 and c8 and c9 and c10): bad += 1
    # THE SITES
    if len(SITES) < MIN_SITES:
        print("  FAIL: %d site(s) in the table where the floor is %d" % (len(SITES), MIN_SITES)); bad += 1
    for rel, fn, needle, why, more in SITES:
        p = site_problem(fg_machine_text.read(os.path.join(ROOT, rel), encoding="utf-8", errors="replace"), fn, needle,
                         count=more.get("count", 1), before=more.get("before"), product_file=more.get("product_file", False))
        if p: print("  FAIL: %s: %s (%s)" % (rel, p, why)); bad += 1
    for rel, inc in PRODUCT_INCLUDES:
        p = include_problem(open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace").read(), inc)
        if p: print("  FAIL: %s: %s (a product header's lines carry no #if of their own: its include is what holds them to the switch)" % (rel, p)); bad += 1
    # NEVER RETURNS
    files = 0
    for d, _ in SCAN_DIRS:
        full = os.path.join(ROOT, d)
        for f in sorted(os.listdir(full)):
            if not f.endswith((".h", ".cpp")): continue
            files += 1
            for nm, ln in never_hits(open(os.path.join(full, f), encoding="utf-8", errors="replace").read()):
                print("  FAIL: %s:%d: %s is back, and the plan lists it as never returning" % (os.path.join(d, f), ln, nm)); bad += 1
    print("%d site(s) of the must-find table hold (the floor is %d); %d never-returning names absent from %d files" % (
        len(SITES), MIN_SITES, len(NEVER), files))
    print("FAIL" if bad else "PASS")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
