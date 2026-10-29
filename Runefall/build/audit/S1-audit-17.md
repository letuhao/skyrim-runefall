# S1 implementation audit 17 — code candidate a03bdda / state 3be876e / tooling bbb6b96

Subject code: **a03bdda** ("fix(s1): assert fresh registry defaults and finish A6 host matrix"); state child **3be876e** (candidate runtime batch passing); tooling **bbb6b96** (kill skse64_loader too + batch evidence). HEAD on disk = 3be876e; clean tracked worktree.
Contract: rev8 spec SHA-256 **FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1** + approved amendment 2 SHA-256 **E54563976FF17974E22CED1C71483086BCA1276EF6833C35177A2273009F2F95** (approval: S1-spec-amendment-2-audit-rev2; hashes match remediation.S1.contract).
Auditor: independent, read-only; only this report written. Prior: S1-audit-16 (PARTIAL @947bf67), S1-audit-12 (FAIL @3e34031). Date 2026-10-29, timeboxed.

## 1. Read-only commands / checks actually run (captured)

| Check | Result |
|---|---|
| `git rev-parse/log/status`; `git show --stat a03bdda/3be876e/bbb6b96` | HEAD 3be876e, clean tree; a03bdda = CoSave.cpp +8, CoSaveCodecTests.cpp +14, host-tests-1620.txt; 3be876e = EXECUTION-STATE.json; bbb6b96 = run-batch-test.ps1 +11/−6, batch-215510(.raw), build-2149 |
| Host tests **independently re-run**: `src/plugin/build/test-windows-vendored/RunefallTests.exe` | **exit 0, "All tests passed (140 assertions in 16 test cases)"** — matches evidence host-tests-1620 (cwd/command/exit=0) |
| Host test coverage probe (all A6 tokens/classes) | **37/37 A6.1+A6.2 tokens/classes present** in CoSaveCodecTests.cpp (incl. item fields 8b-8j, 7e affix, 13a at-cap, 13b exact-max, 13c optional, 10 drain, 12 dedupe, duplicate-key, budget, missing/empty-section, count-remaining) |
| Batch evidence `evidence/S1/batch-20261029-215510.md` | cwd, command, exit=0, body_sha256 **E0B7303D320663A34969500E86E206ACBEC03D80B46763D61F9ED7B4D52CE224**, dll_sha256 **0F7A5AEDD26A91F3A02A83810EE72C7A91960666FBA6C0EB7E3A1EC9CCE15A5A**, raw_report link; 38 self-tests pass, Save-files-present/load/player-not-Prisoner/cell PASS, full 16-byte header `524E465303000000000003000A000000`, complete pre/post field round-trip, no-new-crash |
| Raw report hash | SHA-256 of `batch-20261029-215510-raw.md` = **E0B7303D320663A34969500E86E206ACBEC03D80B46763D61F9ED7B4D52CE224** = recorded body_sha256 (match) |
| Rig DLL `J:\SkyrimTest\1.6.1170\Data\SKSE\Plugins\Runefall.dll` | 1,512,448 B, mtime 21:51:19; SHA-256 **0F7A5AED…5A5A** = evidence dll_sha256 (independently recomputed, match) |
| Build evidence `build-20261029-2149.txt` | BUILD_EXIT=0, compiled CoSave.cpp ([1/4],[2/4] Building …CoSave.cpp.obj), linking Runefall/RunefallStage, DLL_SHA256 = 0F7A5AED…; DLL mtime 21:51:19 < batch start 21:55:10 |
| State `remediation.S1` | state **AUDIT_PENDING**, status PENDING, commit **a03bdda**, implementationAudit null (this report is the pending record); contract spec+amendment hashes correct; **19 evidence entries all exist and hash-match**; F7-CONSOLE-THREAD followup retained |
| Tooling (bbb6b96 diff) | `Stop-TestGame` now kills **skse64_loader AND SkyrimSE**, 4 attempts, then throws; Steam not targeted; then launches skse64_loader from GameRoot |
| NOT run | game launch, production build, static gates (read-only audit; batch/build claims verified from committed evidence + rig DLL hash + mtime) |

## 2. Key-question answers

