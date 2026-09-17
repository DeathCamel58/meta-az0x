# Regenerating the AZ01/AZ05 device trees

The device trees shipped by
`recipes-kernel/linux/az01-kernel-files/0001-ARM-dts-rockchip-add-inMusic-AZ01-AZ05-device-trees.patch`
are generated, not hand-written. This is how to reproduce them.

## Why they are generated

The vendor ships 33 compiled DTBs and no source. Decompiling gives 33
near-identical trees totalling ~97k lines, which is unmaintainable: a fix to a
shared block would have to be repeated 27 times. These scripts decompile them
and factor out what is genuinely shared, producing a layered tree under 20k
lines that rebuilds to semantically identical DTBs.

## Inputs

The vendor DTBs, as extracted from a stock firmware image:

    vendor/<board-dir>/boot/*.dtb

## Procedure

    # 1. decompile every vendor DTB
    mkdir -p dts-work/raw
    for f in vendor/*/boot/*.dtb; do
        dtc -I dtb -O dts -o dts-work/raw/$(basename $f .dtb).dts $f
    done

    # 2. a mainline rk3288.dtsi baseline, used only to report how much of the
    #    vendor tree matches upstream (the output does not include it -- see below)
    cat > dts-work/soconly.dts <<'DTS'
    /dts-v1/;
    #include "rk3288.dtsi"
    / { model="soc-only"; compatible="rockchip,rk3288";
        memory@0 { device_type="memory"; reg=<0 0x1000>; }; };
    DTS
    K=<kernel source>/arch/arm/boot/dts/rockchip
    cpp -nostdinc -I<kernel source>/include -I$K -undef -D__DTS__ \
        -x assembler-with-cpp dts-work/soconly.dts > dts-work/soconly.pre.dts
    dtc -I dts -O dtb -o dts-work/soconly.dtb dts-work/soconly.pre.dts
    dtc -I dtb -O dts -o dts-work/soconly.dts.out dts-work/soconly.dtb

    # 3. factor
    AZ0X_DTS_WORK=dts-work python3 tools/dts/build-layers.py

Output lands in `dts-work/inferred/`. Turn it into the kernel patch by copying
the `rk3288-az0[15]*` files into `arch/arm/boot/dts/rockchip/`, adding one
`dtb-$(CONFIG_ARCH_ROCKCHIP) +=` line per board `.dts`, and running
`git format-patch`.

## Verification -- do not skip this

Every generated tree must rebuild to a DTB semantically identical to the
vendor's. Compile each one, decompile it again, and compare node-by-node
against the decompiled vendor DTB. The comparison in `dtscmp.py` is
phandle-aware: two cells are equal iff their numeric values match **or** both
are valid phandles resolving to the same node path. Without that, dtc's
renumbering hides real differences.

The expected result is total:

    33 device trees / 14040 nodes -- missing=0  differing=0  extra=0

### The blind spot, and how it bit

The phandle-aware comparison has a hole worth understanding, because it let a
real bug through. It calls two cells equal when both are phandles resolving to
the same path. If a property is MISCLASSIFIED as phandle-bearing, the wrong
cells get symbolised on our side, dtc renumbers them, and the comparison still
passes -- because the same wrong rule is applied to both sides.

That is exactly what happened to `assigned-clock-parents`. It is a phandle+args
list governed by the target's `#clock-cells`, but it was listed as a bare
phandle. Every clock INDEX cell was therefore symbolised, dtc renumbered them,
and each index shifted by one:

    vendor  assigned-clock-parents = <&cru 4> <&cru 5>   (GPLL, NPLL)
    ours    assigned-clock-parents = <&cru 5> <&cru 6>   (NPLL, ARMCLK)

which silently reparented both VOP pixel clocks. The kernel said so -
`clk: failed to reparent dclk_vop1 to armclk: -22`, a line the vendor kernel
never prints - and the visible symptom was a barely perceptible flicker.

`audit.py` catches cells changed UNINTENTIONALLY, which is a different and
also useful class. It cannot catch misclassification, for the same circular
reason: misclassification changes what counts as intended.

**The check that does catch it** is to list every property NAME whose raw cells
differ from the vendor's, and confirm each one is genuinely phandle-bearing. A
plain-data property showing up in that list -- `reg`, `interrupts`,
`brightness-levels`, `opp-hz` -- means a cell is being rewritten that should
never be touched. Run it whenever the classification in `dtsgen.py` changes.

This check is not ceremonial. It caught four real bugs that a single-board
spot-check passed: `rockchip,pins` carries a phandle in every 4th cell and was
being emitted as a raw integer that dtc then renumbered, silently repointing
every pin at the wrong pull config; root node properties were dropped; option
dtsis leaked the root `compatible` of whichever board they were sampled from;
and empty leaf nodes were skipped.

## Files

* `dtsgen.py` -- parser and emitter. Holds the phandle-property classification
  (single phandles, `#*-cells`-counted lists, and strided ones like
  `rockchip,pins`). An unclassified phandle property is a silent corruption
  bug, so add new ones here and re-run the verification.
* `dtscmp.py` -- the phandle-aware comparison.
* `build-layers.py` -- the factoring driver.

## Known non-goals

* **Not byte-identical to the vendor DTBs.** Nodes are emitted in sorted rather
  than original order, which shifts phandle numbering and the string table.
  `/aliases` pins mmc, i2c, serial and spi numbering, so ordering has no
  functional meaning here.
* **Does not `#include "rk3288.dtsi"`.** The vendor tree is a long-lived fork
  whose node names froze at mainline <= v5.5 (`dwmmc@` not `mmc@`, `mipi@` not
  `dsi@`) while its content tracks 6.6, so including mainline duplicates nodes
  instead of overriding them. For reference, 207 of the 342 nodes shared by all
  27 AZ01 boards are byte-identical to mainline v6.6 `rk3288.dtsi` and 34 more
  differ only in retuned properties -- worth revisiting if the boards are ever
  ported to mainline node names.
