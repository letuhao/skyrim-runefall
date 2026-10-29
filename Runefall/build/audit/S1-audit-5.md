# S1 co-save implementation re-audit 5 — candidate 576a840

Subject: **576a840733ac94b97d071d2cfb5a77969aa93afd** (HEAD; includes C1 adcc529, 3cc6ef1, 0baf215).
Baseline a50b2b6. Spec build/slices/S1/spec.md **rev 6**, SHA-256 E94ECAEFF8375ADF282B93F348370AD6677733EE4EDF7F19D1D78F7E17EC9427 (= remediation.S1.contract).
Prior audits read: S1-audit-3.md (FAIL @4411d3b), S1-audit-4.md (FAIL @adcc529). Independent; no implementer history.
Files audited: CoSave.cpp (1048 lines) / CoSave.h, fixtures/PROVENANCE.md, expected_fields.json, verify_fixtures.py, decode_cosave.py, export_fixtures.py, evidence/S1/*, state, HISTORY.

## 1. Read-only commands actually run (captured output)

| Command (cwd Runefall/) | Actual result |
|---|---|
| `python build/slices/S1/tools/verify_fixtures.py` | exit 0, RESULT PASS (10 checks; v2 tier5→100, tier2→40; v1 header/payload; canonical==baseline; ladder fields) |
| `python build/slices/S1/tools/decode_cosave.py <4 fixtures>` | exit 0; schema1/ver1/count9/trailing0; schema2/ver2/count9; schema3/flags0/ver3/count10/trailing0 for baseline+canonical |
| `powershell tools/audit-gate.ps1 -StatePath EXECUTION-STATE.json -Mode truthfulness` | exit 0, `{"ok":true,"code":"GATE-PASS","slices":48,"active":"S1","overall":"IN_PROGRESS"}` |
| `git rev-parse HEAD` / `git diff --name-only adcc529..HEAD` / `git status --porcelain` | HEAD 576a840; candidate files = CoSave.cpp/h, run-batch-test.ps1, verify_fixtures.py, evidence/S1/** , state, HISTORY (allowlisted); tree clean except prior untracked S1-audit*.md |
| CoSave.cpp inspection + node hash of embedded canonicalHex vs committed fixture | canonicalHex bytes == canonical_v3.bin (192 B, SHA **DC78DF40…**); baseline_v3.bin identical bytes |
| Coverage probe of §3.1 token rows | 13 of 32 spec rows have a matching `co-save: reject <row>` text; 19 rows missing (list in A6) |
| Not run: native build, static gates, in-game batch (game/user-side) | batch/self-test/static-gate claims inspected in committed evidence, not independently reproduced |

## 2. Criterion verdicts

| ID | Verdict | Evidence / exact remaining gap |
|---|---|---|
| S1-A1 | **PARTIAL** | FAIL→improved: malformed-then-valid now tested (SelfTest LoadRecords repair records, CoSave.cpp:1029-1033; loop continues on invalid :680-685); zero-record token now spec text "co-save: no record; defaults" (:762, matches §3.4); zero/one/multi/unknown seam tests :1010-1028; Decode failure-pure via candidate+assign (:387-394) and sentinel tests :854-861, :999-1008. **Gaps:** (a) OnLoad failure path still not exercised by SelfTest (seam only) and no assertion that CoSave/ItemRegistry/Progression/MaterialRegistry reach exact default state; (b) "explicit clear" of Progression/MaterialRegistry is still implemented as `LoadFromCoSave()` on a reset state (:757-762), not a clear call — semantics undocumented; (c) LoadRecords(empty) asserts only return false, not post-state bytes. |
| S1-A2 | **PASS** | v1/v2 12-byte and v3 16-byte layouts match code and docs (POWER-LADDER.md:178, L4-RUNTIME.md:117/134); SelfTest asserts header bytes :828-831 and logs full **16-byte** hex :832-834; decode_cosave.py reproduces all four fixtures exactly. |
| S1-A3 | **PARTIAL** | Fixtures+hashes committed; PROVENANCE.md honest (one-off pre-S1 writer mirror at 37dfb68, explicitly not a compiled export); baseline/canonical identical 192 B SHA DC78DF40…; engine now decodes canonicalExpected in SelfTest (:941-948), and canonical bytes == baseline bytes, so v3 fixture decoding is engine-evidenced. **Gaps:** (a) v1 pinned migration (rank=stage=1, difficulty=255, curve=1) is still only a tautological check in verify_fixtures.py (compares expected_fields.json literals to constants; does not derive from the fixture or engine MigrateLegacy); engine SelfTest migration covers schema2 tier5 only, no schema1 case; (b) schema2_tier2 fixture not engine-loaded. |
| S1-A4 | **PARTIAL** | Distinct gates/tokens exist and are now asserted by exact text: LastRejectToken()=="co-save: reject schema" for schema0/4 (:863-878) and =="co-save: reject record-version" for recordVersion 4/0 (:871-874); schema>3 rejected before decode. **Gaps:** rejected-state post-state not asserted at LoadRecord seam (only Decode-level sentinel purity); OnLoad-level default-state behavior untested; in-game newer-save simulation BLOCKED per spec (accepted). |
| S1-A5 | **PARTIAL** | encode(decode(encode)) byte-identity now asserted (:851-852); current encoder reproduces embedded canonical golden (:944-945) = committed canonical_v3.bin (verified); deterministic unordered-map reorder test (:906-931); decode canonicalizes sorts/dedupe (:647-651); smallest-FormID eviction tested (:1035-1043). **Gaps:** spec verification requires a duplicate test — none exists (duplicate registryId is rejected during decode at :652-655 but no SelfTest/duplicate-key case); duplicate policy for ladderOverrides/other collections undocumented. |
| S1-A6 | **PARTIAL (was FAIL)** | Fixed: distinct decode tokens for item.registryId/rarity/ilvl/socketCount/enhancementCount and **item.tags** known-mask (:504-524); section-count token text now exact "co-save: reject section-count" (:437); token capture in LastRejectToken. **Gaps:** 19 of 32 §3.1 rows still have no fixed token — item-count, item.baseFormId, item.socketRunes, item.category, item.tier, hunt-count, material-count, pack-count, container-count, quest-count, override-count, skill-count, string(.class/.skill/.map-key/.pack/.affix), range; those paths return false/log unrelated text (e.g. socketRunes :508-510, category/tier :519-520, count caps :481/494/530/543/560). "co-save: reject trailing bytes" ≠ spec "co-save: reject trailing"; duplicate-section text carries id. SelfTest still has no per-row decode negatives and no trailing/duplicate-section test. |
| S1-A7 | **PASS** | kMaxEncodedBytes=1,048,576 and kMaxRecordBytes=2,097,152 (CoSave.h); docs state 1 MiB; 1000-item legal case under budget (:960-976); EncodeWithBudget(sample,100) refusal (:998); OnSave refuses before OpenRecord (:700-707). Carried minor residual (same as audit-4): the "encoded bytes exceed budget" token branch is unreachable because Encode() already enforces the cap; oversize state reports "encode cap exceeded". |
| S1-A8 | **PARTIAL** | Evidence metadata now complete vs spec field list: batch-20261028-183923.md carries cwd/command/exit=0/body_sha256; field table includes rank, stage, xp, class, **skillRank**, difficulty, curve, endgame, generatedContainers count+member, **rewardedQuests count+member (firstQuest)**, overrides count, items count+firstItem registryId, materials count, configHash; "No new crash log" PASS; **crash-assessment.log** committed (pre-existing 17:37 BSWin32KeyboardDevice crash, no Runefall frame, reruns 17:41–18:39 PASS, not waived). **Gaps:** (a) recorded body_sha256 E924D5FD… does not reproduce from the committed evidence file under whole-file/body/trailing/trim/LF/CRLF conventions (convention undocumented — provenance unverifiable); (b) header-hex row asserts only an **11-byte prefix** ("524E465303000000000003") of the 16-byte v3 header; (c) no independent in-game re-run by this auditor (game/user-side, per spec only if rig available). |

## 3. Residual closure vs S1-audit-4 (adcc529)

Closed: A1 malformed-then-valid + zero-record token; A4 token capture/assertions; A5 idempotence + current-encoder golden; A6 item.tags and distinct item tokens; A8 skill rank + rewardedQuests member + crash assessment.
Still open: A1 explicit-clear/OnLoad default-state assertion; A3 schema1 migration assertion (tautological) + schema2_tier2 engine load; A4 rejected post-state; A5 duplicate-key test/policy; A6 19/32 token rows; A8 body_sha256 convention + full 16-byte header hex in batch.

## 4. Governance observations (not code)

- `remediation.S1`: state AUDIT_PENDING, status PARTIAL, `commit`=**3cc6ef1** (≠ submitted candidate 576a840), implementationAudit = audit-4 **FAIL** @adcc529; evidence[] lists batch-183341 and self-test.log, but **self-test.log hash now mismatches** its recorded sha (file rewritten in 576a840) and batch-183923 is absent from state evidence. No HISTORY candidate/gate record exists for 576a840 (latest seq 15 = audit-4 FAIL).
- Candidate-only diff (adcc529..HEAD) stays inside the spec §4 allowlist; however the literal C1 enforcement command `git diff --name-only a50b2b6..HEAD` still includes the pre-C1 S0 layer, which the S1 allowlist does not grandfather (same spec-wording gap noted by audit-4) — unresolved.
- This report was written to S1-audit-5.md (Lead-directed path); spec §4 names S1-audit.md, so the audit path/record must be reconciled at C2.

## 5. Overall verdict

**S1 not PASS at 576a840 — closure blocked (PARTIAL).** A2 and A7 pass; A1/A3/A4/A5/A8 are PARTIAL (narrow seams) and A6 remains PARTIAL with 19 of 32 mandated token rows absent. The candidate is a clear improvement over adcc529 (token capture, tags, canonical decode, LoadRecords repair test, crash assessment, A8 fields), but the frozen S1-A1..A8 pass conditions are not all demonstrated.

Remaining closure work: (1) A6 add decode validation+fixed token for every §3.1 row and one SelfTest negative per row (incl. trailing/duplicate-section); (2) A3 derive v1 migration assertions from a decoded fixture/engine MigrateLegacy test; (3) A4/A1 assert rejected/default post-state at LoadRecord/OnLoad level and document explicit clear semantics; (4) A5 add duplicate-key test and document overrides policy; (5) A8 document/repair body_sha256 convention and print full 16-byte header hex in batch; (6) governance: record candidate commit 576a840 in state/HISTORY, refresh evidence hashes, reconcile audit path.

Auditor independence: read-only; no source/state/fixture/build/game action; only this report written.
