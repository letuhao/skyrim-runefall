# S1 co-save implementation audit 16 — code 947bf67 / state a5f1b92

Auditor: teammate `environment-analyst` (independent; did not implement S1; no S1 implementation
history). Date: 2026-10-29, Asia/Bangkok. Mode: read-only; only this file written.
Subject: candidate code commit **947bf67** ("test(s1): cover remaining A6 classes and A1 reset seam"),
state child **a5f1b92**. Contract: rev8 `build/slices/S1/spec.md` SHA-256
FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1 (state contract matches) +
approved amendment 2 `spec-amendment-2.md` SHA E54563976FF17974E22CED1C71483086BCA1276EF6833C35177A2273009F2F95
(approved by `S1-spec-amendment-2-audit-rev2.md`, hash F6519FBE… matches state contract.amendments).

Read: spec rev8, amendment 2, audit 10 precedent, CoSaveCodec.h/.cpp, CoSave.h/.cpp,
CoSaveCodecTests.cpp, CoSaveCodecExtraTests.cpp, CMakeLists (preset wiring), evidence
batch-20261029-154959.md + raw + build-20261029-1549.txt + host-tests-20261029-1549.txt,
C1-allowlist.txt, EXECUTION-STATE.json, git show a5f1b92/947bf67.

## Read-only checks actually run
1. Host tests re-run: `src/plugin/build/test-windows-vendored/RunefallTests.exe` -> exit 0,
   **All tests passed (132 assertions in 14 test cases)**, matching evidence host-tests-1549
   (cwd/command/exit/132/14 identical).
2. Build/rig provenance: build-1549 BUILD_EXIT=0, DLL_SHA256 386A7E56…; re-hashed rig DLL
   J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\Runefall.dll = **386A7E56…** (match), rig mtime
   15:49:31 < batch start 15:49:59; DLL newer than CoSaveCodec.cpp (15:47:10); earlier candidates
   1432/1513/1529 have distinct DLL hashes (5396/891E/CF90), so the rig DLL is candidate-specific.
3. Batch: raw-report SHA-256 = 9BA2EAA72B82A43E7639D93D7A31481E5E3F7DFB6E3C9139429D93C6BB55C37C
   = recorded body_sha256; batch has 16-byte header hex, save/load round-trip of all pre/post fields,
   no-new-crash row (pre-batch 04:26 crash covered by F7 waiver).
4. State: remediation.S1 state=AUDIT_PENDING, status=PENDING, commit=947bf67, implAudit=null (this
   audit is the pending record); all 17 evidence entries exist and hash-match; spec + amendment
   hashes current (item 1 above).
5. NOT run: game/batch (already evidenced), production build compile (build log inspected), gate.

## Criterion verdicts (exact code commit 947bf67)

