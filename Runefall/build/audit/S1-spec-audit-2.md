# Second independent spec audit — S1 co-save correctness (revision 2)

Auditor: teammate `environment-analyst` (fresh; no S1 implementation history; did not author the S1 spec,
`PROGRAM.md`, or the first audit). Date: 2026-10-28, Asia/Bangkok. Mode: read-only.
Subject: `build/slices/S1/spec.md` revision 2, 9,354 B, SHA-256
`19396E9DB9770216897EA86F3C1E76F5395BB00604421727E97353B83DECEBB0`.
Read: spec; `build/audit/S1-spec-audit.md` (F1-F9/M1-M14); `PROGRAM.md`; `src/plugin/src/CoSave.{h,cpp}`;
`Core.h` (kSaveVersion=3); `tools/run-batch-test.ps1` co-save block; `SPECS/POWER-LADDER.md` section 9;
`SPECS/L4-RUNTIME.md` section 5; git state.

Note: an untracked draft (73 lines, 64 KiB-budget era) was present at this path when I started; it does not
match the current rev2 text and was not authored by me. This audit supersedes it; the stale draft is
flagged to the Lead because `build/` is gitignored and its content is not preserved elsewhere.

## 1. Verdict

**REJECTED** — five blocking gaps. Revision 2 closes the header truth (F2/M3/M4), fixture provenance and
migration mapping (F3/M5/M6), the two downgrade gates (F4/M7), and most governance items (F9/M12-M14).
Four criteria (A1 record-loop, A5 order semantics, A6 table/tokens, A7 arithmetic) and one governance
contradiction (A8 gating) remain unfixed; A1-A7 also inherit the runtime dependency the spec assigns
only to A8.

## 2. Independently re-verified facts

- Canonical header in spec 1.20 is byte-exact vs current writer: `CoSave.cpp:224-227` emits magic[0-3],
  u16 schema[4-5], u32 flags[6-9]=0, u16 saveVersion[10-11]; sectionCount[12-15] at `:332`; v1/v2
  12-byte branch at `:346-360`. Correct side: code. POWER-LADDER.md:178 and L4-RUNTIME.md:117/134 still
  state the wrong order and must be edited by C1 (spec 1.21 requires it).
- Caps in code: kMaxPersistedRefs 4096, kMaxPersistedQuests 1024, kMaxLadderOverrides 256
  (`CoSave.cpp:26-28`) match spec 3.39.
- Batch co-save evidence is log-only (`run-batch-test.ps1:246-266` matches "wrote/loaded schema 3");
  A8's pre/post field comparison is genuinely new work, as the first audit said.
- Budget arithmetic: 262144 B is consistent with the cap table (rough worst case ≈125 KB) and with
  L4 §14 item 11's 1,000-item test, resolving the first audit's 64 KiB conflict. The defect is that the
  required arithmetic table is still not in the spec (B3).

## 3. Closed items (no further amendment)

M1 (failure-pure Decode + LoadRecord seam + registry reset specified: 2/A1, 3.38), M3/M4 (header table
normative vs code, docs correction mandated, standalone decoder), M5/M6 (fixtures from e0b9dfa history +
baseline-v3; exact v1/v2 migration; LUT vs legacy_rf_tier_map.csv resolved: CSV = content bands,
1/20/40/60/80/100 = save rank mapping), M7 (distinct recordVersion vs payload-schema gates, distinct
tokens, in-game downgrade BLOCKED), M12 (criterion→evidence table section 6), F2/F3/F4, and the
financial/governance frame (C1 allowlist, spec sha in remediation.S1.contract + HISTORY, state
transitions, audit-gate command, configHash opaque, 262144 constants and pre-OpenRecord refusal).

## 4. Remaining blocking findings

**B1 — Cap/range/log-token table still absent (M9/F6; A1, A4, A6, A7).**
Spec 3.39 lists encode caps and a few ranges but no per-field decode cap, no enum range for
schema/saveVersion/flags/material counts/hunt flag/endgame/enhancement category+tier/tags mask/
registryId/baseFormId/container+quest ids/override flags, and A6 demands one negative per row with
"tokens asserted", A1/A4/A7 demand "fixed log token"/"fixed log line"/"fixed error" — but no token text
is pinned anywhere, and schema 0 / absent schema is still unspecified (current decoder accepts it:
`CoSave.cpp:356` tests only the upper bound). Also the existing encoder bug (skillRanks declares
min(size,256) then writes all entries, `:324-328`) needs an explicit "declared count == entries
written" rule.
Minimum: add the full cap/range table (encode cap, decode cap, allowed range per field/collection,
including schema ∈ {1,2,3} and saveVersion mirror), one fixed token per rejection family (e.g.
`co-save: reject <code>`), and A6 wording "encoder enforces the same caps and writes exactly the
declared count".

