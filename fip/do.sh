#!/bin/sh

set -x

cp u-boot.bin fip/bl33.bin

fip/fip_create --bl30 fip/bl30.bin --bl301 fip/bl301.bin --bl31 fip/bl31.bin --bl33 fip/bl33.bin fip/fip.bin
fip/fip_create --dump fip/fip.bin
cat fip/bl2.package  fip/fip.bin > fip/boot_new.bin
fip/aml_encrypt_gxb --bootsig --input fip/boot_new.bin --output fip/u-boot.bin
dd if=fip/u-boot.bin of=sd_fuse/u-boot.bin bs=512 skip=96

