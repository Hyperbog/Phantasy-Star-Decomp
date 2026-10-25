## Mandatory delivery-first rule — applies to the entire Git portfolio

**Scope:** RLOFT, CVIII, Ys, Phantasy Star, and every other repository or workstream. This is a hard instruction for AI coding/research agents, not an optional preference. Existing source-authentication, copyright, safety, branch-ownership, and required acceptance gates remain binding.

1. **Deliver functioning code, not a procession of tests.** Begin with the requested user-visible end state. Identify the causal defect, make the smallest principled production-level correction, and integrate it. Test infrastructure, static inventories, synthetic fixtures, reports, and experimental variants are supporting tools, **not completed features or progress milestones** by themselves.
2. **Hard cap on exploratory loops.** Reuse the code, logs, recordings, and existing tests first. Allow **at most one baseline reproduction and two focused diagnostic experiments per unresolved defect**. Each experiment must decide a specific implementation choice. After that, **stop experimenting**: implement the best source-supported correction, switch to a fundamentally different implementation strategy, or report the precise blocker. Do not create new named stages/checkpoints to evade this limit.
3. **No speculative piles of variants.** Do not keep adjusting constants, offsets, timing, flags, emulator settings, or synthetic tests until one happens to match. Do not build adjacent diagnostic frameworks where direct product implementation is possible. A narrow diagnostic is justified only when it directly changes the next code decision.
4. **One integrated fix, then essential proof.** Validate the changed functionality with a focused regression and the existing relevant end-to-end/CI gates. Preserve independent original-vs-native evidence where required. **After two non-improving implementation/validation attempts, stop that approach**; isolate the blocker and change strategy rather than loop.
5. **Progress means real capability.** Never call a milestone successful merely because tools compile, CI smoke passes, an experimental replay looks better, or a report is committed. Claim “fixed,” “working,” “playable,” or “certified” **only when the required real acceptance behavior passes**. Otherwise say “partial” or “blocked” with exact evidence. Do not inflate source-match or gameplay metrics.
6. **Prioritize the highest-value next action.** When blocked, make one concrete, useful code deliverable if possible, state the exact impediment and necessary next decision, and stop. No busywork, no routine micro-commits, no repetitive status theater, and no requests for new user playthroughs when existing inputs suffice.
7. **Brief completion reporting.** Report what actually changed, the end-to-end result, what remains broken, and the next causal fix. Separate user-performed tests from internal/CI tests. Do not promise work that has not run.

**Non-negotiable principle:** **Fix the real problem. Prove the fix. Move the project forward. Stop the test rabbit hole.** The user must explicitly authorize an extended exploratory investigation beyond the limits above; such authorization never permits weakening mandatory safety, authenticity, or regression checks.

---

# Phantasy Star — automated native recompilation contract

- This repository tracks the **game-specific, distributable source and tests only**. Never commit copyrighted ROMs, ROM-equivalent generated C, extracted commercial assets, executable binaries, memory dumps, screen captures, or audio.
- Primary target is the supplied 512 KiB English Rev. 3 SMS ROM, verified by SHA-256 before every generation or run. Japanese ROM work must use a separate game config and independent verification.
- Recreate the exact Stage 11 patched engine using `python tools/bootstrap_engine.py --engine-zip ... --core-zip ...`. It compares upstream hashes before applying all patches and refuses unexpected dependency revisions.
- Changes belong in the recompiler, runtime, tests or `patches/`, **not** in `generated/`. Regenerate generated C from the original ROM after generator changes.
- Every new gameplay claim requires a native versus interpreter replay, machine-state comparison, and a failing-then-passing regression. The bundled interpreter is not an independent emulator; confirm later against Mesen or equivalent.
- Keep unmatched state differences and sound sample differences visible. Do not conflate screenshots, register write sequences, or frame hashes with exact CPU, cycle, or PCM parity.
- Prefer integrated whole-game progress (menu, map, battle, save) over new bank-wide `.DB` inventories.
- No agent may disable or weaken the verification gates to pass a check; use explicit quarantined failures and restore exact evidence.
- The current tested checkpoint is Stage 11; inspect `docs/STAGE11_VALIDATION.md` before claiming broader coverage.

- For cartridge save tests, assert actual file-byte equality in addition to frame parity. Never report a working SAVE/CONTINUE flow based only on backup-RAM initialization.

- Stage 11 uses actual in-game SAVE, fresh-process CONTINUE, and an erased-slot negative control for slots 1/2. Preserve these replay gates when changing mapper, fallback or code generation.

## Stage 12: game-progress verification gate

When a native/interpreter comparison passes but both outputs are stationary,
that result establishes parity only, **not gameplay progress**. For dungeon
routes, require real SAVE/CONTINUE prerequisites, chronological mode transitions,
per-checkpoint `$C30A` facing and `$C30C` position assertions, actual coordinate
changes, byte-exact final video memory, persistent SRAM integrity, and negative
controls. Use `tools/run_dungeon_route.py` and `tests/test_stage12_dungeon.py`.
Do not call these routes a verified battle; no encounter was observed. Never
edit ROM-derived `generated/` files; change the generator and retest instead.
