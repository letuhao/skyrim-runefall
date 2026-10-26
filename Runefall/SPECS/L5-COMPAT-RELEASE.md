# L5 - Compatibility, packaging and release

Status: draft 2026-10-26. Final design layer. Depends on L3/L3E/L4, D-040..D-055 and the ship
gate / FOMOD / Nexus publishing requirements. Purpose: define dependencies, compatibility
passes, module packaging, release gates, publishing deliverables and support policy.

## 1. Dependency matrix

| Dependency | Type | Version policy | Why |
|---|---|---|---|
| Skyrim SE/AE 1.6.1170 | hard (release and test target) | pinned | supported game version (D-062) |
| Skyrim SE/AE 1.7.104 | deferred / unsupported in v1 | not validated | modding ecosystem not ready; no SKSE 2.3.1 / AL v13 support requirement (D-062) |
| SKSE | hard | 1.6.1170 build (record exact tested build) | DLL loader |
| Address Library | hard | 1.6.1170 bins (versionlib-1-6-1170-0.bin and -0-1) | relocation data |
| CommonLibSSE-NG | build-time | **alandtse fork v14.0.1** (commit 723cfda480ae, 2026-10-25) | DLL framework; active fork with 1.7.99+ support |
| SKSE Menu Framework | hard for the UI module | **3.18 release** (Nexus 120352; DEV only for debug DLLs) | v1 UI pages |
| MCM Helper | optional fallback | tested version range | fallback UI only |
| SkyUI | not required | - | no hard SkyUI dependency |
| JContainers / PO3 Papyrus Extender / UILib / PapyrusUtil | not used | - | dependency hygiene (L4) |
| OWL / DiabloRim / Worlds Dawn / overhauls | optional compatibility | tested list versions | compatibility suite |

Rules: no optional mod is a hard requirement of Core; UI features degrade to MCM or message
boxes; compatibility patches live in the Patches pack, never in Core.

## 2. Compatibility pass matrix

| Target | Risk | Pass | Status |
|---|---|---|---|
| OWL + OWL Randomized Special Loot | loot double-application | additive mode; no leveled-list edits; test loot volume | to test |
| DiabloRim | two loot engines | loot authority mode; optional patch disables its distribution | to test |
| Worlds Dawn | overlapping RPG loot/procs | no AV hijack; additive loot; patch for proc overlap | to test |
| Fortunate Foes | boss density/unique overlap | spawn rank caps; density config; patch | to test |
| Perk overhauls (Adamant, etc.) | skill/AV interactions | abilities only; no perk tree edits | to test |
| Enchanting/alchemy/armor overhauls | stacking | percentage caps; additive bonuses | to test |
| Survival mods | food/rest, damage | no AV repurposing; caps | to test |
| Boss overhauls | rank stacking | caps and density config | to test |
| UI stacks (SKSE Menu Framework, SkyUI, Prisma UI) | menus | framework pages coexist; no vanilla UI replacement | to test |
| Leveled-list/container mods (LLOS, CDF) | loot routing | runtime injection only | to test |
| DynDOLOD/Occlusion/world mods | world edits | none in our mods | n/a (expected safe) |

Compatibility status values: PASS, PASS WITH PATCH, KNOWN LIMITATION, FAIL (must be resolved or
disclosed before release). No untested item may be published as "compatible".

## 3. Version matrix and support policy

| Runtime | Status | Notes |
|---|---|---|
| 1.6.1170 (AE) | **release and test target** | Mantras of Mara stock game, pinned; Phase 0 runtime verified |
| 1.6.1179 (GOG AE) | best effort | same 1.6 branch; untested until available |
| 1.7.104 (AE) | **deferred / unsupported in v1** | SKSE 2.3.1 and AL v13 are Nexus-login gated and the plugin ecosystem is not ready (D-062); multi-runtime build retained, no v1 support claim |
| 1.5.97 (SE) | reserved (F-32) | not supported in v1 |
| VR 1.4.15 | reserved (F-31) | multi-runtime build possible later |
| Consoles | unsupported | SKSE requirement; state clearly |

