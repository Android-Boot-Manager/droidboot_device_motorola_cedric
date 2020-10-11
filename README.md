# lk2nd for Android Boot Manager
lk2nd is a bootloader for Qualcomm MSM devices,
based on the [CodeAurora Little Kernel fork](https://source.codeaurora.org/quic/la/kernel/lk/).
It provides an Android dualboot menu.

lk2nd does not replace the stock bootloader. It is packaged into an Android
boot image and then loaded by the stock bootloader as a "secondary" bootloader.
The real Android boot images are placed into the unused OEM partition.

Important: This fork is used by Android Boot Manager and has code specific to it. It will not work without. Take a look at https://github.com/Junak/lk2nd

------
Be sure to check out our [wiki](https://github.com/Android-Boot-Manager/App/wiki)!

------
### Supported devices
- Moto G5 - cedric
- Samsung Galaxy Avant (Core LTE)

## Installation
Install using our Android App. (NOT YET FOR SAMSUNG GALAXY AVANT OR CORE LTE!!)

## Building
```
$ make TOOLCHAIN_PREFIX=arm-none-eabi- <SoC>-secondary
```

**Requirements:**
- ARM (32 bit) GCC tool chain
  - Arch Linux: `arm-none-eabi-gcc`
- [Device Tree Compiler](https://git.kernel.org/pub/scm/utils/dtc/dtc.git)
  - Arch Linux: `dtc`

Replace `TOOLCHAIN_PREFIX` with the path to your tool chain.
`lk2nd.img` is built and placed into `build-<SoC>-secondary/lk2nd.img`.


## WolfLink115's tutorial for Samsung Galaxy Avant (Core LTE)

**Until there is an installer for this device, here are some temporary instructions to get this device up and running.**

1) Download device-tree-compiler and alien using sudo. (sudo apt install -y device-tree-compiler alien)
2) Download heimdall v1.4.2 from this link (http://ftp.altlinux.org/pub/distributions/ALTLinux/Sisyphus/x86_64/RPMS.classic/heimdall-1.4.2-alt1.x86_64.rpm)
3) Clone the source (git clone https://github.com/Android-Boot-Manager/droidboot_device_motorola_cedric.git)
4) cd into the cloned directory.
5) Run this command (sudo alien heimdall-1.4.2-alt1.x86_64.rpm -d) this will create a deb installer for heimdall v1.4.2.
6) Run (dd if=lkbootimg/lk.bin of=boot.img conv=notrunc bs=2048 seek=1) this will merge the lk2nd bootloader located in the lkbootimg/ folder into your boot.img.)
7) Run heimdall and flash your new image! (heimdall flash --BOOT boot.img) 
8) Enjoy!

**This tutorial is by WolfLink115, and I couldnt have done it if it weren't mainly for minecrell of the PostmarketOS Mainline team as well as the rest of the team for creating LK2ND Bootloader. Thanks!**