# lk2nd for Android Boot Manager
lk2nd is a bootloader for Qualcomm MSM devices,
based on the [CodeAurora Little Kernel fork](https://source.codeaurora.org/quic/la/kernel/lk/).
It provides an Android dualboot menu.

lk2nd does not replace the stock bootloader. It is packaged into an Android
boot image and then loaded by the stock bootloader as a "secondary" bootloader.
The real Android boot images are placed into the unused OEM partition.

Important: This fork is used by Android Boot Manager and has code specific to it. It will not work without. Take a look at https://github.com/Junak/lk2nd

### Supported devices
- Moto G5 - cedric

## Installation
Install using our Android App.

## How it works
We use LVGL to draw the boot menu, lk-ext2 to read the OEM partition and the normal booting skills of little kernel to start Android (or other OSes).

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
