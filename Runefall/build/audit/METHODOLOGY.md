# Runefall audit and test methodology (2026-10-29)

This document replaces the data-centric audit loop used in S1.

## 1. Principle

Skyrim is data-driven. Runtime data (forms, leveled lists, generated loot, co-save state) cannot be
exhaustively enumerated. Do **not** audit every data value. Audit:

1. the data contract: schema, version, bounds, types, migration rules;
2. the loader code: validation-before-use, caps, atomic publish, fail-closed behavior;
3. the runtime behavior: save/load, malformed input, downgrade, reload, registries, logging;
4. representative fixtures only as regression anchors.

A fixture value is not an acceptance target. The loader's response to generated/malformed input is.

## 2. Evidence layers

- **Contract evidence:** schema/version table, exact header layout, migration mapping.
- **Codec evidence:** round-trip, idempotence, deterministic ordering, encode/decode symmetry,
  duplicate policy, caps, budget.
- **Failure evidence:** one representative case per failure class (bad version, bad count, bad
  length, bad range, bad string, duplicate, trailing, oversized), fixed token, no partial publish.
- **Runtime evidence:** real save load, malformed/truncated co-save, reload, defaults, no crash.
- **Build evidence:** foreground build exit 0, DLL existence/hash/mtime, test-env copy.

## 3. Test strategy

- Add a **host-side C++ codec test** target. It runs in seconds, constructs arbitrary valid/invalid
  payloads, and exercises codec/migration without Skyrim.
- Use property/fault-injection tests, not a hand-written matrix per data row.
- Assert invariants:
  - decode failure leaves destination unchanged;
  - OnLoad publishes only after success;
  - failure clears/restores all registries to a defined default;
  - encode(decode(encode(x))) is byte-identical;
  - duplicate keys/sections are rejected or documented;
  - caps and budget are enforced before write.
- In-game batch is only the runtime integration gate, with save/load preflight and field
  round-trip. It must abort before gameplay checks if a save is not loaded.

## 4. Reference learning gate

Before writing or changing a native slice:

1. Search `_context/code-index/` for comparable patterns
   (`SerializationInterface`, `OpenRecord`, `GetNextRecordInfo`, hooks, data loaders).
2. Read at least one real implementation and record what it does and what it does **not** prove.
3. Add a short "Reference review" section to the slice spec or ADR.
4. Do not claim compatibility testing from the index; it is code learning only.

## 5. Workflow

1. **Reference review** — index/search/compare; record gaps.
2. **Contract** — behavior classes + data contract; independent spec audit.
3. **Host harness** — write codec tests first; they must fail for missing behavior.
4. **Implementation** — smallest change to pass the host harness.
5. **Build proof** — foreground build, exit 0, DLL hash/mtime.
6. **Runtime preflight** — save files exist, MCP connected, load accepted, player not Prisoner,
   cell formId; abort before gameplay if not.
7. **Runtime integration** — short batch/self-test, raw logs.
8. **Independent audit** — exact commit, per-criterion PASS/PARTIAL/FAIL; no self-review.
9. **Close only on PASS**; otherwise fix root cause/design, not token-level symptoms.

One candidate per gate. Do not enter a micro-edit -> build -> game -> audit loop.

## 6. S1 reset plan

The old S1-A6 per-row token/data matrix is withdrawn. The amended S1 contract should require:

- A6: **failure classes** rejected through the real decode path with fixed tokens; representative
  crafted payloads; no unbounded allocation; no partial state. Not one case per data field.
- A5: duplicate policy as a code invariant, with a crafted duplicate-registry payload and
  documented map/override behavior.
- A1: atomic publish/failure-purity and explicit registry reset, tested at a loader seam and, if
  possible, the real callback.
- A3: representative v1/v2/v3 migration behavior, not every data value.
- A8: runtime integration only after preflight; evidence hash and build proof.

## 8. Reference-derived design patterns (2026-10-29)

- CodexOfPowerNG: split serialization state store from SKSE callback; stage a Snapshot, ReplaceState
  only after the record loop; host C++ tests with CMake/CTest and standalone script; save snapshots
  and runs all writers, logging failures.
- calamity-reactive-loot-affixes: bounded DrainRecordBytes for unknown records; per-record
  version/length validation; rebuild derived state after load; defer inventory-dependent migration
  to post-load.
- isekai-hero-skyrim: source-decidable checks separate from a read-only in-game SelfTest; do not
  use the in-game test as the primary codec oracle.

Runefall S1 should adopt the split/host-test pattern and behavioral atomicity tests, not Codex's
source-grep atomicity test or a per-row data matrix.

## 9. MEO-derived crafting/data rules (2026-10-29)

See _context/code-index/MEO-REVIEW.md. For any future Runefall crafting slice:

1. Canonical data pipeline: authored JSON -> validated generation -> runtime data + generated C++
   catalog/ESP forms. Generator must fail on unknown ids/missing mappings and keep indexes
   append-only. DLL must not parse gameplay JSON at runtime.
2. Stable identity: persist stable string ids (gid), never enum indexes; key per-instance state by
   (baseFormID, uid, slot), with a collision-checked uid range.
3. Runtime resolver: missing forms/families register DISABLED; records go dormant, never deleted.
4. Co-save: versioned fields, ResolveFormID on load, clamp ranges, drop unresolved forms with a log,
   and warn loudly that a newer save cannot be read and saving would destroy unread records.
5. Snapshot target lists before mutation; use engine flows for item/effect changes; never give an
   item two enchant sources.
6. Before a large crafting feature, run a P0-style premise test: item/instance model survives
   save/load, re-equip, stacked items, uninstall. Then build recipes on top.
