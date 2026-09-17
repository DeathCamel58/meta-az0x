#!/usr/bin/env python3
"""Emit a layered DTS/DTSI tree from decompiled vendor DTBs.

Phandle cells are rewritten to dtc path references (&{/foo/bar}) so the output
compiles with no synthesised labels.  A cell is only treated as a phandle when
the property is phandle-typed AND the cell sits at an argument boundary derived
from the target's #*-cells -- never by "this number matches some phandle".
"""
import re, os, sys, glob, json
from collections import defaultdict

HEX = re.compile(r'0x[0-9a-fA-F]+')

# property -> the #*-cells property naming how many args follow each phandle
CELLED = {
    'clocks': '#clock-cells', 'resets': '#reset-cells', 'iommus': '#iommu-cells',
    'phys': '#phy-cells', 'dmas': '#dma-cells', 'pwms': '#pwm-cells',
    'io-channels': '#io-channel-cells', 'sound-dai': '#sound-dai-cells',
    'power-domains': '#power-domain-cells', 'mboxes': '#mbox-cells',
    'interrupts-extended': '#interrupt-cells', 'thermal-sensors': '#thermal-sensor-cells',
    'cooling-device': '#cooling-cells', 'nvmem-cells': None, 'gpios': '#gpio-cells',
}
# properties holding exactly one bare phandle
SIMPLE = {
    'interrupt-parent', 'remote-endpoint', 'memory-region', 'operating-points-v2',
    'next-level-cache', 'cpu', 'rockchip,grf', 'rockchip,pmu', 'rockchip,pmugrf',
    'rockchip,hdmi-phy',
    # found by round-tripping all 31 boards: each of these silently repointed
    # because the decompiler leaves them as bare phandle integers
    'ddc-i2c-bus', 'phy-handle', 'mmc-pwrseq', 'backlight', 'panel', 'target',
    'trip', 'shmem', 'peer-hub', 'snps,axi-config', 'snps,mtl-rx-config',
    'snps,mtl-tx-config', 'gpio1830',
    'rockchip,vo-grf', 'rockchip,vo1-grf', 'rockchip,pipe-grf', 'rockchip,php-grf',
    'rockchip,u2phy-grf', 'rockchip,usb-grf', 'rockchip,usbdpphy-grf',
    'rockchip,pipe-phy-grf', 'rockchip,vop-grf', 'rockchip,phy-grf',
    # inMusic vendor audio bindings
    'inmusic,cpu', 'inmusic,codec', 'inmusic,master-codec', 'inmusic,speaker-codec',
}
# every cell is a bare phandle
PHANDLE_LIST = {'pm_qos', 'power-supplies', 'cpu-idle-states', 'interrupt-affinity',
                'affinity', 'ports'}
STRIDED = {
    'rockchip,pins': (4, [3]),        # <bank pin func &pcfg_xxx>
    'gpio-ranges':   (4, [0]),        # <&pinctrl gpio_off pin_off count>
}

def is_celled(k):
    if k in CELLED: return CELLED[k]
    if k.endswith('-gpios') or k.endswith('-gpio'): return '#gpio-cells'
    if k.startswith('pinctrl-') and k[8:].isdigit(): return '__PINCTRL__'
    # assigned-clock-parents is a phandle+args list governed by the TARGET's
    # #clock-cells, NOT a bare phandle. Getting this wrong symbolises the clock
    # INDEX cells as phandles, dtc renumbers them, and every index shifts -
    # which silently reparented both VOP pixel clocks on this board.
    if k in ('assigned-clocks', 'assigned-clock-parents', 'clock-parent'):
        return '#clock-cells'
    if k == 'gpio': return '#gpio-cells'
    return False
def is_simple(k):
    return k in SIMPLE or k in PHANDLE_LIST or k.endswith('-supply')

