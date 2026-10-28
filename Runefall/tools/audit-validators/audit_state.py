#!/usr/bin/env python3
"""Truthfulness/release gate validator for Runefall EXECUTION-STATE schema v2."""
from __future__ import annotations
import argparse, hashlib, json, subprocess, sys
from pathlib import Path

PASS_IDS = {"F0.1","F0.4","C1.4","C1.5","R5.1","R5.4","R5.6"}
FAIL_IDS = {"F0.7","F0.9","W2.2","A3.3","A3.7","P4.2","P4.4","P4.5","P4.6","P4.7","R5.3"}
ALLOWED_CRITERION = {"PASS","PARTIAL","FAIL","BLOCKED","NOT_RUN","NOT_VERIFIED","DEFERRED","NOT_APPLICABLE"}
FORBIDDEN = ["48/48 PASS","all gates pass","all 48 slices PASS","first public release","published release","ship gate PASS"]
DOCS = ["STATE.md","RELEASE-READINESS.md","VALIDATION.md","CHANGELOG.txt"]

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()

def fail(code: str, message: str) -> None:
    print(json.dumps({"ok": False, "code": code, "message": message}))
    raise SystemExit(1)

def git(repo: Path, *args: str) -> str:
    r = subprocess.run(["git","-C",str(repo),*args],capture_output=True,text=True)
    return r.stdout.strip()

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--state", required=True)
    ap.add_argument("--repo", required=True)
    ap.add_argument("--mode", choices=["truthfulness","release"], default="truthfulness")
    ap.add_argument("--bootstrap", action="store_true")
    ap.add_argument("--skip-git", action="store_true")
    ap.add_argument("--docs-root", default="")
    ap.add_argument("--history-path", default="")
    a = ap.parse_args()
    repo = Path(a.repo).resolve()
    state_path = Path(a.state).resolve()
    try:
        state = json.loads(state_path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail("GATE-E01", f"state is not valid JSON: {exc}")
    schema = json.loads((repo/"build/state.schema.json").read_text(encoding="utf-8"))
    try:
        import jsonschema
        jsonschema.validate(state, schema)
    except Exception as exc:
        fail("GATE-E01", f"state schema invalid: {exc}")
    baseline = state["baseline"]
    if baseline["counts"] != {"PASS":7,"PARTIAL":30,"FAIL":11}:
        fail("GATE-E02", f"baseline counts mismatch: {baseline['counts']}")
    reports = {r["path"]: r["sha256"] for r in baseline["reports"]}
    for rel, expected in reports.items():
        p = repo/rel
        if not p.is_file(): fail("GATE-E02", f"baseline report missing: {rel}")
        if sha(p) != expected.upper(): fail("GATE-E02", f"baseline hash mismatch: {rel}")
    slices = state["slices"]
    if len(slices) != 48: fail("GATE-E02", f"expected 48 slices, got {len(slices)}")
    if set(slices) != set(PASS_IDS)|set(FAIL_IDS)|(set(slices)-set(PASS_IDS)-set(FAIL_IDS)):
        pass
    statuses = {k:v["status"] for k,v in slices.items()}
    for sid, st in statuses.items():
        expected = "PASS" if sid in PASS_IDS else "FAIL" if sid in FAIL_IDS else "PARTIAL"
        if st != expected:
            fail("GATE-E03", f"{sid}: status {st} contradicts baseline {expected}")
    if {k for k,v in statuses.items() if v=="PASS"} != PASS_IDS: fail("GATE-E02","PASS slice set mismatch")
    if {k for k,v in statuses.items() if v=="FAIL"} != FAIL_IDS: fail("GATE-E02","FAIL slice set mismatch")
    if {k for k,v in statuses.items() if v=="PARTIAL"} != set(slices)-PASS_IDS-FAIL_IDS: fail("GATE-E02","PARTIAL set mismatch")
    # checklist tokens and PASS audit requirements
    for sid, sl in slices.items():
        for key, val in sl.get("checklist",{}).items():
            if val["criterionStatus"] not in ALLOWED_CRITERION:
                fail("GATE-E03", f"{sid}/{key}: invalid criterionStatus {val['criterionStatus']}")
        if sl["status"]=="PASS":
            reports_ok = any(r["verdict"]=="PASS" for r in sl["baselineAudit"]["reports"])
            if not reports_ok:
                fail("GATE-E03", f"{sid}: PASS without PASS baseline audit")
    # traceability
    tr_path = repo/"build/TRACEABILITY.json"
    if not tr_path.is_file(): fail("GATE-E02","traceability missing")
    tr = json.loads(tr_path.read_text(encoding="utf-8"))
    if tr.get("rowCount") != 48 or len(tr.get("rows",[])) != 48: fail("GATE-E02","traceability must have 48 rows")
    if {r["sliceId"] for r in tr["rows"]} != set(slices): fail("GATE-E02","traceability ID set mismatch")
    # remediation
    rem = state.get("remediation",{})
    if set(rem) != {f"S{i}" for i in range(16)}: fail("GATE-E03","remediation must contain S0..S15")
    active = state["currentSlice"]
    s0 = rem.get("S0",{})
    if s0.get("implementationAudit"):
        aud = s0["implementationAudit"]
        if aud.get("verdict") != "PASS": fail("GATE-E04","S0 implementation audit is not PASS")
        if aud.get("subjectCommit") != s0.get("commit"): fail("GATE-E06","S0 audit subjectCommit != slice commit")
        for ev in s0.get("evidence",[]):
            p = repo/ev["path"]
            if not p.is_file(): fail("GATE-E05",f"missing evidence {ev['path']}")
            if p.stat().st_size == 0: fail("GATE-E05",f"empty evidence {ev['path']}")
            if sha(p) != ev["sha256"].upper(): fail("GATE-E05",f"evidence hash mismatch {ev['path']}")
    elif s0.get("state") not in ("IMPLEMENTING","AUDIT_PENDING") or not a.bootstrap:
        fail("GATE-E04","S0 audit missing and not in bootstrap state")
    if s0.get("blockers"):
        fail("GATE-E08","unresolved blockers in S0")
    if a.mode == "release":
        if state["overallStatus"] != "COMPLETE": fail("GATE-E06","release requires overallStatus COMPLETE")
        for sid, r in rem.items():
            if r.get("status") != "PASS": fail("GATE-E04",f"{sid}: remediation not PASS")
    # docs current claim check (first 30 lines only; historical allowed after)
    docs_root = Path(a.docs_root).resolve() if a.docs_root else repo
    for doc in DOCS:
        p = docs_root/doc
        if not p.is_file(): continue
        head = "\n".join(p.read_text(encoding="utf-8",errors="replace").splitlines()[:30])
        for pat in FORBIDDEN:
            if pat.lower() in head.lower(): fail("GATE-E09",f"{doc}: forbidden current claim '{pat}'")
    # history chain
    hist = Path(a.history_path).resolve() if a.history_path else repo/"build/audit/HISTORY.jsonl"
    if not hist.is_file(): fail("GATE-E10","HISTORY.jsonl missing")
    prev="GENESIS"
    for line in hist.read_text(encoding="utf-8").splitlines():
        if not line.strip(): continue
        e=json.loads(line)
        if e.get("prevHash")!=prev: fail("GATE-E10","HISTORY prevHash mismatch")
        body={k:v for k,v in e.items() if k!="hash"}
        h=hashlib.sha256((prev+json.dumps(body,separators=(",",":"))).encode()).hexdigest().upper()
        if e.get("hash")!=h: fail("GATE-E10","HISTORY hash mismatch")
        prev=e["hash"]
    if not a.skip_git:
        if git(repo,"status","--porcelain","--untracked-files=no"):
            fail("GATE-E07","tracked working tree is dirty")
        head=git(repo,"rev-parse","HEAD")
        src=state.get("sourceRevision")
        if src:
            r=subprocess.run(["git","-C",str(repo),"merge-base","--is-ancestor",src,head])
            if r.returncode!=0: fail("GATE-E06","sourceRevision is not an ancestor of HEAD")
        if s0.get("commit"):
            r=subprocess.run(["git","-C",str(repo),"merge-base","--is-ancestor",s0["commit"],head])
            if r.returncode!=0: fail("GATE-E06","S0 commit is not an ancestor of HEAD")
    if state["overallStatus"]=="PASS":
        fail("GATE-E03","top-level PASS is forbidden")
    print(json.dumps({"ok":True,"code":"GATE-PASS","mode":a.mode,"slices":len(slices),"active":active,"overall":state["overallStatus"]}))

if __name__=="__main__":
    main()
