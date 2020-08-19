// SPDX-License-Identifier: GPL-2.0+
// © 2019 Mis012

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lib/fs.h>
#include <platform/iomap.h>
#include <platform.h>
#include <decompress.h>

#include "aboot/aboot.h"
#include "aboot/bootimg.h"

#include "config.h"
#include "fs_util.h"

static int boot_linux_from_ext2(char *kernel_path, char *ramdisk_path, char *dtb_path, char *cmdline) {
	void *kernel_addr = VA((addr_t)(ABOOT_FORCE_KERNEL64_ADDR));
	void *ramdisk_addr = VA((addr_t)(ABOOT_FORCE_RAMDISK_ADDR));
	void *tags_addr = VA((addr_t)(ABOOT_FORCE_TAGS_ADDR));

	unsigned char *kernel_raw = NULL;
	off_t kernel_raw_size = 0;
	off_t ramdisk_size = 0;
	off_t dtb_size = 0;

	unsigned int dev_null;
	int ret;

	printf("booting from ext2 partition 'system'\n");

	if(fs_mount("/boot", "ext2", "hd1p51")) {
		printf("fs_mount failed\n");
		return -1;
	}

	kernel_raw_size = fs_get_file_size(kernel_path);
	if(!kernel_raw_size) {
		printf("fs_get_file_size (%s) failed\n", kernel_path);
		fs_unmount("/boot");
		return -1;
	}
	kernel_raw = ramdisk_addr; //right where the biggest possible decompressed kernel would end; sure to be out of the way

	if(fs_load_file(kernel_path, kernel_raw, kernel_raw_size) < 0) {
		printf("failed loading %s at %p\n", kernel_path, kernel_addr);
		fs_unmount("/boot");
		return -1;
	}

	if(is_gzip_package(kernel_raw, kernel_raw_size)) {
		ret = decompress(kernel_raw, kernel_raw_size, kernel_addr, ABOOT_FORCE_RAMDISK_ADDR - ABOOT_FORCE_KERNEL64_ADDR, &dev_null, &dev_null);
		if(ret) {
			printf("kernel decompression failed: %d\n", ret);
			fs_unmount("/boot");
			return -1;
		}
	} else {
		memmove(kernel_addr, kernel_raw, kernel_raw_size);
	}

	kernel_raw = NULL; //get rid of dangerous reference to ramdisk_addr before it can do harm

	ramdisk_size = fs_get_file_size(ramdisk_path);
	if (!ramdisk_size) {
		printf("fs_get_file_size (%s) failed\n", ramdisk_path);
		fs_unmount("/boot");
		return -1;
	}

	if(fs_load_file(ramdisk_path, ramdisk_addr, ramdisk_size) < 0) {
		printf("failed loading %s at %p\n", ramdisk_path, ramdisk_addr);
		fs_unmount("/boot");
		return -1;
	}

	dtb_size = fs_get_file_size(dtb_path);
	if (!ramdisk_size) {
		printf("fs_get_file_size (%s) failed\n", dtb_path);
		fs_unmount("/boot");
		return -1;
	}

	if(fs_load_file("/boot/msm8937-motorola-cedric.dtb", tags_addr, dtb_size) < 0) {
		printf("failed loading /boot/msm8916-samsung-a3u-eur.dtb at %p\n", tags_addr);
		fs_unmount("/boot");
		return -1;
	}

	fs_unmount("/boot");

	boot_linux(kernel_addr, tags_addr, (const char *)cmdline, board_machtype(), ramdisk_addr, ramdisk_size);

	return -1; //something went wrong
}

int boot_to_entry(struct boot_entry *entry) {
	if(!entry->error) {
		char *linux = malloc(strlen("/boot/") + strlen(entry->linux) + 1);
		char *initrd = malloc(strlen("/boot/") + strlen(entry->initrd) + 1);
		char *dtb = malloc(strlen("/boot/") + strlen(entry->dtb) + 1);
        char *cmdline = malloc(strlen(" ABM.bootloader=1") + strlen(entry->options) + 1);
		if (!linux || !initrd || !dtb)
			return ERR_NO_MEMORY;

		strcpy(linux, "/boot/");
		strcat(linux, entry->linux);
		strcpy(initrd, "/boot/");
		strcat(initrd, entry->initrd);
		strcpy(dtb, "/boot/");
		strcat(dtb, entry->dtb);
        strcat(cmdline, entry->options);
        strcpy(cmdline, " ABM.bootloader=1");

		return boot_linux_from_ext2(linux, initrd, dtb, cmdline); // only returns on error
	}
}

