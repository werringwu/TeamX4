1. Instructions of Makefile in the Top Directory of OSDRV
The compilation scripts in this directory support the uclibc library corresponding to the arm-himix100-linux toolchain. The commands are described as follows (Hi3516E V200 is used as an example):

Note:
Due to the rectification of open source tools, some open-source tools do not
provide source code packages. Customers need to download the source code
packages when compiling OSDRV.
a:linux-4.9.37.tar.gz (https://www.kernel.org/pub/)
Save the downloaded linux-4.9.37.tar.gz file to the opensource/kernel
directory of the osdrv.

b:yaffs2utils-0.2.9.tar.gz (https://github.com/dorigom/yaffs2utils/releases)
Save the downloaded yaffs2utils-0.2.9.tar.gz to the tools/pc/mkyaffs2image/
directory of the osdrv.

c:gdb-7.9.1.tar.gz (http://ftp.gnu.org/gnu/gdb/)
Save the downloaded gdb-7.9.1.tar.gz to the tools/board/gdb/ directory of the
osdrv.

d:ncurses-6.0.tar.gz (http://ftp.gnu.org/gnu/ncurses/)
Save the downloaded ncurses-6.0.tar.gz to the tools/board/gdb/ directory of
the osdrv.

e:util-linux-2.31.tar.gz
(https://www.kernel.org/pub/linux/utils/util-linux/v2.31)

Save the downloaded util-linux-2.31.tar.gz to the tools/pc/cramfs_tool/
directory of the osdrv.

(1) Compile the entire OSDRV directory:
     	Note: By default, the kernel source code package is not released and only patch files are released. You need to download the kernel source code package from the open source community.
Download the V4.9.37 kernel from the Linux open source community.
     	1) Access the website: www.kernel.org.
     	2) Select https://www.kernel.org/pub/ of the HTTP resource. The sub-page is displayed.
     	3) Select linux/. The sub-page is displayed.
     	4) Select kernel/. The sub-page is displayed.
     	5) Select v4.x/. The sub-page is displayed.
6) Download linux-4.9.37.tar.gz (or linux-4.9.37.tar.xz) to the osdrv/opensource/kernel directory.

	make all

Note: In Makefile, the file system compilation depends on a large number of components. Therefore, the file system that is compiled independently may not be feasible. Therefore, you are advised to run the make all command.
However, U-Boot and the kernel can be compiled independently.

You can transfer the following parameters:
a. BOOT_MEDIA: spi (default) or emmc
b. CHIP: hi3516ev200 (default), hi3516ev300, hi3518ev300 or hi3516dv200 
c. TARGET_XLSM=*.xlsm (used to specify the U-Boot table file)



(2) Clear all compilation files in the osdrv directory.
	make OSDRV_CROSS=arm-himix100-linux clean

(3) Thoroughly clear all compilation files in the osdrv directory and delete the compiled images:
	make OSDRV_CROSS=arm-himix100-linux distclean

(4) Compile the kernel independently:
     	Note: Before compiling the kernel, read readme_cn.txt in osdrv/opensource/kernel.

     	After entering the kernel source code directory, run the following commands:
	cp arch/arm/configs/hi3516ev200_full_defconfig  .config
     	(When the eMMC is started, run the following command: cp arch/arm/configs/hi3516ev200_emmc_defconfig .config)
	make ARCH=arm CROSS_COMPILE=arm-himix100-linux- menuconfig
	make ARCH=arm CROSS_COMPILE=arm-himix100-linux- uImage

(5) Compile the modules independently:
After entering the kernel source code directory, run the following commands:
	cp arch/arm/configs/hi3516ev200_full_defconfig  .config
     	(When the eMMC is started, run the following command: cp arch/arm/configs/hi3516ev200_emmc_defconfig .config)
	make ARCH=arm CROSS_COMPILE=arm-himix100-linux- menuconfig
	make ARCH=arm CROSS_COMPILE=arm-himix100-linux- modules

