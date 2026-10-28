#!/usr/bin/env python3
"""Generate and execute S0 audit-gate negative fixtures."""
from __future__ import annotations
import copy, hashlib, json, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GATE = ROOT/"tools/audit-validators/audit_state.py"
STATE = json.loads((ROOT/"EXECUTION-STATE.json").read_text(encoding="utf-8"))
FIX = ROOT/"build/audit/fixtures/S0"
LOG = ROOT/"build/evidence/S0/negative-fixtures.log"
LOG.parent.mkdir(parents=True, exist_ok=True)
results=[]

def write(name, data):
    p=FIX/name
    p.write_text(json.dumps(data,indent=2),encoding="utf-8")
    return p

def run(state_path, expected_code, *extra, expect_ok=False):
    cmd=[sys.executable,str(GATE),"--state",str(state_path),"--repo",str(ROOT),"--mode","truthfulness","--bootstrap","--skip-git",*extra]
    r=subprocess.run(cmd,capture_output=True,text=True)
    out=(r.stdout or r.stderr).strip()
    try: payload=json.loads(out)
    except Exception: payload={"code":"NO_JSON","message":out}
    ok=(payload.get("code")==expected_code) if not expect_ok else (r.returncode==0 and payload.get("ok") is True)
    results.append((state_path.name, expected_code, payload.get("code"), r.returncode, ok, payload.get("message","")))
    return ok

# positive control
write("positive.json", copy.deepcopy(STATE))
run(FIX/"positive.json","GATE-PASS",expect_ok=True)

# E01 schema: wrong schemaVersion
s=copy.deepcopy(STATE); s["schemaVersion"]=1; write("e01-schema.json",s); run(FIX/"e01-schema.json","GATE-E01")
# E02 baseline counts
s=copy.deepcopy(STATE); s["baseline"]["counts"]["PASS"]=6; write("e02-baseline.json",s); run(FIX/"e02-baseline.json","GATE-E02")
# E03 false PASS
s=copy.deepcopy(STATE); s["slices"]["F0.7"]["status"]="PASS"; write("e03-false-pass.json",s); run(FIX/"e03-false-pass.json","GATE-E02")
# E04 PASS without audit
s=copy.deepcopy(STATE); s["remediation"]["S0"]["state"]="PASS"; s["remediation"]["S0"]["status"]="PASS"; write("e04-audit-missing.json",s); run(FIX/"e04-audit-missing.json","GATE-E04")
# E05 missing evidence with a passed audit
s=copy.deepcopy(STATE); s["remediation"]["S0"]["commit"]="a50b2b6"; s["remediation"]["S0"]["implementationAudit"]={"path":"build/audit/S0-audit.md","sha256":"0"*64,"verdict":"PASS","subjectCommit":"a50b2b6","recordCommit":None}; s["remediation"]["S0"]["evidence"]=[{"path":"build/audit/does-not-exist.md","sha256":"0"*64,"kind":"report","exists":True}]; write("e05-missing-evidence.json",s); run(FIX/"e05-missing-evidence.json","GATE-E05")
# E06 audit commit mismatch
s=copy.deepcopy(STATE); s["remediation"]["S0"]["commit"]="a50b2b6"; s["remediation"]["S0"]["implementationAudit"]={"path":"build/audit/S0-audit.md","sha256":"0"*64,"verdict":"PASS","subjectCommit":"deadbeef","recordCommit":None}; write("e06-stale-audit.json",s); run(FIX/"e06-stale-audit.json","GATE-E04")
# E08 blocker
s=copy.deepcopy(STATE); s["remediation"]["S0"]["commit"]="a50b2b6"; s["remediation"]["S0"]["implementationAudit"]={"path":"build/audit/S0-audit.md","sha256":"0"*64,"verdict":"PASS","subjectCommit":"a50b2b6","recordCommit":None}; s["remediation"]["S0"]["blockers"]=[{"id":"B1","detail":"blocked"}]; write("e08-blocker.json",s); run(FIX/"e08-blocker.json","GATE-E08")
# empty evidence
empty=FIX/"empty.log"; empty.write_text("",encoding="utf-8")
s=copy.deepcopy(STATE); s["remediation"]["S0"]["commit"]="a50b2b6"; s["remediation"]["S0"]["implementationAudit"]={"path":"build/audit/S0-audit.md","sha256":"0"*64,"verdict":"PASS","subjectCommit":"a50b2b6","recordCommit":None}; s["remediation"]["S0"]["evidence"]=[{"path":"build/audit/fixtures/S0/empty.log","sha256":hashlib.sha256(b"").hexdigest().upper(),"kind":"log","exists":True}]; write("e05b-empty-evidence.json",s); run(FIX/"e05b-empty-evidence.json","GATE-E05")
# E09 docs forbidden claim
docs=FIX/"docs"; docs.mkdir(exist_ok=True); (docs/"STATE.md").write_text("STATUS: bad\n48/48 PASS\n",encoding="utf-8")
s=copy.deepcopy(STATE); write("e09-doc-claim.json",s); run(FIX/"e09-doc-claim.json","GATE-E09","--docs-root",str(docs))
# E10 broken history chain
hist=FIX/"bad-history.jsonl"; hist.write_text('{"seq":1,"prevHash":"GENESIS","hash":"BAD"}\n',encoding="utf-8")
s=copy.deepcopy(STATE); write("e10-history.json",s); run(FIX/"e10-history.json","GATE-E10","--history-path",str(hist))
# release rejection on truthful incomplete state
r=subprocess.run([sys.executable,str(GATE),"--state",str(FIX/"positive.json"),"--repo",str(ROOT),"--mode","release","--skip-git"],capture_output=True,text=True)
results.append(("release-incomplete","NONZERO",str(r.returncode),r.returncode,"PASS" if r.returncode!=0 else "FAIL","release gate must reject incomplete program"))
# dirty tracked tree: append to tracked STATE.md and restore
state_file=ROOT/"STATE.md"; original=state_file.read_text(encoding="utf-8")
try:
    state_file.write_text(original+"\n<!-- dirty test -->\n",encoding="utf-8")
    r=subprocess.run([sys.executable,str(GATE),"--state",str(FIX/"positive.json"),"--repo",str(ROOT),"--mode","truthfulness","--bootstrap"],capture_output=True,text=True)
    try: payload=json.loads((r.stdout or r.stderr).strip())
    except Exception: payload={"code":"NO_JSON"}
    results.append(("dirty-tracked","GATE-E07",payload.get("code"),r.returncode,payload.get("code")=="GATE-E07","dirty tree rejection"))
finally:
    state_file.write_text(original,encoding="utf-8")

lines=["# S0 audit gate negative fixtures",""]
for row in results:
    lines.append(f"| {row[0]} | expected {row[1]} | got {row[2]} | exit {row[3]} | {'PASS' if row[4] else 'FAIL'} | {row[5]} |")
LOG.write_text("\n".join(lines)+"\n",encoding="utf-8")
print("\n".join(lines))
if not all(r[4] for r in results): raise SystemExit(1)
