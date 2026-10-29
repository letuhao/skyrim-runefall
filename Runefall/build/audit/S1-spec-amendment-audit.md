# S1 spec amendment 1 — independent specification audit

Auditor: independent spec auditor (audited S1 rev6 in earlier rounds; did not implement S1).
Date: 2026-10-29, Asia/Bangkok. Timebox: 8 minutes. Scope: amendment contract only; no implementation.

Verdict: **REJECTED — 10 blocking amendments (BA1-BA10) before implementation may continue.**

## 0. Inputs audited

- build/slices/S1/spec-amendment-1.md — sha256 C64AE00043740FBE77A4533149F5348CA6158D0D0C9E3D1F04FA653C4AED0E35, 4,721 B (committed at HEAD; still marked DRAFT, not referenced by state contract).
- build/slices/S1/spec.md revision 6 — sha256 E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427, 14,804 B (= remediation.S1.contract sha256).
- Repo HEAD 915a26f; git status: only README.md modified.
- State: remediation.S1 state=AUDIT_PENDING, status=PARTIAL, commit=a8fe10e, implementationAudit=S1-audit-11 (PARTIAL).
- Read: rev6 spec; amendment; build/audit/METHODOLOGY.md; build/decisions/ADR-001-data-driven-audit.md; _context/code-index/REFERENCE-REVIEW.md and MEO-REVIEW.md; S1-audit-9, S1-audit-10, S1-audit-11; AGENTS.md 3.2 (updated); src/plugin/CMakeLists.txt; CMakePresets.json; test/ExampleTests.cpp; tests/ladder_host.cpp; src/PCH.h; src/Core.h includes; Progression.h/MaterialRegistry.h includes; git state.

## 1. Answers to the five review questions

### Q1. Does the amendment correctly replace per-row data/token enumeration with loader/behavior/failure-class evidence?

**Intent: yes. As written: no, not yet.** The amendment correctly states the principle (METHODOLOGY, ADR-001) and names the right replacements (A1 loader seam, A3 representative migration, A5 code invariant, A6 failure classes, A8 runtime gate). But it is not a self-contained contract: it never states which rev6 clauses it replaces, so the frozen contract would contain both the old per-row requirements and the new class requirements. Specifically still binding-looking and contradictory:
- rev6 A6 (spec.md:28) "SelfTest one negative per table row"; rev6 §3.1 is a 30-row token table (spec.md:33-75); rev6 §5 C1 mapping (spec.md:118) "one negative per cap/range row + >256 skills".
- The amendment supplies no class-to-token mapping, so "fixed token" per class is not assertable; §3.1 tokens are per-row.
- The class list is incomplete (see BA6): no declared-count-vs-remaining allocation guard, no absent/zero-length section policy, no duplicate raw container/quest entries, no exact boundary (at cap / cap+1) cases.

### Q2. Are A1/A3/A5/A6/A8 concrete, testable and not weaker than the original behavior requirements?

| Criterion | Verdict | Finding |
|---|---|---|
| Amended A1 | NOT TESTABLE AS WRITTEN | Internal contradiction: "rejected input never changes destination or live state" vs "on failure/no record, clear CoSave state and ItemRegistry, Progression, MaterialRegistry". It needs two levels: codec failure-purity (destination untouched) and OnLoad fail-closed publish (defaults exactly once). Also "explicit default assertions for all three registries" cannot be host-tested: Progression/MaterialRegistry include Core.h which includes PCH.h (RE/SKSE), so a host target cannot compile them; evidence-layer assignment is missing. "OnLoad callback code inspection (or executed seam if available)" keeps an escape hatch instead of naming the required seam. |
| Amended A3 | WEAKER | It asserts only rank/stage/difficulty/curve, but rev6 A3 pinned more: bossKillTier retained, XP0, class empty, endgame false, v1 rank/stage=1, v2 tier5->100 and tier2->40, plus fixture hashes/provenance and baseline-v3 load. Unless rev6 A3 stays normative verbatim, the amendment silently weakens the migration contract. |
| Amended A5 | UNDERSPECIFIED | "Map/override duplicate policy documented and tested at class level" defers the actual policy (reject with token vs deterministic last-wins). Decoder-side duplicate-registry payload test is named but no fixed token or post-state assertion. It does not say whether duplicate raw container/quest entries are deduped or rejected, though rev6 §3.2 requires dedupe. |
| Amended A6 | UNDERSPECIFIED / CONTRADICTORY | 10 classes are a good skeleton but incomplete (BA6) and carry no fixed-token table. It says the in-game self-test "may cover a subset" while rev6 A8 requires all A1-A7 self-tests in the same game session and rev6 A5 requires golden-byte evidence. Precedence must be explicit. |
| Amended A8 | CONFLICTING | "Save/load preflight and field round-trip remain" does not restate the rev6 A8 contract (exact command, environment, field list, evidence format, crash check, rig-unavailable = BLOCKED) and conflicts with the A6 "F11 subset" wording. "Corrupt/truncated injection required if a safe seam exists; otherwise record BLOCKED" is undefined (what seam, what is recorded, does it block A8?). |

