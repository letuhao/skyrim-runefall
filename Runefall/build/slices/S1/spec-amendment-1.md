# S1 acceptance-contract amendment 1 — data-driven audit correction

Status: DRAFT — requires independent specification audit before implementation
Applies to: build/slices/S1/spec.md revision 6
Author: Lead, 2026-10-29
Reference: _context/code-index/REFERENCE-REVIEW.md, build/audit/METHODOLOGY.md, ADR-001
Reference review status: completed 2026-10-29; CodexOfPowerNG, calamity-reactive-loot-affixes and isekai-hero-skyrim indexed and read.

## Reference evidence (2026-10-29)

The reference index now includes 12,114 files / 40,454 term hits, including the three Prisma UI
source trees. Concrete patterns:

- CodexOfPowerNG: load stages into a SerializationStateStore::Snapshot and calls ReplaceState only
  after the record loop; save snapshots and runs all writers; host C++ tests under tests/*.test.cpp;
  CMake COPNG_BUILD_CPP_TEST_TARGETS + CTest; scripts/test.sh. Its atomicity JS test greps source
  rather than executing load; Runefall must use a behavioral host test instead.
- calamity-reactive-loot-affixes: split Save/Load/Lifecycle/StatMigration; bounded DrainRecordBytes
  for unknown records; per-record version/length validation; rebuild derived state after load;
  inventory-dependent migration deferred to post-load.
- isekai-hero-skyrim: source-decidable checks separate from a read-only in-game SelfTest for
  runtime-only facts; no SerializationInterface found in its indexed tree.

These patterns require the amendment below: split pure codec/state-store from the SKSE callback,
add host tests, drain unknown records, and test failure classes rather than per-row data.

## Why

S1 revision 6 made A6 a per-row data/token matrix and A5/a A3 partly data-value checks. Skyrim is
data-driven; runtime data cannot be exhaustively enumerated. That requirement produced
self-referential token-table tests and blocked closure without proving loader behavior. The
contract must audit loader code/contracts and invariants, using data only as representative inputs.

## Changes

### A1 — atomic load and registry reset

- Decode/publish is failure-pure: rejected input never changes destination or live state.
- On success, only then publish; on failure/no record, clear CoSave state and ItemRegistry,
  Progression, MaterialRegistry to their defined defaults.
- Required evidence: a loader seam test for zero/one/multi/unknown/malformed records; explicit
  default assertions for all three registries; OnLoad callback code inspection (or executed seam if
  available). No data-value enumeration.

### A3 — migration behavior

- Representative v1/v2/v3 payloads exercise the real migration function; assert migrated rank/stage/
  difficulty/curve and that invalid/unknown records are skipped or rejected.
- Fixtures are samples, not an exhaustive data audit.

### A5 — duplicate/canonical policy (code invariant)

- Encoder rejects duplicate registryId; decoder rejects a crafted duplicate-registry payload.
- Known-section duplicates rejected; map/override duplicate policy documented and tested at class
  level. Deterministic ordering and encode(decode(encode)) byte equality remain.

### A6 — failure classes, not per-row data

Replace the 27-row token matrix with one representative case per failure class, exercised through
the real decode path:

1. bad schema / record version / flags / version mirror
2. section count zero/too large
3. truncated record / trailing bytes
4. duplicate known section
5. collection count over cap
6. string over length
7. numeric field out of range
8. item identity duplicate/zero
9. unknown record type / unknown section policy
10. oversized record / budget refusal

For each: fixed token, decode false, no partial publish, no unbounded allocation. Token setter/getter
tests are not sufficient. A host codec test may supply crafted payloads; in-game self-test may cover
a subset.

### A8 — runtime integration

- Save/load preflight and field round-trip remain.
- Corrupt/truncated co-save injection is required if a safe seam exists; otherwise record BLOCKED.
- Build proof and reproducible evidence remain.

## Explicitly not weakened

- Header format, migration behavior, atomicity, caps, deterministic encoding, budget, runtime
  save/load, no-crash behavior remain mandatory.
- The amendment removes data-value enumeration, not behavior requirements.

## Review questions for the spec auditor

1. Does each criterion now test loader/runtime behavior rather than generated data?
2. Is the failure-class list complete enough for S1's threat model?
3. Is the host-codec-test approach compatible with the frozen S1 allowlist (add test target under
   build/slices/S1/tools or a CMake test target)?
4. Are any old requirements lost that must be retained?