(6) Compile U-Boot independently.
     	Note: By default, the release package is released according to the configuration of the DMEB board. If the customer board is different from the DEMB board, modify the U-Boot table based on your board environment. Otherwise, U-Boot fails or other problems may occur.

     	After the boot source code directory is entered, run the following commands:
	make ARCH=arm CROSS_COMPILE=arm-himix100-linux- hi3516ev200_defconfig
     	(When the eMMC is started, run the following command: make ARCH=arm CROSS_COMPILE=arm-himix100-linux-hi3516ev200_emmc_defconfig)
	make ARCH=arm CROSS_COMPILE=arm-himix100-linux- -j 20
     	make -C ../../../tools/pc/hi_gzip
     	cp ../../../tools/pc/hi_gzip/bin/gzip arch/arm/cpu/armv7/hi3516ev200/hw_compressed/ -rf 

     	Go to the osdrv/tools/pc/uboot_tools/ directory on Windows, open the Excel file of the board, and click Generate reg bin file on the main tab page. The reg_info.bin file is generated, which is the table file of the corresponding platform.
     	Copy reg_info.bin from the osdrv/tools/pc/uboot_tools directory to the boot source code directory and change its file name extension to .reg as follows:
     	cp ../../../tools/pc/uboot_tools/reg_info.bin .reg

     	make ARCH=arm CROSS_COMPILE=arm-himix100-linux- u-boot-z.bin
    
     	u-boot-hi3516ev200.bin generated in opensource/uboot/u-boot-2016.11 is the U-Boot image.
	
(7) Create the file system image.
     	Compiled file systems already exist in the osdrv/pub/ directory. You only need to create file system images according to the specifications of the flash memories on the board.

     	The SPI NOR flash requires the JFFS2 image. The block size information of the SPI NOR flash is used. The information will be displayed during U-Boot.
     	You are advised to directly run the mkfs.jffs2 tool and fill the parameters based on the displayed information. 
The following uses the 64 KB block as an example:
	osdrv/pub/bin/pc/mkfs.jffs2 -d osdrv/pub/rootfs_uclibc -l -e 0x10000 -o osdrv/pub/rootfs_uclibc_64k.jffs2

     	The NAND flash requires the YAFFS2 image. The page size and ECC information of the NAND flash are used. The information will be displayed during U-Boot.
     	You are advised to directly run the mkyaffs2image tool and fill the parameters based on the printed information.
The following uses the SPI NAND flash with a 2 KB page and the 4-bit ECC as an example:
    	osdrv/pub/bin/pc/mkyaffs2image100 osdrv/pub/rootfs_uclibc osdrv/pub/rootfs_uclibc_2k_4bit.yaffs2 1 2

     	The NAND flash uses the UBI file system. You can use the mkubiimg.sh tool in osdrv/tools/pc/ubi_sh to make the UBI file system.
The page size, block size, and UBIFS partition size of the NAND flash are used.
     	The following uses the NAND flash with a 2 KB page, a 128 KB block, and a 32 MB UBI partition as an example:
	mkubiimg.sh hi3516ev200 2k 128k osdrv/pub/rootfs 32M osdrv/pub/bin/pc
	
     	osdrv/pub/rootfs is the directory of the root file system.
     	osdrv/pub/bin/pc is the tool directory for making the image of the UBI file system.
     	The generated file rootfs_hi3516ev200_2k_128k_32M.ubifs is the UBI file system's image to be burnt.

2. Image Directories 
The compiled images and rootfs are stored in the osdrv/pub directory.
pub
├─ bin

