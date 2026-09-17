#!/usr/bin/env python3
"""Strict cell-level audit: generated DTB vs vendor DTB.

The phandle-aware round trip in dtscmp.py has a blind spot. It calls two cells
equal when both are phandles resolving to the same path -- so if a property is
MISCLASSIFIED as phandle-bearing, the wrong cells get symbolised on our side,
dtc renumbers them, and the comparison still passes because the same wrong rule
is applied to both sides.

This audit closes that hole from the other direction: every cell that DIFFERS
numerically from the vendor's must sit at a position we deliberately intended
to symbolise. A differing cell anywhere else is a bug.

It is what would have caught assigned-clock-parents being treated as a bare
phandle, which shifted every clock index by one and reparented both VOP pixel
clocks.
"""
import re, sys, os, glob
from collections import defaultdict
S = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, S)
from dtsgen import parse, symbolise

HEX = re.compile(r'0x[0-9a-fA-F]+')

def sym_positions(k, v, nodes, ph):
    """Indices of the hex cells symbolise() would replace."""
    if v is None or not v.startswith('<'):
        return set()
    out = symbolise(k, v, nodes, ph)
    before, after = HEX.findall(v), re.findall(r'&\{[^}]*\}|0x[0-9a-fA-F]+', out)
    pos, i = set(), 0
    for tok in after:
        if tok.startswith('&'):
            pos.add(i)
        i += 1
    return pos if len(after) == len(before) else set(range(len(before)))

bad = defaultdict(list)
checked = 0
for g in sorted(glob.glob(S + '/rt/*.gen.dts')):
    b = os.path.basename(g)[:-8]
    vend = f'{S}/vend/{b}.norm.dts'
    if not os.path.exists(vend):
        continue
    vn, vp, _ = parse(vend)
    gn, gp, _ = parse(g)
    for path in vn:
        if path not in gn:
            continue
        vprops = {k: val for k, val in vn[path] if k not in ('phandle', 'linux,phandle')}
        gprops = {k: val for k, val in gn[path] if k not in ('phandle', 'linux,phandle')}
        for k, vv in vprops.items():
            gv = gprops.get(k)
            if gv is None or vv is None or not str(vv).startswith('<'):
                continue
            vc, gc = HEX.findall(vv), HEX.findall(gv)
            if len(vc) != len(gc):
                bad[(k, 'cell-count')].append(f'{b}{path}')
                continue
            checked += 1
            allowed = sym_positions(k, vv, vn, vp)
            differing = {i for i, (a, c) in enumerate(zip(vc, gc)) if int(a, 16) != int(c, 16)}
            stray = differing - allowed
            if stray:
                bad[(k, 'unexpected-diff')].append(f'{b}{path} cells {sorted(stray)}')

print(f"  audited {checked} numeric properties across the corpus")
if not bad:
    print("  CLEAN: every differing cell sits at a position we meant to symbolise")
else:
    print(f"  *** {len(bad)} property/kind combinations with unexplained differences ***")
    for (k, kind), where in sorted(bad.items(), key=lambda kv: -len(kv[1])):
        print(f"    {k}  ({kind})  x{len(where)}")
        for w in where[:3]:
            print(f"        {w}")