Conclusion: the amendment is weaker or ambiguous on every criterion it touches except the methodological direction. It cannot be approved until the rewritten A1/A3/A5/A6/A8 are complete, and A2/A4/A7 must be explicitly declared unchanged.

### Q3. Is the host-side codec test target compatible with the frozen S1 allowlist? Exact paths/CMake changes to authorize.

**No — not under the current frozen allowlist (rev6 §4 spec.md:104).** The allowlist contains only CoSave.h, CoSave.cpp, DebugHarness.cpp, two SPECS docs, tools/run-batch-test.ps1, build/slices/S1/**, build/audit/S1-*, evidence/S1/**, EXECUTION-STATE.json, HISTORY.jsonl. A host target requires files/directories it does not include. Additional constraints verified from the tree:
- PCH.h includes RE/Skyrim.h and SKSE/SKSE.h; Core.h includes PCH.h; CoSave.h includes Core.h; Progression.h and MaterialRegistry.h include Core.h. Therefore CoSave.cpp/CoSave.h cannot be compiled host-side as-is, and registry state cannot be asserted in a host test.
- Existing host-test infrastructure: CMakePresets "test-windows" (PLUGIN_TESTS_ONLY=ON, binaryDir build/test-windows, VCPKG_MANIFEST_FEATURES=tests), Catch2 in test/ExampleTests.cpp; standalone host example tests/ladder_host.cpp; vcpkg.json already has a tests feature (no vcpkg change needed).

Minimum authorization (exact):
1. NEW: src/plugin/src/CoSaveCodec.h and src/plugin/src/CoSaveCodec.cpp — pure codec/state-store, no PCH.h/Core.h/RE/SKSE includes; fixed-width types; token callback; load/store bytes. CoSave.h/CoSave.cpp remain the SKSE adapter (already allowed).
2. NEW: src/plugin/test/CoSaveCodecTests.cpp — Catch2 host tests (failure-pure decode, header, migration fixtures, determinism, dup policy, failure classes, allocation guards, budget).
3. EDIT: src/plugin/CMakeLists.txt — add CoSaveCodec.cpp to RUNEFALL_CORE_SOURCES, and add the test source to the PLUGIN_TESTS_ONLY target (or a dedicated EXCLUDE_FROM_ALL host target modeled on tests/ladder_host.cpp). No CMakePresets edit is strictly required if the existing test-windows preset is reused; authorize CMakePresets.json only if a dedicated preset is added.
4. OPTIONAL/ALLOWED-ALREADY: build/slices/S1/tools/* scripts and fixtures; a host-test runner script under build/slices/S1/tools is already inside the allowlist.
5. No vcpkg.json or workflow changes required for one host target.

Commands to pin in the spec: cmake --preset test-windows; cmake --build --preset test-windows; ctest --preset test-windows --output-on-failure (or the exact new target). If the lead refuses a CMakeLists edit, the only allowlist-compatible alternative is a build/slices/S1/tools script compiling the pure codec directly with a pinned compiler path/flags — but that is the weaker option; a CMake/CTest target is recommended by METHODOLOGY §3 and REFERENCE-REVIEW design implication 6.

Registry reset assertions (A1) must stay in the in-game F11 evidence layer (they depend on SKSE), or the amendment must require an injectable registry-reset sink in the pure state-store. Pick one explicitly.

### Q4. Are duplicate-key policy, atomic publish/registry reset, migration, downgrade and runtime integration still covered?

- Duplicate-key policy: partially. A5 names duplicate registryId and known-section duplicates, but map/override policy is deferred; duplicate raw container/quest entries are unaddressed; no token/post-state evidence. BLOCKING (BA5).
- Atomic publish/registry reset: intent covered but self-contradictory as written (BA2/BA3), and the three-registry assertion has no feasible evidence layer as stated.
- Migration: representative requirement is right but weaker than rev6 pinned assertions (BA4).
- Downgrade: not mentioned in the amendment at all. Rev6 A4 must be explicitly retained and the rejection/warning token in OnLoad must stay distinct (BA9).
- Runtime integration: A8 is referenced but not restated; the "all A1-A7 in F11" vs "host subsets" conflict must be resolved (BA7).

### Q5. What from rev6 must remain unchanged? Hidden omissions/contradictions with MEO/Codex/Calamity patterns?

Must remain unchanged (and be declared as such in the amendment): §1 canonical v1/v2/v3 header table; A2 header/doc/decoder evidence; A4 downgrade gates; A7 budget/caps constants and oversize refusal; §3.1 cap/range table as the data contract (it stops being the acceptance matrix, but its bounds/tokens stay normative); §3.2 ordering/eviction rule (smallest-FormID eviction, canonical order, dedupe); §3.3 budget arithmetic; §3.4 OnLoad record-loop semantics (zero/one/multi/unknown/malformed-then-valid); §4 governance (with the host-target allowlist amendment); §5 C1 mapping (with A3/A5/A6 rows rewritten); §6 out of scope.

Hidden omissions/contradictions against the reference patterns:
- CodexOfPowerNG: split SerializationStateStore from the callback, Snapshot -> ReplaceState, host C++ tests, CMake/CTest. The amendment names the split in "Why" but does not list it as a required deliverable/path; without BA8 the split cannot be built or audited.
- calamity-reactive-loot-affixes: bounded DrainRecordBytes for unknown records, per-record version/length validation, rebuild derived state after load. Rev6 §3.4 says unknown record types are "ignored"; the amendment does not add the bounded-drain requirement or its test. Add it to A6 class 9 and the OnLoad seam.
- MEO: newer-save loads must warn loudly that saving would destroy unread records. Rev6 A4 rejects newer schema but does not state this consequence; the amendment is silent. Add the consequence + token to A4/A8 wording (without changing behavior).
- Adaptive Attributes (weak reference): unchecked ReadRecordData results; Runefall host tests must include short-read/truncated-record variants per section (A6 class 3) and assert failure-pure behavior — state this explicitly.
- METHODOLOGY §4 requires a "Reference review" section in the slice spec or ADR. Rev6 has none; amendment has "Reference evidence" but not a conforming section. Add a reference-review section to the merged revision (BA10).
- Governance: amendment approval must update remediation.S1.contract (rev6 spec hash is the only frozen contract). With a separate amendment file, the gate cannot bind both documents. Merge into rev7 or add state contract.amendments[] with an explicit precedence table (BA1).

## 2. Blocking amendments (exact)

BA1. Merge and freeze: publish spec.md revision 7 that incorporates amendment-1, or record contract as spec rev6 + amendment-1 (path + sha256) with an explicit precedence table that lists every replaced rev6 clause (A1, A3, A5, A6, A8 and §5 mapping rows) and every retained clause. Update remediation.S1.contract and HISTORY on approval. Without this the contract is not frozen and the gate cannot check it.

BA2. A1 two-level contract: (a) codec API is failure-pure — Decode(bytes) returns a candidate/optional; rejected input leaves the caller destination untouched; (b) OnLoad is fail-closed — on reject/no-record it publishes empty/default state exactly once, clears CoSave state, ItemRegistry, Progression and MaterialRegistry, logs the fixed token. Required seam: LoadRecords(vector<uint8_t>, recordVersion, State&) with zero/one/multi/unknown/malformed-then-valid cases; OnLoad callback code inspection allowed only for the GetNextRecordInfo/ReadRecordData plumbing. Delete the "never changes live state" sentence or scope it to the codec API only.

BA3. A1 registry defaults: name the exact expected default state for all three registries and the evidence layer for each (host codec test for CoSaveState + tokens; in-game F11 for ItemRegistry.empty(), Progression defaults, MaterialRegistry defaults). If host assertions are required, authorize an injectable reset interface in the pure state-store; otherwise state clearly that host tests cover codec/publish only and F11 covers registry defaults. Remove "or executed seam if available" and name the required seam.

BA4. A3: retain rev6 A3 in full — fixtures schema1.bin/schema2.bin/baseline-v3.bin with committed SHA-256 and derivation commands; pinned expected table including bossKillTier retained, XP0, class empty, endgame false, v1 rank/stage=1, v2 tier5->rank/stage 100 and tier2->40, difficultyProfile=255, curveId=1; baseline-v3 loads; re-encode canonical. Data values become samples, but these assertions are migration behavior, not data enumeration.

BA5. A5: state the exact duplicate policy as code invariants with tokens: items duplicate registryId -> reject whole record, token co-save: reject item.registryId (decoder-side crafted payload, destination unchanged); known-section duplicate -> existing token co-save: reject duplicate-section; map/override duplicate key -> choose reject with a dedicated token, or document deterministic last-wins and test it in the host suite; containers/quests duplicate entries -> dedupe on decode/encode and test. Add eviction-at-cap test retention (smallest FormID) from rev6 §3.2.

BA6. A6: replace the row matrix with a class table that includes the 10 named classes plus (11) declared count vs remaining bytes / allocation guard even when the count is within cap, (12) absent/zero-length required section and empty payload policy, (13) duplicate raw container/quest entries, (14) exact boundary sizes (at cap, cap+1, record-size boundary). For every class: crafted payload constructed in host test, exercised through the real decode path, exact fixed token, decode false, destination unchanged, no partial publish, no unbounded allocation. Add a class->token mapping table. Explicitly retire rev6 A6 line 28 and §5 A6 row ("one negative per table row") and state that in-game F11 is regression-only, not the codec oracle.

BA7. A8: restate rev6 A8 verbatim (exact batch command, clean-save manifest, environment, pre/post field list, evidence file format/cwd/exit/sha256, crash-log check, rig unavailable = BLOCKED) and add: F11 runs the amended self-test suite (not a per-row matrix); corrupt/truncated co-save injection is BLOCKED unless a documented safe seam exists, in which case define the injection method and expected tokens; the A8 PASS condition must state exactly which sub-items may be BLOCKED and which are mandatory.

BA8. Host target authorization: amend the allowlist to add src/plugin/src/CoSaveCodec.h, src/plugin/src/CoSaveCodec.cpp, src/plugin/test/CoSaveCodecTests.cpp and src/plugin/CMakeLists.txt (plus CMakePresets.json only if a new preset is added). State the pure-codec constraint (no PCH/Core/RE/SKSE includes; tokens via callback) and the exact build/test commands. Update the C1 diff check to cover these paths.

BA9. Retain A4 explicitly (distinct record-version/schema gates and tokens, empty post-state, schema 0 rejected, in-game newer-save simulation BLOCKED) and add the MEO-derived consequence statement: a rejected newer save means the unsupported data is not read; document that the next save would overwrite it (loud warning token), so players are not silently degraded.

BA10. Governance/reference closure: add a conforming "Reference review" section to the merged revision (Codex split/host-test adoption; Calamity bounded drain adoption; MEO reset-then-read rejected; Adaptive Attributes weak read handling improved). Add the bounded-drain requirement for unknown record types (with token + seam test). Record the amendment approval in remediation.S1.contract/HISTORY. Fix the carried-forward C1 enforcement wording (audit-11: literal command git diff a50b2b6..HEAD includes pre-C1 S0 paths); require a grandfathered manifest or baseline reset for C1.

## 3. Verdict

Amended A1: REJECT (contradiction, missing evidence layer). Amended A3: REJECT (weakened). Amended A5: REJECT (policy deferred). Amended A6: REJECT (incomplete classes, no token map, conflicting old text). Amended A8: REJECT (conflicting/undefined). A2/A4/A7: acceptable only if explicitly retained unchanged. Host-test approach: correct direction, not authorized.

Overall: **REJECTED — revise as spec rev7 with BA1-BA10; re-audit before implementation.**