Game-patch policy: v1 supports the pinned 1.6.1170 runtime. 1.7.x is not a supported target until
SKSE 2.3.1, Address Library v13 and the surrounding plugin ecosystem stabilize (D-062); if a user
reports 1.7.x issues, the answer is "unsupported, use the 1.6.1170 target". 1.6.1179 GOG is best
effort. Never claim 1.7.x compatibility without a full smoke test.

## 4. Packaging and FOMOD module matrix

Archive root (no extra folder, no nested Data):

    Data
      Runefall.esp                 (required core ESL)
      Runefall - Unique.esp        (optional unique-affix pack)
      Runefall - Hunt.esp          (optional hunt pack)
      Runefall/...                 data files, runefall.ini, pack manifests
      Scripts/...                  RunefallNative.pex, RunefallProbe.pex, quest scripts
      SKSE/Plugins/Runefall.dll
    fomod
      info.xml
      ModuleConfig.xml

Actual v1 module matrix (reconciled 2026-10-27):

| Module | Required | Depends on | Contents |
|---|---|---|---|
| Core | yes | SKSE 1.6.1170 + Address Library 1.6.1170 | base ESL, data, DLL, core scripts |
| Unique | no | Core | 40 boss/quest unique affixes |
| Hunt | no | Core | 19 race families, hunt uniques/materials |

Reserved for later expansions (not shipped in v1): Bosses, Affixes, Quests, Patches, UI,
Balance. They are described here as future modules only; the FOMOD must not offer them until
their plugins/data exist. FOMOD rules still apply: one canonical Core, packs are deltas, explicit
names/descriptions, UTF-8 XML, validate every branch, verify archive root, hash members.

## 5. Ship gates (adapted to available tooling)

Required gates and how they are satisfied in this project:

| Gate | Requirement | Tooling |
|---|---|---|
| Version gate | active version, changelog, ownership correct | versioned workspace, CURRENT/CHANGELOG |
| Diff gate | no unrelated/deployed files modified | git diff once repo exists; read-only snapshots |
| Plugin gate | header audit: HEDR, ESL flag, masters, FormID ranges, no vanilla overrides | Runefall.Build validator (Mutagen) |
| Papyrus gate | scripts compile and packaged PEX match build | PapyrusCompiler from MO2 Stock Game |
| Runtime-config gate | structural + semantic audit of our data; installed framework versions recorded | build validator + data schemas |
| Native gate | every supported runtime builds; dependencies inspected | CMake build matrix, dumpbin-style import audit |
| Asset gate | referenced paths exist; generated outputs identified | asset list checker |
| FOMOD gate | XML parses; referenced sources exist; branches validated | FOMOD validator script (to build) |
| Archive gate | release tree validator; manual root inspection | runefall release-tree validator |
| Disclosure gate | list unperformed tests | VALIDATION.md + Nexus page |

Tooling gaps to resolve before release: houseCARL/Skyrim Forge are not installed here, so the
ship-gate references to forge plugin-info are replaced by our Mutagen validator until those MCP
tools are installed; record this substitution in VALIDATION.md. Papyrus compiler and xEdit tools
are available locally.

Every release produces the standard completion report (INPUTS, MEMORY CHECK, AUTHORITY, VERSION
GATE, EXACT BUILD/COMPILE, FRAMEWORK VALIDATION, SEMANTIC TEST, SHIP GATE, FINAL ARCHIVE, RUNTIME
STATUS, SSEEDIT/CK, UNRESOLVED).

Runtime reconciliation: record packs_discovered, packs_validated, packs_enabled,
packs_disabled, data_rows_accepted, data_rows_rejected, unresolved_editorids, features_disabled.
A pack with zero accepted rows fails the release.

## 6. Test plan

