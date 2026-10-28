#!/usr/bin/env python3
"""Generate S1 golden co-save fixtures from the pre-S1 writer format (commit 37dfb68).
This mirrors CoSave.cpp Encode at 37dfb68 exactly for the selected sample states.
"""
import hashlib, json, struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4]
FIX=ROOT/"build/slices/S1/fixtures"

def u16(v): return struct.pack("<H",v)
def u32(v): return struct.pack("<I",v)
def i32(v): return struct.pack("<i",v)
def section(i,payload): return u16(i)+u32(len(payload))+payload

def header(schema):
    if schema==3:
        return b"RNFS"+u16(3)+u32(0)+u16(3)
    return b"RNFS"+u16(schema)+u16(schema)

def progression_tier(tier):
    p=b"".join(i32(0) for _ in range(9))+i32(0)+i32(0)+i32(tier)+i32(-1)
    return section(1,p)

def progression(schema):
    p=b"".join(i32(0) for _ in range(9))
    p+=i32(0)+i32(0)
    if schema>=2:
        p+=i32(5)+i32(2)
    return section(1,p)

def make(schema, name, sections):
    out=header(schema)
    out+=u32(len(sections))
    for s in sections: out+=s
    p=FIX/name; p.write_bytes(out)
    print(name, len(out), hashlib.sha256(out).hexdigest().upper())
    return out

sections=[progression(2),section(2,b"".join(i32(0) for _ in range(5))),section(3,u32(0)),section(4,u32(0)),section(5,u32(0)),section(7,u32(0)),section(8,u32(0)),section(9,u32(0)),section(10,u32(0))]
make(2,"schema2_tier5.bin",sections)
sections_t2=[progression_tier(2),section(2,b"".join(i32(0) for _ in range(5))),section(3,u32(0)),section(4,u32(0)),section(5,u32(0)),section(7,u32(0)),section(8,u32(0)),section(9,u32(0)),section(10,u32(0))]
make(2,"schema2_tier2.bin",sections_t2)
sections1=[progression(1),section(2,b"".join(i32(0) for _ in range(5))),section(3,u32(0)),section(4,u32(0)),section(5,u32(0)),section(7,u32(0)),section(8,u32(0)),section(9,u32(0)),section(10,u32(0))]
make(1,"schema1_legacy.bin",sections1)
# v3 baseline: same 9 sections plus ladder section 11 (version1, rank42, stage42, diff2, curve1, override count0, xp0, class empty, skill count0, endgame0)
ladder=bytes([1,42,42,2,1])+u16(0)+u32(0)+u16(0)+u16(0)+bytes([0])
sections3=[progression(2),section(2,b"".join(i32(0) for _ in range(5))),section(3,u32(0)),section(4,u32(0)),section(5,u32(0)),section(7,u32(0)),section(8,u32(0)),section(9,u32(0)),section(10,u32(0)),section(11,ladder)]
make(3,"baseline_v3.bin",sections3)
expected={"schema2_tier5":{"playerRank":100,"playerStage":100,"difficultyProfile":255,"curveId":1,"bossKillTier":2},"schema2_tier2":{"playerRank":40,"playerStage":40,"difficultyProfile":255,"curveId":1,"bossKillTier":-1},
 "schema1_legacy":{"playerRank":1,"playerStage":1,"difficultyProfile":255,"curveId":1},"baseline_v3":{"playerRank":42,"playerStage":42,"difficultyProfile":2,"curveId":1}}
(FIX/"expected_fields.json").write_text(json.dumps(expected,indent=2)+"\n",encoding="utf-8")
print("fixtures written")
