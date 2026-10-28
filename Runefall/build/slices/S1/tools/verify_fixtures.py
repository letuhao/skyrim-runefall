#!/usr/bin/env python3
"""Verify S1 migration fixtures against expected_fields.json."""
import json, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4]
FIX=ROOT/"build/slices/S1/fixtures"

def header(b):
    schema=struct.unpack_from("<H",b,4)[0]
    if schema==3:
        flags=struct.unpack_from("<I",b,6)[0]; ver=struct.unpack_from("<H",b,10)[0]; count=struct.unpack_from("<I",b,12)[0]; off=16
    else:
        ver=struct.unpack_from("<H",b,6)[0]; count=struct.unpack_from("<I",b,8)[0]; off=12; flags=None
    return schema,flags,ver,count,off

def sections(b,off):
    out={}
    for _ in range(struct.unpack_from("<I",b,8 if struct.unpack_from("<H",b,4)[0]<3 else 12)[0]):
        sid=struct.unpack_from("<H",b,off)[0]; ln=struct.unpack_from("<I",b,off+2)[0]; off+=6
        out.setdefault(sid,[]).append(b[off:off+ln]); off+=ln
    return out

def check(name, cond, msg):
    if not cond: raise SystemExit(f"FAIL {name}: {msg}")
    print(f"PASS {name}: {msg}")

exp=json.loads((FIX/"expected_fields.json").read_text())
for name in ("schema2_tier5.bin","schema2_tier2.bin"):
    b=(FIX/name).read_bytes(); schema,flags,ver,count,off=header(b); secs=sections(b,off)
    prog=secs[1][0]; vals=list(struct.unpack("<13i",prog)); tier=vals[11]
    want=exp[name[:-4]]
    rank=100 if tier==5 else 40 if tier==2 else 1
    check(name, schema==2 and flags is None and ver==2, "v2 header")
    check(name, rank==want["playerRank"] and tier in (5,2), f"runeTier {tier} -> rank {rank}")
b1=(FIX/"schema1_legacy.bin").read_bytes(); s1,_,v1,c1,o1=header(b1); check("schema1_legacy.bin", s1==1 and v1==1 and struct.unpack_from("<I",b1,8)[0]==9, "v1 12-byte header, 9 sections, legacy defaults rank/stage 1")
secs1=sections(b1,o1); check("schema1_legacy.bin", struct.unpack("<11i",secs1[1][0])==(0,)*11, "v1 progression payload has no runeTier/bossKillTier")
b=(FIX/"baseline_v3.bin").read_bytes(); schema,flags,ver,count,off=header(b); secs=sections(b,off)
lad=secs[11][0]
version,rank,stage,diff,curve=lad[0],lad[1],lad[2],lad[3],lad[4]
overrides=struct.unpack_from("<H",lad,5)[0]; xp=struct.unpack_from("<I",lad,7)[0]; classlen=struct.unpack_from("<H",lad,11)[0]; skillcount=struct.unpack_from("<H",lad,13+classlen)[0]; endgame=lad[15+classlen]
check("baseline_v3.bin", schema==3 and flags==0 and ver==3, "v3 header")
c=(FIX/"canonical_v3.bin").read_bytes(); check("canonical_v3.bin", c==b, "canonical v3 bytes equal baseline golden")
check("baseline_v3.bin", rank==42 and stage==42 and diff==2 and curve==1 and overrides==0 and xp==0 and classlen==0 and skillcount==0 and endgame==0, "ladder fields")
print("RESULT: PASS")