| ID | Verdict | Basis / exact gap |
|---|---|---|
| A1 | **PARTIAL** | Pure-layer closed: `CoSaveCodec::LoadOrReset` exists (CoSaveCodec.cpp:678) and host test "A1 LoadOrReset defaults" covers empty list -> false + default, malformed record -> false + default, valid -> true (132-assertion suite). Adapter seam `CoSave::LoadRecords` delegates to the codec (CoSave.cpp:85-88); OnLoad failure -> `ClearLoadedState()` (CoSave.cpp:226). **Gap:** spec A1 requires in-game F11 assertion of registry defaults "against a freshly constructed default instance (CodecState{}, ItemRegistry empty, Progression defaults, MaterialRegistry default map), not hardcoded data values"; the adapter SelfTest still asserts only `playerRank == 1 && ItemRegistry empty` (CoSave.cpp:555) — hardcoded, and Progression/MaterialRegistry default post-state is loaded but never asserted. |
| A2 | **PASS** | Header table/bytes unchanged; batch-154959 records full header hex 524E465303000000000003000A000000; codec header paths unchanged except refactor; rev8 states rev6 evidence retained. |
| A3 | **PASS** | Migration fixtures/expected fields untouched by this candidate; rev8 retains rev6 assertions; no code change to migration paths in 947bf67 (diff touches tokens/dup tests only). |
| A4 | **PASS** | record-version gate token test (CoSaveCodecTests.cpp:82); schema 0/4 tests (ExtraTests.cpp:88-97); both reject before decode; newer-save in-game sim BLOCKED per spec. |
| A5 | **PASS (residual)** | Crafted decode-side duplicate registryId payload + exact token "reject item.registryId" (CoSaveCodecTests.cpp:226-247); duplicate section (ExtraTests:194 in token matrix + :414 MarkReject); duplicate-key token implemented for materials (:450), hunt (:508), pack (:524), overrides (:584), skills (:606) with host tests for materials (:196) and override (:216-224). Residual: **hunt/pack/skill duplicate-key paths have no host test**, and the A5 duplicate-registryId test does not assert destination unchanged (sentinel purity is covered only by the generic failure-purity test). |
| A6 | **PARTIAL** | 947bf67 fills most A6.1 classes: item fields 8b-8j + 9 zero (:69-86), schema 1a (:88-97), truncated 3a, dedupe 12, skill-count 5h + string.skill 7b (:107-117), record-version/flags/mirror/section-count/empty/missing/duplicate-section/count-remaining/record-size/budget (:175-199), caps 5a-5g + strings 7a/7c/7d + range 8a (:249-279). **Remaining A6 gaps:** (i) A6.1 **7e affix/effect >64** has no crafted payload/token test; (ii) A6.2 accepted rows **13a collection exactly at cap**, **13b record at exact max size**, **13c missing optional section** have no host test — spec pass condition "every rejection row has a host-crafted payload and exact token; every accepted row has a host test" is therefore not yet met. |
| A7 | **PASS** | Constants/1000-item/budget refusal unchanged; `EncodeWithBudget` budget token exercised at A6 (CoSaveCodecTests.cpp:199); docs state 1 MiB. |
| A8 | **PASS (evidence note)** | Batch evidence/raw hash reproduced; build exit 0 and DLL hash equals rig DLL; host metadata (cwd/command/exit) present; state evidence hashes current; pre/post field list complete including header hex and crash check. Note: build-1549 log contains "ninja: no work to do" (a second no-op invocation); the compiling invocation is proven only indirectly by mtime (DLL 15:49:31 > codec source 15:47:10) and the distinct per-candidate DLL hashes — acceptable, but the log itself is not a compile transcript. |

## Governance / amendments
- Amendment 2 is approved and its hash is recorded; its scope (test harness/preset/allowlist) is
  consistent with rev8 and adds no acceptance weakening.
- **Allowlist gap (blocking for C1 acceptance):** candidate 947bf67 adds
  `src/plugin/test/CoSaveCodecExtraTests.cpp` (133 lines, new file), but neither rev8 §5,
  amendment-2 §3, nor `build/slices/S1/C1-allowlist.txt` lists it (manifest lists only
  `src/plugin/test/CoSaveCodecTests.cpp`). The enforcement command filtered against the manifest
  would flag this file, so the C1-is-within-allowlist condition is not met.
- F7-CONSOLE-THREAD follow-up remains open and is out of S1 codec scope (waiver unchanged).

## Overall verdict
**PARTIAL — not PASS.** Host tests (132/14) are genuine and reproduce; the pure codec, A5 crafted
duplicate-registryId, A6 rejection-class coverage, build/rig/DLL and batch evidence are solid.
Remaining gaps: A1 in-game registry-defaults assertion still hardcoded/incomplete (Progression/
MaterialRegistry unasserted); A6 rows 7e + accepted 13a/13b/13c missing; CoSaveCodecExtraTests.cpp
outside the approved C1 allowlist. Minimum amendments: (1) extend the adapter SelfTest to compare
CodecState/ItemRegistry/Progression/MaterialRegistry against fresh default instances; (2) add the
four A6 tests; (3) amend the spec/allowlist (amendment 3) to include CoSaveCodecExtraTests.cpp or
fold it into CoSaveCodecTests.cpp. No source, data, state, fixture, gate, build or game action was
performed; only this file was written.
