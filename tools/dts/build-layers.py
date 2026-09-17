import sys, os, re, glob, json, shutil
from collections import defaultdict
S=os.environ.get('AZ0X_DTS_WORK', 'dts-work')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dtsgen import parse, write_tree
from dtscmp import node_equal

OUT='re-work/dts/inferred'

# --- hand-written per-board overlays -----------------------------------------
# A board may need overrides the vendor template cannot express, e.g. what
# u-boot rewrites at boot. Those live in dts/fitted/<fam>-<board>-fitted.dtsi
# and are included LAST so they win.
#
# This used to be a manual step documented in the README, and it silently broke:
# the re-add anchored on a feature include whose name changed when the corpus
# grew, the string replace matched nothing, and a build shipped without the
# overrides. Generating the include removes the chance to forget.
FITTED_DIR = 're-work/dts/fitted'

def fitted_for(fam, board):
    p = os.path.join(FITTED_DIR, f'{fam}-{board}-fitted.dtsi')
    return p if os.path.exists(p) else None

os.makedirs(OUT, exist_ok=True)

# True mainline rk3288.dtsi node set (NOT evb n firefly -- that wrongly captured
# board-level nodes like /external-gmac-clock that both boards happen to share).
SOC_N, SOC_P, _ = parse(S+'/soconly.dts.out')
SOC = {p for p in SOC_N if p not in ('/','/chosen','/aliases','/memory@0')}

def base_of(b):
    return re.sub(r'-c(?=$|-rev)', '', b, count=1)