- **A1 LoadOrReset + fresh defaults:** codec `LoadOrReset` exists and is host-tested (empty list → false+default, malformed → false+default, valid → true). Adapter SelfTest now builds a fresh `CoSaveState defaults` and asserts cleared state rank/stage/xp/class/endgame/items against it, plus `ItemRegistry::Get().All().empty()`, `Progression::Get().AttributePoints()==0`, `MaterialRegistry::Get().Total()==0` (CoSave.cpp:552-566). Gap from audit-16 (hardcoded rank/registry-unasserted) is functionally closed.
- **A6 matrix:** complete. The 16 host cases exercise the real decode path (`CoSaveCodec::Decode`) with crafted payloads and exact `LastRejectToken()` assertions for every A6.1 row (schema 0/4, record-version, flags, mirror, section-count 0/65, truncated, record-size, duplicate-section, all 5a-5h caps, count-remaining, 7a-7e strings incl 7e affix, 8a range, 8b-8j item fields, 9 registryId, 11a/11b, 14a/14b budget) and A6.2 accepted rows (10 drain unknown-record + later valid, 12 dedupe, 13a at cap=4096 containers, 13b exact kMaxRecordBytes, 13c optional section). 140 assertions/16 cases, re-run exit 0.
- **A5 registryId/duplicate:** crafted duplicate-registryId payload decoded via real path → false + token `co-save: reject item.registryId`; duplicate-key token tested for materials and ladder overrides; duplicate known section tested. Residuals below.
- **A8:** evidence is durable and internally consistent: metadata block + raw report hash + DLL hash + build log + host log; state evidence manifest (19 entries) hash-verified; batch 38/38 with header hex, full field round-trip, clean save, no new crash.
- **Tooling:** `run-batch-test.ps1` now kills both game-side processes (not Steam) with 4-attempt verification before launch — as specified by AGENTS.md §3.2.9.

## 3. Criterion verdicts (exact commit a03bdda)

| ID | Verdict | Basis / notes |
|---|---|---|
| S1-A1 | **PASS** | LoadOrReset + failure purity host-tested; adapter SelfTest asserts fresh-default post-state for CoSaveState and all three registries. *Note:* Progression/MaterialRegistry are compared through default-zero accessors (AttributePoints()/Total()) rather than a freshly constructed default instance object; CoSaveState is compared to a fresh `CoSaveState defaults`. Equivalent for current defaults; wording-level residual only. |
| S1-A2 | **PASS** | Header contract unchanged; batch records full 16-byte header hex 524E465303000000000003000A000000; fixtures/decoder unchanged. |
| S1-A3 | **PASS** | Fixtures/expected fields unchanged; adapter SelfTest retains v1/v2/tier2 migration + canonical decode; batch self-tests 38 pass; verify_fixtures unchanged (prior 10/10). |
| S1-A4 | **PASS** | schema 0/4 and record-version gates tested with distinct tokens; destination purity tested; `co-save: warn newer-save` token present in adapter (in-game newer-save simulation BLOCKED per spec, on-screen warning user-side). |
| S1-A5 | **PASS (residual)** | Crafted decode-side duplicate registryId + exact token; duplicate-key token tested for materials and ladder overrides; duplicate-section tested; canonical/deterministic/eviction unchanged. Residuals: hunt/pack/skill duplicate-key paths have no dedicated host test; destination-unchanged for the duplicate-registryId case is not sentinel-asserted (structurally guaranteed by candidate-then-publish Decode + generic failure-purity test). |
| S1-A6 | **PASS** | 37/37 A6.1/A6.2 tokens/classes present in host tests with crafted payloads and exact-token assertions; 140 assertions/16 cases; independent host-exe re-run exit 0. No self-referential token-setter-only test found. |
| S1-A7 | **PASS** | 1 MiB/2 MiB constants; 1000-item case; EncodeWithBudget refusal with `reject budget` token; OnSave refusal before OpenRecord; docs state 1 MiB. |
| S1-A8 | **PASS** | Batch metadata cwd/command/exit=0/body_sha256 + dll_sha256; raw report SHA = E0B7303D…; rig DLL = 0F7A5AED… = evidence; build-2149 BUILD_EXIT=0 with CoSave.cpp compiled and DLL hash match; host log metadata cwd/command/exit=0; state 19/19 evidence hashes current; batch 38/38, header hex, field round-trip, clean save, no new crash (pre-batch 04:26 crash under F7 waiver). |

## 4. Governance / allowlist