| Level | What | Status |
|---|---|---|
| Build | DLL multi-runtime build, ESL generation, data validator, Papyrus compile | automated |
| Structural | data cross-refs, FormID ranges, pack manifests, FOMOD XML | automated |
| Minimal profile | SKSE + base only: load, new game, one hunt kill, one loot roll, save/load | manual |
| Copied Mantras profile | full feature smoke, compatibility passes, performance | manual |
| Save matrix | new game, existing save install, uninstall, pack disable, pack re-enable | manual |
| Performance | per-hook timings, 1,000-item save, co-save size | manual instrumentation |
| 1.7.x | NOT a v1 target; deferred until the ecosystem supports it (D-062) | later |
| Publishing | media capture from final archive, page proofread | manual |

Uninstall and pack-disable matrix (N-10):

| Action | Expected |
|---|---|
| Remove UI pack | menus disappear; gameplay unaffected |
| Remove Hunt pack | race rewards skipped; general loot works; state kept |
| Remove Unique pack | uniques not awarded; held flags preserved |
| Remove all packs, keep Core | reduced content, no crash, no lost progression |
| Remove Core + DLL | items fall back to base + engine-native name/enchantment; log explains |

## 7. Release process

1. Version bump follows semantic versioning: MAJOR = data format or save break, MINOR = new packs
   or features, PATCH = fixes.
2. Changelog and all version strings (DLL metadata, plugins, FOMOD, README, page) must match.
3. Build the release tree, run every gate, produce hashes, archive.
4. Beta discipline: pre-release to testers on a copied profile and a clean profile before public.
5. Publish only evidence-backed claims; never "tested" without VALIDATION entries.
6. Hotfix: patch version, same gates, concise changelog; previous archive stays available.

## 8. Nexus publication deliverables

- Short summary and a BBCode page with the hierarchy in the publishing skill (value proposition,
  features, requirements, install/update/uninstall, compatibility, limitations, troubleshooting,
  credits, changelog).
- Media folder + NEXUS-MEDIA-PLAN.md: one 16:9 hero, four to eight images (feature, proof, detail),
  one short video/GIF for the loot/hunt loop, captions and alt text, no fabricated gameplay media,
  disclose other visual mods/ENB.
- Requirements split: hard (SKSE, Address Library, game version), optional (SKSE Menu Framework for
  UI), tool-only (none).
- Credits/permissions for every third-party asset/code source; CommonLibSSE-NG MIT; no Bethesda
  assets shipped; mined FormKeys/EditorIDs used only as references; original unique affix names.
- AI disclosure: state that content/data design and code were produced with AI assistance where
  required by Nexus policy.
- Known limitations and unperformed tests listed exactly.

## 9. Risks and mitigations

1. **Game update breaks the DLL**: pinned CommonLibSSE-NG, multi-runtime build, 72h triage policy.
2. **Antivirus false positives**: reproducible build, hashes, source repo, clear install docs.
3. **Compatibility claims without tests**: status matrix with explicit PASS/LIMITATION; disclose.
4. **Pack fragmentation**: manifest validation, install matrix, one canonical Core.
5. **Scope creep from the future register**: scope lock (D-047); reserved items never enter v1.
6. **Permissions**: no third-party assets without license; source references documented.
7. **Support load**: issue template, log path, required evidence, documented response expectations.

## 10. Decisions recorded by this layer

- D-056 Dependency policy: SKSE + Address Library hard; SKSE Menu Framework for the UI module;
  no JContainers/PO3/PapyrusUtil/UILib; MCM fallback only.
- D-057 Compatibility pass matrix and patch-pack policy; untested items are disclosed, never
  claimed compatible.
- D-058 FOMOD module matrix, one canonical Core, deltas-only packs, XML and archive validation.
- D-059 Release/version policy: semver, beta discipline, game-patch triage, hotfix path.
- D-060 Ship-gate adaptation to available tooling, with the Forge/houseCARL gap disclosed and our
  Mutagen validator substituting; completion report required per release.
- D-061 Nexus publishing deliverables, media plan, credits/AI disclosure and evidence-only claims.