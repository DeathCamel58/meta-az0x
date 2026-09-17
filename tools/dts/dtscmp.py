#!/usr/bin/env python3
"""Compare decompiled DTS node-by-node, sound under phandle renumbering.

Two cells are equal iff their numeric values are equal, OR both are valid
phandles in their own file and resolve to the same node path.  That avoids the
false "varying" my earlier script produced: it used to rewrite any hex token
that happened to equal some phandle value, so clock *indices* like 0xc7 got
symbolised in one file and not the other.
"""
import os, re, sys, json, glob, os
from collections import defaultdict

HEX = re.compile(r'0x[0-9a-fA-F]+')

def parse(path):
    """-> (nodes, phandle_map)  nodes: path -> [(propname, value_text), ...]"""
    nodes, ph = {}, {}
    stack = []
    props = defaultdict(list)
    for line in open(path, encoding='utf-8', errors='replace'):
        s = line.strip()
        if not s or s.startswith('/dts-v1/') or s.startswith('//') or s.startswith('/memreserve/'):
            continue
        if s.endswith('{'):
            name = s[:-1].strip()
            stack.append(name)
            continue
        if s == '};':
            if stack: stack.pop()
            continue
        m = re.match(r'^([\w@,.#+-]+)\s*=\s*(.*);$', s)
        if m:
            k, v = m.group(1), m.group(2).strip()
            p = '/' + '/'.join(stack[1:])
            props[p].append((k, v))
            if k == 'phandle':
                try: ph[int(v.strip('<> '), 16)] = p
                except ValueError: pass
            continue
        m = re.match(r'^([\w@,.#+-]+);$', s)   # boolean property
        if m:
            p = '/' + '/'.join(stack[1:])
            props[p].append((m.group(1), None))
    return dict(props), ph

def cells_equal(va, vb, pha, phb):
    """Compare two property value strings under the phandle rule."""
    if va is None or vb is None:
        return va == vb
    ta, tb = HEX.findall(va), HEX.findall(vb)
    if HEX.sub('#', va) != HEX.sub('#', vb):   # non-numeric skeleton must match
        return False
    if len(ta) != len(tb):
        return False
    for a, b in zip(ta, tb):
        na, nb = int(a, 16), int(b, 16)
        if na == nb:
            continue
        pa, pb = pha.get(na), phb.get(nb)
        if pa is not None and pa == pb:
            continue
        return False
    return True

def node_equal(pa_list, pb_list, pha, phb):
    a = [(k, v) for k, v in pa_list if k != 'phandle' and k != 'linux,phandle']
    b = [(k, v) for k, v in pb_list if k != 'phandle' and k != 'linux,phandle']
    if len(a) != len(b): return False
    for (ka, va), (kb, vb) in zip(sorted(a), sorted(b)):
        if ka != kb: return False
        if not cells_equal(va, vb, pha, phb): return False
    return True

if __name__ == '__main__':
    files = sorted(sys.argv[1:])
    parsed = {f: parse(f) for f in files}
    ref = files[0]
    rn, rph = parsed[ref]

    common, varying, partial = [], [], []
    allpaths = set()
    for f in files:
        allpaths |= set(parsed[f][0])

    for p in sorted(allpaths):
        present = [f for f in files if p in parsed[f][0]]
        if len(present) != len(files):
            partial.append((p, len(present)))
            continue
        r = present[0]
        ok = all(node_equal(parsed[r][0][p], parsed[f][0][p], parsed[r][1], parsed[f][1])
                 for f in present[1:])
        (common if ok else varying).append(p)

    print(f"files={len(files)}  node paths total={len(allpaths)}")
    print(f"  identical in ALL files : {len(common)}")
    print(f"  present in all, differ : {len(varying)}")
    print(f"  present in only some   : {len(partial)}")
    json.dump({'common': common, 'varying': varying, 'partial': partial},
              open('/tmp/claude-1000/-media-deathcamel57-2tb-nvme-dev-yocto-inMusic/390afc2c-c130-4547-b8ba-bfc605f0519c/scratchpad/cmp.json', 'w'), indent=1)