- Amendment 2 is approved (rev2 audit hash matches state) and broadens §4/§5 only for the test harness: vendored Catch2, ItemMetadata.h, CMakePresets.json, evidence/S1/**. `C1-allowlist.txt` (SHA CBB6AF7E16529F58B111DF13F09B8D807CC95CC5CF78DC571C81B04784EB3DD2) now lists these, incl. `src/plugin/test/vendor/*`, `src/plugin/src/ItemMetadata.h`, `src/plugin/CMakePresets.json`, `evidence/S1/*`. The audit-16 allowlist gap (CoSaveCodecExtraTests.cpp) is moot: the file does not exist in a03bdda; extra cases were folded into tracked CoSaveCodecTests.cpp (git ls-files confirms only CoSaveCodecTests.cpp + pre-existing ExampleTests.cpp + vendor).
- Candidate deltas are inside the amended allowlist: a03bdda = CoSave.cpp + test file + evidence/S1 host log; bbb6b96 = run-batch-test.ps1 + evidence/S1 batch/build logs; 3be876e = EXECUTION-STATE.json. No content/fomod/Runefall.Build changes.
- State lifecycle is correct for submission: AUDIT_PENDING / commit a03bdda / implementationAudit null pending this report; F7-CONSOLE-THREAD follow-up retained as accepted crash-safety exception (waiver unchanged).
- Non-blocking tooling risk: `Stop-TestGame` kills by process name without verifying the image path, so a non-test SkyrimSE/skse64_loader running on the host would also be killed. The batch rig is dedicated (J:\SkyrimTest), and the script's launch is rig-scoped, but the kill condition is not path-fenced — record as a rig-hygiene note, not an S1 criterion failure.

## 5. Evidence hashes (independently computed)

```
spec.md                          FADE63AC4AE10973E9746F8E54CE86D17ED967C725E7E2AE941DDAD57E5744C1
spec-amendment-2.md              E54563976FF17974E22CED1C71483086BCA1276EF6833C35177A2273009F2F95
CoSaveCodec.cpp                  2CE4B1AC74E7ABE9240506612E70AA5C408C9F61504871978E1F99EC505FC9D7
CoSave.cpp                       4FE2C18032730AD1B31DB860CBB590712CE3B543DF5B8E08A27F8F923DA60809
CoSaveCodecTests.cpp             04211AFFEDE78E2A70822BF8603E0A0DCA1EF0D884741BB99172581CEE68C3F8
C1-allowlist.txt                 CBB6AF7E16529F58B111DF13F09B8D807CC95CC5CF78DC571C81B04784EB3DD2
evidence/S1/host-tests-20261029-1620.txt   576ED74823A32C7BB22512E5C9401771A4C6FD3998B3BBAE8A23B1CAE423AE94
evidence/S1/batch-20261029-215510.md       97469C74AC05E3FD74F72C4454144083A9F318D682B5F009072A840F60A4C157
evidence/S1/batch-20261029-215510-raw.md   E0B7303D320663A34969500E86E206ACBEC03D80B46763D61F9ED7B4D52CE224
evidence/S1/build-20261029-2149.txt        B65C6B00F0564CBDE3CC7ED761AABC819E58CD713125E3C354A70FF05274F3AA
rig Runefall.dll                 0F7A5AEDD26A91F3A02A83810EE72C7A91960666FBA6C0EB7E3A1EC9CCE15A5A
```

## 6. Overall verdict: **PASS** for exact candidate a03bdda

All A1-A8 criteria are demonstrated on the exact commit: host codec matrix 140 assertions/16 cases (independently re-run exit 0, all A6.1/A6.2 classes covered), A1 fresh-default reset assertions present, A5 crafted duplicate tests present, A8 batch/build/DLL/state evidence durable and hash-consistent, and the tooling kill fix matches AGENTS.md §3.2.9. Remaining items are non-blocking wording/hygiene notes: A1 registries compared via zero/empty accessors rather than default-instance objects; A5 hunt/pack/skill duplicate-key host tests and duplicate-case destination-unchanged sentinel absent; build log relies on incremental [1/4] compile lines (acceptable, DLL hash ties output to source); rig kill is name-based. No false PASS, no false evidence found.

Auditor independence: read-only; no source/state/fixture/gate/build/game action; only this report written.
