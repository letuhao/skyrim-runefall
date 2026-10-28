# S1 fixture provenance

- Generator: build/slices/S1/tools/export_fixtures.py (committed).
- Writer source of truth: pre-S1 CoSave.cpp at commit 37dfb68 (ladder payload: version, rank, stage, difficulty, curve, u16 overrideCount, overrides, u32 xp, string class, u16 skillCount, skills, u8 endgame; v1/v2 12-byte header without flags).
- The generator is a one-off format emitter derived from that source; it is not a compiled pre-S1 DLL export and does not claim to be one. It is validated against the pre-S1 writer layout by verify_fixtures.py and by the current decoder.
- baseline_v3.bin / canonical_v3.bin: 192 B, SHA-256 DC78DF40A37CB0D18A8C635550FE4057FA60D8134106F94D35A50CF395CBDE0C.
- schema2_tier5.bin: 166 B, B846584A... ; schema2_tier2.bin: 166 B, 7E35B27D... ; schema1_legacy.bin: 158 B, C06AE63F...
- Expected migration fields: expected_fields.json, consumed by verify_fixtures.py.
- Limitation: an actual save/build export from the pre-S1 binary is not available in this workspace; the format emitter is the strongest reproducible provenance available and is marked as such.