def parse(path):
    nodes, ph, order = {}, {}, []
    stack, props = [], defaultdict(list)
    for line in open(path, encoding='utf-8', errors='replace'):
        s = line.strip()
        if not s or s.startswith(('/dts-v1/', '//', '/memreserve/')): continue
        if s.endswith('{'):
            stack.append(s[:-1].strip())
            p = '/' + '/'.join(stack[1:])
            if p not in props: props[p] = []; order.append(p)
            continue
        if s == '};':
            if stack: stack.pop()
            continue
        p = '/' + '/'.join(stack[1:])
        m = re.match(r'^([\w@,.#+-]+)\s*=\s*(.*);$', s)
        if m:
            k, v = m.group(1), m.group(2).strip()
            # STRING_LIST_NORM: dtc renders a string list as "a", "b" while our
            # decompiles carry "a\0b". Same bytes; normalise so both sources
            # compare and emit identically.
            if v.startswith('"') and '", "' in v:
                v = v.replace('", "', '\\0')
            props[p].append((k, v))
            if k in ('phandle', 'linux,phandle'):
                try: ph[int(v.strip('<> '), 16)] = p
                except ValueError: pass
            continue
        m = re.match(r'^([\w@,.#+-]+);$', s)
        if m: props[p].append((m.group(1), None))
    return dict(props), ph, order

def cells_of(nodes, path, key):
    for k, v in nodes.get(path, []):
        if k == key:
            try: return int(v.strip('<> '), 16)
            except Exception: return 0
    return 0

def symbolise(k, v, nodes, ph):
    """Rewrite phandle cells in value v to &{/path}."""
    if v is None or not v.startswith('<'): return v
    if k in STRIDED:
        stride, idxs = STRIDED[k]
        def do_str(m):
            toks = m.group(1).split()
            out = []
            for i, t in enumerate(toks):
                if i % stride in idxs and t.startswith('0x'):
                    tgt = ph.get(int(t, 16))
                    out.append('&{%s}' % tgt if tgt else t)
                else:
                    out.append(t)
            return '<' + ' '.join(out) + '>'
        return re.sub(r'<([^>]*)>', do_str, v)
    ck = is_celled(k)
    if not ck and not is_simple(k): return v
    def do_group(m):
        body = m.group(1)
        toks = body.split()
        out, i = [], 0
        while i < len(toks):
            t = toks[i]
            if not t.startswith('0x'): out.append(t); i += 1; continue
            n = int(t, 16)
            tgt = ph.get(n)
            if tgt is None: out.append(t); i += 1; continue
            out.append('&{%s}' % tgt)
            i += 1
            if ck == '__PINCTRL__' or ck is None or is_simple(k):
                nargs = 0
            else:
                nargs = cells_of(nodes, tgt, ck)
            for _ in range(nargs):
                if i < len(toks): out.append(toks[i]); i += 1
        return '<' + ' '.join(out) + '>'
    return re.sub(r'<([^>]*)>', do_group, v)

def emit(path, props, nodes, ph, indent=1):
    """Emit one node as nested /{...} overlay form."""
    t = '\t' * indent
    lines = []
    for k, v in props:
        if k in ('phandle', 'linux,phandle'): continue
        if v is None: lines.append(f'{t}{k};')
        else: lines.append(f'{t}{k} = {symbolise(k,v,nodes,ph)};')
    return lines

def write_tree(fh, paths, nodes, ph, header):
    """Write a set of node paths as a properly nested / { } tree."""
    fh.write(header)
    tree = {}
    for p in sorted(paths):
        cur = tree
        for seg in [s for s in p.split('/') if s]:
            cur = cur.setdefault(seg, {})
        cur['__self__'] = p
    def rec(d, depth):
        for seg in sorted(k for k in d if k != '__self__'):
            sub = d[seg]
            p = sub.get('__self__')
            body = emit(p, nodes.get(p, []), nodes, ph, depth + 1) if p else []
            kids = [k for k in sub if k != '__self__']
            # a real node with neither properties nor children still has to be
            # emitted (empty leaves exist, e.g. rk3588 pinctrl stubs)
            if p is None and not kids: continue
            fh.write('\t' * depth + seg + ' {\n')
            for l in body: fh.write(l + '\n')
            if body and kids: fh.write('\n')
            rec(sub, depth + 1)
            fh.write('\t' * depth + '};\n')
    fh.write('/ {\n')
    # only emit root properties when '/' is genuinely one of the requested
    # paths -- otherwise a feature dtsi leaks the root model/compatible of
    # whichever board it happened to be sampled from
    if '/' in set(paths):
        for l in emit('/', nodes.get('/', []), nodes, ph, 1): fh.write(l + '\n')
        if nodes.get('/'): fh.write('\n')
    rec(tree, 1)
    fh.write('};\n')
