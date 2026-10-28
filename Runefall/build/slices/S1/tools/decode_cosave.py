#!/usr/bin/env python3
"""Standalone decoder for S1 co-save fixtures. Read-only."""
import struct,sys,json
from pathlib import Path
def parse(b):
    if b[:4]!=b'RNFS': raise ValueError('magic')
    schema=struct.unpack_from('<H',b,4)[0]
    if schema==3:
        flags=struct.unpack_from('<I',b,6)[0]; version=struct.unpack_from('<H',b,10)[0]; count=struct.unpack_from('<I',b,12)[0]; off=16
    else:
        version=struct.unpack_from('<H',b,6)[0]; count=struct.unpack_from('<I',b,8)[0]; off=12; flags=None
    secs=[]
    for _ in range(count):
        sid=struct.unpack_from('<H',b,off)[0]; ln=struct.unpack_from('<I',b,off+2)[0]; off+=6
        secs.append((sid,b[off:off+ln])); off+=ln
    return {'schema':schema,'flags':flags,'saveVersion':version,'sectionCount':count,'sections':[(s,len(p)) for s,p in secs],'trailing':len(b)-off}
if __name__=='__main__':
    for name in sys.argv[1:]:
        b=Path(name).read_bytes(); print(name,json.dumps(parse(b)))