**B2 — Persisted-order/eviction semantics unresolved (M8/F5).**
A5 canonicalises container/quest/override ordering but never defines what persisted order means:
`RememberGeneratedContainer`/`RememberRewardedQuest` evict with `erase(begin())` at cap, so
canonical re-encode changes which entries survive the next cap eviction. First audit F5 explicitly
required "declare persisted order non-semantic or define deterministic eviction policy".
Minimum: one clause + test (set semantics; deterministic eviction over canonical order, or order
declared non-semantic).

**B3 — Save-budget arithmetic table missing (M10/F7; A7).**
A7 requires "cap arithmetic table proves realistic legal worst case is under budget", but the spec has
only the caps and the two 262144 constants; the "constant/doc audit" has no table/document to audit, and
the oversize path still does not state engine-visible behaviour (save proceeds without Runefall state?
user-visible warning?) or the accepted data-loss consequence.
Minimum: embed the per-collection worst-case sum (or pin path/hash) with the resulting legal worst case;
state OnSave behaviour after refusal and the data-loss consequence.

**B4 — OnLoad record-loop semantics underspecified (F1/M2).**
A1 requires zero-record, one-record, multiple-RNF2-record, unknown-record and oversized-record behaviour
to be "specified and tested", but 3.38 only says "OnLoad resets only at record-loop start, then
LoadRecord into candidate". Multiple records (override? merge?), unknown record types (skip? reject?),
and the zero-record post-state are still undefined.
Minimum: 3-5 line record-loop definition with exact post-state per case.

**B5 — A8 command/environment not pinned; environment gating contradicts audit 1 section 5 (F8/M11).**
A8 says "exact batch command and environment documented", but section 5 gives only
`C:/Users/NeneScarlet/.cache/build-runefall.bat` and the script path: no invocation/parameters, test
root, save name, MCP dependency, selftest command, or crash-log check. Also line 6 says "S2-S15 may not
consume co-save changes until S1 PASS" while 5.56 says "if the environment is unavailable, A8 is BLOCKED
and S1 cannot PASS": the first audit's condition for not splitting was "keep one slice only if A8 is
explicitly environment-gated and does not gate S2-S15". As written an absent runtime blocks the program.
Minimum: (a) pin exact commands/environment in the spec; (b) either split into S1a (A1-A7) / S1b (A8) as
audit 1 section 5 recommended, or state that A8-BLOCKED holds only co-save consumption in S2-S15.

## 5. Nonblocking findings

- N1 Section 1.16-18 table: offset-6 width says u16 while the v3 field is u32 flags; row 8 mixes v1/v2
  and v3 offsets. 1.20's "Precise" paragraph is correct — clean the table to match.
- N2 A1-A7 as specified are executed by `CoSave.SelfTest` in DebugHarness (kDataLoaded) plus the F11
  probe, i.e. they need the game, not just a build; the spec marks only A8 environment-gated. Either
  state that all SelfTest-based criteria share the environment gate or add a host/runnable codec harness
  to the C1 allowlist.
- N3 Section 1.10 "do NOT reorder" vs A5 canonical sorting: clarify it means header/section order only.
- N4 A3 baseline-v3 fixture needs a pre-S1 build artifact or real save; pin origin + derivation command.
- N5 M13/M14 residue: single-writer audit-path rule and C1 enforcement command
  (`git diff --name-only a50b2b6..HEAD` + allowlist + clean-tree check) are not stated.
- N6 Build command is an absolute out-of-repo path; acceptable on this host, but record its hash and note
  clone-reproducibility limits.

## 6. Testability notes

- A2/A4/A5/A6/A7 are repo-auditable once build+SelfTest execution exists; A1's record-loop and
  registry-reset paths need either a host seam or in-game malformed-record probe (N2).
- A3 needs the baseline-v3 artifact (N4); A8 is inherently user-side/MCP-dependent and correctly
  BLOCKED-able, but its gate coupling is B5.
- SelfTest execution itself is in-game today (DebugHarness at kDataLoaded), which is why N2 matters.

## 7. Auditor statement

Only this file was written (overwriting the stale untracked draft described above). No source, docs,
state, fixtures, git, builds, gates or game actions. Independence: not the implementer; did not author
the spec, PROGRAM.md, or the first audit.