def build(fam, socinc):
    files = sorted(glob.glob(f're-work/dts/raw/{fam}-*.dts'))
    pre = len(fam)+1
    short = lambda f: os.path.basename(f)[pre:-4]
    P = {short(f): parse(f) for f in files}
    boards = sorted(P)
    if len(boards) < 2:
        return P, boards, {}, {}
    allp = set()
    for b in boards: allp |= set(P[b][0])

    common, varying, partial = [], [], []
    for p in sorted(allp):
        own = [b for b in boards if p in P[b][0]]
        if len(own) != len(boards): partial.append(p); continue
        (common if all(node_equal(P[own[0]][0][p], P[b][0][p], P[own[0]][1], P[b][1])
                       for b in own[1:]) else varying).append(p)

    REF = boards[0]
    n, ph, _ = P[REF]
    from_ml, changed, new = [], [], []
    for p in common:
        if p in SOC:
            (from_ml if node_equal(n[p], SOC_N[p], ph, SOC_P) else changed).append(p)
        else: new.append(p)

    delta = {}
    for p in changed:
        b = dict((k,v) for k,v in SOC_N[p] if k not in ('phandle','linux,phandle'))
        keep = [(k,v) for k,v in n[p] if k not in ('phandle','linux,phandle')
                and (k not in b or not node_equal([(k,v)],[(k,b[k])], ph, SOC_P))]
        if keep: delta[p] = keep
    dn = dict(n); dn.update(delta)

    with open(f'{OUT}/{fam}.dtsi','w') as fh:
        fh.write(f'''// SPDX-License-Identifier: GPL-2.0-or-later OR MIT
/*
 * inMusic {fam.split('-')[1].upper()} common platform description.
 *
 * Inferred from {len(boards)} vendor device trees: this file holds only what is
 * identical on all {len(boards)} boards -- {len(common)} nodes in total.
 *
 * Self-contained by design.  Including mainline {socinc} is NOT correct here:
 * the vendor tree descends from a downstream Rockchip dtsi that names the same
 * hardware differently (dwmmc@ff0c0000 vs mmc@ff0c0000, mipi@ff960000 vs
 * dsi@ff960000), so an #include duplicates nodes instead of overriding them.
 * For reference, {len(from_ml)} of these {len(common)} nodes ARE byte-identical to
 * mainline {socinc}, and {len(changed)} more differ only in retuned properties.
 *
 * Phandle references use dtc path syntax (&{{/foo}}) because the decompiled
 * source carries no labels.
 */

''')
        write_tree(fh, common, n, ph, '')

    # feature dtsis, keyed by owner-set; names made UNIQUE (the earlier version
    # silently overwrote one group with another that auto-named identically)
    groups, extra = defaultdict(list), defaultdict(list)
    for p in partial:
        own = [b for b in boards if p in P[b][0]]
        if len(own) > 1 and all(node_equal(P[own[0]][0][p], P[b][0][p], P[own[0]][1], P[b][1])
                                for b in own[1:]):
            groups[frozenset(own)].append(p)
        else:
            for b in own: extra[b].append(p)

    HINT = [('/cpu-fan','fan-pwm'), ('/sound','sound'), ('/i2c@ff160000/ili2116@41','touch-ili2116'),
            ('/i2c@ff160000/ili2110@41','touch-ili2110'), ('/i2c@ff140000/usb5537@2d','sata-usb-hub'),
            ('/flash-regulator','regulators-jp'), ('/regulator-flash','regulators-acv'),
            ('/pinctrl/sound-codec/mute','sound-codec-pins'),
            ('/opp-table-0/opp-1800000000','opp-1800'), ('/opp-table-0/opp-1704000000','opp-1704')]
    feat, used = {}, set()
    for own, paths in sorted(groups.items(), key=lambda kv: (-len(kv[1]), sorted(kv[0]))):
        nm = None
        for key, label in HINT:
            if key in paths: nm = label; break
        if nm is None:
            if len(paths) < 2:
                for b in own: extra[b].extend(paths)
                continue
            nm = 'opt-' + sorted(own)[0]
        base = nm; i = 2
        while nm in used: nm = f'{base}-{i}'; i += 1   # guarantee uniqueness
        used.add(nm); feat[nm] = (own, paths)
        src = sorted(own)[0]; sn, sp, _ = P[src]
        with open(f'{OUT}/{fam}-{nm}.dtsi','w') as fh:
            write_tree(fh, paths, sn, sp,
              f'// SPDX-License-Identifier: GPL-2.0-or-later OR MIT\n/*\n * {fam} option: {nm}\n *\n'
              f' * Present on {len(own)} of {len(boards)} boards:\n *   {", ".join(sorted(own))}\n */\n\n')

    # per-board; -c speed-bin variants become thin deltas over a shared base dtsi
    board_nodes = {}
    for b in boards:
        n, ph, _ = P[b]
        board_nodes[b] = [p for p in varying if p in n] + sorted(set(extra[b]))

    for b in boards:
        n, ph, _ = P[b]
        incs = [nm for nm in sorted(feat) if b in feat[nm][0]]
        bs = base_of(b)
        is_c = (bs != b and bs in boards)
        if is_c:
            shared = [p for p in board_nodes[bs] if p in board_nodes[b]
                      and node_equal(P[bs][0][p], n[p], P[bs][1], ph)]
            mine = [p for p in board_nodes[b] if p not in shared]
            with open(f'{OUT}/{fam}-{b}.dts','w') as fh:
                fh.write('// SPDX-License-Identifier: GPL-2.0-or-later OR MIT\n/dts-v1/;\n\n')
                fh.write(f'/* RK3288-C speed bin of {bs}: {len(mine)} nodes differ. */\n')
                fh.write(f'#include "{fam}-{bs}-base.dtsi"\n')
                for i in incs: fh.write(f'#include "{fam}-{i}.dtsi"\n')
                fh.write('\n')
                write_tree(fh, mine, n, ph, '')
        else:
            with open(f'{OUT}/{fam}-{b}-base.dtsi','w') as fh:
                write_tree(fh, board_nodes[b], n, ph,
                    f'// SPDX-License-Identifier: GPL-2.0-or-later OR MIT\n'
                    f'/*\n * {fam}-{b} board nodes.\n */\n\n#include "{fam}.dtsi"\n\n')
            with open(f'{OUT}/{fam}-{b}.dts','w') as fh:
                fh.write('// SPDX-License-Identifier: GPL-2.0-or-later OR MIT\n/dts-v1/;\n\n')
                fh.write(f'#include "{fam}-{b}-base.dtsi"\n')
                for i in incs: fh.write(f'#include "{fam}-{i}.dtsi"\n')
                fp = fitted_for(fam, b)
                if fp:
                    shutil.copy(fp, f'{OUT}/{fam}-{b}-fitted.dtsi')
                    fh.write('\n/* last: overrides status/compatible set above */\n')
                    fh.write(f'#include "{fam}-{b}-fitted.dtsi"\n')
    # -c files need the base board's option includes too
    return P, boards, feat, {'from_ml':len(from_ml),'changed':len(changed),'new':len(new),
                             'common':len(common),'varying':len(varying),'partial':len(partial)}

res = {}
for fam, inc in (('rk3288-az01','rk3288.dtsi'), ('rk3288-az05','rk3288.dtsi')):
    P, boards, feat, stats = build(fam, inc)
    res[fam] = {'boards': len(boards), 'features': sorted(feat), 'stats': stats}
    print(f"{fam}: {len(boards)} boards  {stats}")
    print(f"   features({len(feat)}): {', '.join(sorted(feat))}")
json.dump(res, open(S+'/layers2.json','w'), indent=1)
