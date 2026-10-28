#!/usr/bin/env python3
"""Deterministic legacy checklist normaliser for Runefall state v2."""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

TOKENS = ["PASS","PARTIAL","FAIL","BLOCKED","NOT RUN","NOT_VERIFIED","DEFERRED","N/A"]
MAP = {"NOT RUN":"NOT_RUN","NOT_VERIFIED":"NOT_VERIFIED","N/A":"NOT_APPLICABLE"}

def norm(value: str) -> str:
    up = value.upper()
    if up.startswith("PASS"): return "PASS"
    if up.startswith("PARTIAL"): return "PARTIAL"
    if up.startswith("FAIL"): return "FAIL"
    if up.startswith("BLOCKED"): return "BLOCKED"
    if up.startswith("NOT RUN"): return "NOT_RUN"
    if up.startswith("NOT_VERIFIED") or up.startswith("NOT VERIFIED"): return "NOT_VERIFIED"
    if up.startswith("DEFERRED"): return "DEFERRED"
    if up.startswith("N/A") or up.startswith("NOT_APPLICABLE"): return "NOT_APPLICABLE"
    return "FAIL"

def main() -> None:
    ap=argparse.ArgumentParser(); ap.add_argument("--state",default="EXECUTION-STATE.json"); ap.add_argument("--out",default="build/audit/normalize_legacy.output.json")
    a=ap.parse_args(); root=Path(__file__).resolve().parents[2]
    state=json.loads((root/a.state).read_text(encoding="utf-8"))
    out={"schemaVersion":1,"source":a.state,"normalisation":"case-insensitive prefix; PASS* -> PASS; UNVERIFIED -> PARTIAL; unknown -> FAIL","checklists":{}}
    for sid,sl in state.get("slices",{}).items():
        out["checklists"][sid]={}
        for key,val in sl.get("checklist",{}).items():
            status=val.get("criterionStatus")
            detail=val.get("detail","")
            if status not in {"PASS","PARTIAL","FAIL","BLOCKED","NOT_RUN","NOT_VERIFIED","DEFERRED","NOT_APPLICABLE"}:
                status=norm(detail)
            out["checklists"][sid][key]={"criterionStatus":status,"detail":detail}
    text=json.dumps(out,sort_keys=True,indent=2)+"\n"
    outpath=root/a.out; outpath.parent.mkdir(parents=True,exist_ok=True); outpath.write_text(text,encoding="utf-8")
    print("normalize_sha256="+hashlib.sha256(text.encode()).hexdigest().upper())
if __name__=="__main__": main()
