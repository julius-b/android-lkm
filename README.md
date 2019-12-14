# Loadable Kernel Module for Android
An example of how to compile a Linux kernel module for Android.

## Motivation
Getting to know the Linux Kernel, Android and C.
This repository mostly contains some notes and example code.

## Background
All newer versions of Android kernels are [required](https://source.android.com/devices/architecture/kernel/loadable-kernel-modules) to support loadable kernel modules.
As it turns out, they're also required to be signed with a private key that is configured for the kernel at compile-time. Since I didn't see a way to get the one my kernel way signed with, and there doesn't seem to be a utility with which a key can be trusted at runtime like there is for desktop builds, I first had to compile the kernel for my device.

## Compiling and Installing on Linux
- rename `Makefile.linux` to `Makefile` first
```bash
$ make
$ sudo insmod lkm1.ko param_var=5555 # rmmod 
# c: character device
# major: as printed in dmesg
# minor: 0
$ sudo mknod -m 666 /dev/lkm1 c $major $minor
```

## Compiling the Android Kernel
Specifically the [OniiChanKernel](https://github.com/faham1997/kernel).
```bash
# Get the prebuilt GCC compiler for aarch64
~/git $ git clone https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9

# Add it to your $PATH
~/git/kernel $ export PATH="/home/julius/git/aarch64-linux-android-4.9/bin:${PATH}"

# Get the actual kernel source
# Using my own branch as I had to make certain modifications to get it to compile
~/git $ git clone https://github.com/julius-b/kernel
~/git $ cd kernel

# Configure the kernel before compiling it (using ./arch/arm64/configs/lavender-perf_defconfig)
# `oldconfig` and /proc/config.gz are not needed?
~/git/kernel $ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- lavender-perf_defconfig

# compile it (result: Image.gz-dtb)
~/git/kernel $ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- -j$(nproc --all)
...
writing new private key to 'certs/signing_key.pem'
...
  CAT     arch/arm64/boot/Image.gz-dtb
```

## Packaging the Kernel
- `Image.gz-dtb` has been generated, but it can not be flashed like this. Using `AnyKernel3` by [osm0sis](https://github.com/osm0sis) we can package it into a flashable package.
```bash
# Using a custom branch that has been modified for the Redmi Note 7
~/git $ git clone https://github.com/faham1997/AnyKernel3.git

# First copy `Image.gz-dtb` into this direcory
~/git/AnyKernel3 $ ZIP=oniichan-kernel.zip make
Creating ZIP: oniichan-kernel.zip
...
```

## Installing the Kernel
Assuming you're already running a custom recovery like TWRP or OrangeFox, you can install `oniichan-kernel.zip` from there like any other flashable zip.

Important note for the Redmit Note 7: Flashing **anything** at all can 'corrupt' your data partition, forcing you to wipe it (and loose all data). Xiaomi does not seem to be interested in fixing it. If you're extremely lucky, your bootloader even gets locked again for no reason and you won't be able to flash a new system, wipe the data partition or boot into TWRP. In that case, [XiaoMiTool V2](https://www.xiaomitool.com/V2/) is your only hope. Should your Mi Account be unavailable for any reason or your device not linked to your account, the device would effictively be bricked. Not going to buy Xiaomi again...

After booting again, you can check if it worked with `uname`:
```bash
# name defined in .config
lavender:/ $ uname -a
Linux localhost 4.4.156-Modified-OniiChanKernel-R3+ #2 SMP PREEMPT Thu Dec 12 01:32:48 CET 2019 aarch64 Android
```

## Compiling the Kernel Module
The module could have been added before compiling the kernel, but then it would always be a part of it.
The aim is to use it as a loadable module.
```bash
# clone the module into `kernel/drivers` 
~/git/kernel/drivers $ git submodule add https://github.com/juilus-b/android-lkm lkm
```
> Once Makefile.android works, submodules won't be required anymore

Add the following line to `drivers/Kconfig`, just before `endmenu`: `source "drivers/lkm/Kconfig"`.
It should look like this afterwards:
```
...
source "drivers/tee/Kconfig"

source "drivers/lkm/Kconfig"

endmenu
```

Add the following line to `drivers/Makefile`: `obj-$(CONFIG_LKM_MOD)    += lkm/`.
It should look like this afterwards:
```
...

obj-$(CONFIG_TEE)		+= tee/

obj-$(CONFIG_LKM_MOD)   += lkm/
```

And run the following commands to compile:
```bash
# update the config (keep the old config)
~/git/kernel $ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- -j$(nproc --all) oldconfig
scripts/kconfig/conf  --oldconfig Kconfig
*
* Restart config...
*
...
Linux Kernel Module Test (LKM_MOD) [M/n/y/?] (NEW) M # enter M for 'loadable module'
#
# configuration written to .config
#

# recompile the (new) modules
~/git/kernel $ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- -j$(nproc --all) modules
  CC [M]  drivers/lkm/lkm1.o
  CC      drivers/lkm/lkm1.mod.o
  LD [M]  drivers/lkm/lkm1.ko
```

## Signing the Kernel Module
As mentioned previously, Kernel Modules on Android have to be signed before they can be inserted.
```bash
lavender:/ $ insmod /sdcard/lkm1.ko
insmod: failed to load /sdcard/lkm1.ko: Required key not available
```
> Inserting an unsigned module fails

To do so, execute the following command:
```bash
# sign the module using the key generated during kernel compilation
~/git/kernel $ ./scripts/sign-file sha512 ./certs/signing_key.pem ./certs/signing_key.x509 drivers/lkm/lkm1.ko drivers/lkm/lkm1-signed.ko
```
> `lkm1-signed.ko` can now be pushed to the device

## Inserting the Kernel Module
```bash
lavender:/ $ insmod /sdcard/lkm1-signed.ko
```
> insmod needs to be run as root

## Check if it worked (using dmesg)
```bash
lavender:/ $ dmesg -wH # -w: follow, -H: human friendly
...
[  +0.010251] lkm1: lkm1_init - int_arg : 0, string_arg: test-test-123
[  +0.000020] lkm1: init - major : 238
```
> the major number is 238 (in this case), it will be required in the next step

## Usage
```bash
lavender:/ $ mknod -m 666 /dev/lkm1 c $major $minor
lavender:/ $ echo hello world > /dev/lkm1 
lavender:/ $ cat /dev/lkm1 
hello world
lavender:/ $ cat /dev/lkm1 
lavender:/ $
```
> mknod needs to be run as root

> $minor can be 0 in this case

That's it...

## Common errors
```bash
lavender:/ $ insmod lkm1.ko
insmod: error inserting lkm1.ko: Required key not available.
```
> Module is not signed or signed with a key that's not trusted by the kernel.

```bash
lavender:/ $ insmod lkm1-signed.ko
insmod: failed to load lkm1-signed.ko: Exec format error
```
> Likely compiled for a different kernel version 

## Ideas
- communicate with chrdev from android application (java or jni/ndk)
- custom syscalls (https://www.kernel.org/doc/html/v4.14/process/adding-syscalls.html)

## Notes
- most variables are declared outside of functions because the kernel stack is limited
- config:
```
DRIVER_1=y // y indicate a builtin module
DRIVER_1=m // m inicates a loadable module
```

## TODO
- Makefile.android should work even if the project itself is located outside of the kernel, but it does not.
  - (It compiles and can be signed, but the resulting binary causes a `Required key not available.` exception. (`Exec format error` in the past, somehow))
- GCC will not be supported after 2020, switch to Clang.

## Sources
- https://lwn.net/Kernel/LDD3/ (Linux Device Drivers, Third Edition)
- https://linux-kernel-labs.github.io/master/labs/device_drivers.html
- https://www.kernel.org/doc/html/latest/kernel-hacking/hacking.html
- https://www.kernel.org/doc/html/latest/driver-api/index.html
