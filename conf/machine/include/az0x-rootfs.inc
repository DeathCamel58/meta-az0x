# Root filesystem geometry for every AZ0x machine.
#
# A build replaces the vendor's rootfs partition in place, so whatever image is
# built for these machines must be exactly that partition: the vendor's block
# size, inode ratio and block count, read out with dumpe2fs. Each machine sets
# AZ0X_ROOTFS_KIB (and, where they differ from the az01 values,
# AZ0X_EXT4_BLOCKSIZE / AZ0X_EXT4_INODE_RATIO):
#
#   az01-premium (jp13/jp14)                        1024 x 504276   ~492 MiB
#   az01-player  (jp07/08/11)                       1024 x 504076   ~492 MiB
#   az01-console (jc11/16/nh08)                     1024 x 503936   ~492 MiB
#   az01-mpc     (acv*)                             1024 x 430936   ~421 MiB
#   az0x-signed  (JC11S/JP11S/JP20/JP21/JP21X/JP23) 1024 x 528518
#   az04         (rmz2)                             4096 x 205995   ~805 MiB
#
# Both vendor filesystems are journal-less (flash wear) and reserve no blocks
# for root. An image that outgrows the partition fails at do_image_ext4 rather
# than producing something unflashable.

IMAGE_FSTYPES = "ext4"
IMAGE_CLASSES:remove = "image_types_wic"

AZ0X_ROOTFS_KIB ?= "0"
AZ0X_EXT4_BLOCKSIZE ?= "1024"
AZ0X_EXT4_INODE_RATIO ?= "4096"

# The filesystem is IMAGE_ROOTFS_SIZE plus IMAGE_ROOTFS_EXTRA_SPACE, and an
# image may add extra space of its own (core-image-minimal adds 4096 KiB when
# systemd is on). Take it off the size, so whatever the image asks for, the
# filesystem comes out exactly the partition. image.bbclass evaluates
# IMAGE_ROOTFS_EXTRA_SPACE as an expression the same way.
IMAGE_ROOTFS_EXTRA_SPACE = "0"
IMAGE_ROOTFS_SIZE = "${@int(d.getVar('AZ0X_ROOTFS_KIB')) - int(eval(d.getVar('IMAGE_ROOTFS_EXTRA_SPACE') or '0'))}"
IMAGE_ROOTFS_MAXSIZE = "${AZ0X_ROOTFS_KIB}"
IMAGE_OVERHEAD_FACTOR = "1.0"

EXTRA_IMAGECMD:ext4 = "-b ${AZ0X_EXT4_BLOCKSIZE} -O ^has_journal -m 0 -i ${AZ0X_EXT4_INODE_RATIO}"