│   ├─ board_uclibc -------------------------------------------- tools compiled for the board based on himix100
│   │   ├── ethtool
│   │   ├── flashcp
│   │   ├── flash_erase
│   │   ├── flash_otp_dump
│   │   ├── flash_otp_info
│   │   ├── gdb-arm-himix100-linux
│   │   ├── mtd_debug
│   │   ├── mtdinfo
│   │   ├── nanddump
│   │   ├── nandtest
│   │   ├── nandwrite
│   │   ├── sumtool
│   │   ├── ubiattach
│   │   ├── ubicrc32
│   │   ├── ubidetach
│   │   ├── ubiformat
│   │   ├── ubimkvol
│   │   ├── ubinfo
│   │   ├── ubinize
│   │   ├── ubirename
│   │   ├── ubirmvol
│   │   ├── ubirsvol
│   │   └── ubiupdatevol
│   └─ pc
│       ├── lzma
│       ├── mkfs.cramfs
│       ├── mkfs.jffs2
│       ├── mkfs.ubifs
│       ├── mkimage
│       ├── mksquashfs
│       └── ubinize
├─image_uclibc ------------------------------------------------- image file compiled based on himix100
│   ├── uImage_hi3516ev200 -----------------------------------------kernel image
│   ├── u-boot-hi3516ev200.bin -------------------------------------U-Boot image
│   ├── rootfs_hi3516ev200_64k.jffs2 -------------------------------64 KB JFFS2 image
│   ├── rootfs_hi3516ev200_128k.jffs2 ------------------------------128 KB JFFS2 image
│   ├── rootfs_hi3516ev200_256k.jffs2 ------------------------------256 KB JFFS2 image
│   ├── rootfs_hi3516ev200_2k_4bit.yaffs2 --------------------------YAFFS2 image
│   ├── rootfs_hi3516ev200_2k_24bit.yaffs2 -------------------------YAFFS2 image
│   ├── rootfs_hi3516ev200_4k_4bit.yaffs2 --------------------------YAFFS2 image
│   ├── rootfs_hi3516ev200_4k_24bit.yaffs2 -------------------------YAFFS2 image
│   ├── rootfs_hi3516ev200_2k_128k_32M.ubifs------------------------2 KB 128 KB UBI image
│   └── rootfs_hi3516ev200_4k_256k_50M.ubifs------------------------4 KB 256 KB  UBI image
│
├─ rootfs.ubiimg -----------------------------------------------UBIFS root file system
├─ rootfs_uclibc.tgz------------------------------------------- rootfs compiled based on himix100


3. Structure of the osdrv Directory
osdrv
├─Makefile --------------------------------compilation script in the osdrv directory
├─tools -----------------------------------tools
│  ├─board -------------------------------tools used on the board
│  │  ├─ethtools ------------------------ethtools tool
│  │  ├─reg-tools-1.0.0 -----------------register read/write tool
│  │  ├─eudev-3.2.7 ------------------------udev tool set
│  │  ├─mtd-utils -----------------------tool set for flash direct read/write
│  │  ├─gdb -----------------------------gdb tool
│  │  └─e2fsprogs -----------------------mkfs tool set
│  └─pc ----------------------------------tools used on the PC
│      ├─jffs2_tool-----------------------JFFS2 generator
│      ├─cramfs_tool ---------------------cramFS generator
│      ├─squashfs4.3 ---------------------SquashFS generator
│      ├─nand_production -----------------NAND mass production tool
│      ├─lzma_tool -----------------------LZMA compression tool
│      ├─zlib ----------------------------zlib tool
│      ├─mkyaffs2image -------------------YAFFS2 generator
│      └─uboot_tools ---------------------tools for generating the U-Boot image, XLS file, DDR initialization script, and reg_info.bin 
├─pub -------------------------------------images
│  ├─image_uclibc ------------------------images compiled based on himix100, including the U-Boot, kernel, and file system images, which can be burnt to the flash memory, 
│  ├─bin ---------------------------------tools not placed in the root file system
│  │  ├─pc ------------------------------tool running on the PC
│  │  ├─board_uclibc --------------------tool running on the board, compiled based on himix100 
│  └─rootfs_uclibc.tgz -------------------root file system compiled based on himix100
├─opensource-------------------------------open-source source codes
│  ├─busybox -----------------------------BusyBox source code
│  ├─uboot -------------------------------source codes of U-Boot and secure boot
│  └─kernel ------------------------------kernel source code
└─rootfs_scripts --------------------------script for generating the root file system

4. Precautions

(1) When the source code package is copied on Windows, executable files on Linux may become non-executable. As a result, compilation cannot be performed. After U-Boot or the kernel is compiled, there are many symbolic link files. Duplicating these source code packages on Windows will increase the sizes of the source code packages because the symbolic link files on Linux become real files on Windows, therefore, the size of the source code package increases. Do not copy a source code package on Windows.
(2) If you need to replace the toolchain which has been used for compiling, clear the compilation file of the toolchain before replacing the toolchain.
(3) Compile the board software
    This chip provides the floating-point operation unit and NEON. The libraries in the file system are compiled in the soft floating-point format using NEON. Therefore, you need to add the -mcpu=cortex-a7, -mfloat-abi=softfp, and -mfpu=neon-vfpv4 options to Makefile during compilation of all board codes. 
    The following uses A7 as an example:
    
    CFLAGS += -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations
    CXXFlAGS +=-mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations

    XX in CXXFlAGS is determined according to the specific name of the macro used in Makefile, for example, CPPFLAGS.
