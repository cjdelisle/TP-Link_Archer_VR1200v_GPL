/*
 * Copyright (C) 2004  Manuel Novoa III  <mjn3@codepoet.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* typedef a 32-bit type */
#ifdef _LP64
typedef unsigned int UINT4;
typedef int          INT4;
#else
typedef unsigned long UINT4;
typedef long          INT4;
#endif

typedef struct _ROMFILE_STRUCT
{
	UINT4 signOffset;	/* flash UID signature */
	UINT4 macOffset;	/* MAC address */
	UINT4 pinOffset;	/* PIN code for wireless */
	UINT4 rfpiOffset;	/* RFPI code for dect */
	UINT4 deviceIdOffset;	/* Device ID for cloud */
	UINT4 ispTagOffset; /*ISP TAG*/
}ROMFILE_STRUCT;

typedef struct _LINUX_FLASH_STRUCT
{
	UINT4 appSize;			/* kernel+rootfs size */
	UINT4 bootOffset;		/* boot loader	*/	
	UINT4 kernelOffset;		/* kernel */
	UINT4 rootfsOffset;		/* rootfsOffset is calculate in main() */
	UINT4 ispConfigOffset;
	ROMFILE_STRUCT romfileStruct;
	UINT4 configOffset;
	UINT4 dectRadioOffset;
	UINT4 radio2gOffset;
	UINT4 radio5gOffset;
}LINUX_FLASH_STRUCT;


/* 
 * brief Total image tag length	
 */
#define TAG_LEN         512

/* 
 * brief cloud ID length	
 */
#define CLOUD_ID_BYTE_LEN	16

/* 
 * brief Token length	
 */
#define TOKEN_LEN       20

/* 
 * brief magic number length	
 */
#define MAGIC_NUM_LEN	20

/* 
 * brief signature length	
 */
#define SIG_LEN		128


#define BOOTLOADER_TAG_LEN TAG_LEN


/* 
 * brief Image tag struct,have different position in Linux and vxWorks(see TAG_OFFSET)	
 */
typedef struct _LINUX_FILE_TAG
{
	UINT4 tagVersion;			/* tag version number */
	
	unsigned char hardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned char firmwareId[CLOUD_ID_BYTE_LEN];		/* FWID for cloud */
	unsigned char oemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */

	UINT4 productId;	/* product id */  
	UINT4 productVer;	/* product version */
	UINT4 addHver;		/* Addtional hardware version */
	
	unsigned char imageValidToken[TOKEN_LEN];	/* image validation token - md5 checksum */
	unsigned char magicNum[MAGIC_NUM_LEN];	 	/* magic number */
	
	UINT4 kernelTextAddr; 	/* text section address of kernel */
	UINT4 kernelEntryPoint; /* entry point address of kernel */
	
	UINT4 totalImageLen;	/* the sum of kernelLen+rootfsLen+tagLen */
	
	UINT4 kernelAddress;	/* starting address (offset from the beginning of FILE_TAG) 
									 * of kernel image 
									 */
	UINT4 kernelLen;		/* length of kernel image */
	
	UINT4 rootfsAddress;	/* starting address (offset) of filesystem image */
	UINT4 rootfsLen;		/* length of filesystem image */

	UINT4 bootAddress;		/* starting address (offset) of bootloader image */
	UINT4 bootLen;			/* length of bootloader image */

	UINT4 swRevision;		/* software revision */
	UINT4 platformVer;		/* platform version */
	UINT4 specialVer;

	UINT4 binCrc32;			/* CRC32 for bin(kernel+rootfs) */

	UINT4 reserved1[13];	/* reserved for future */

	unsigned char sig[SIG_LEN];		/* signature for update */
	unsigned char resSig[SIG_LEN];	/* reserved for signature */

	UINT4 reserved2[12];	/* reserved for future */
}LINUX_FILE_TAG;


#define ROUND 1
#define MAX_FILENAME_LEN 256 
/* three files will input-bootloader, kernel, rootfs */
#define FILE_NUM 4

enum W8930G_FILE
{
	BOOT,
	KERNEL,
	FS,
	MFG
};

#define TAG_VERSION		0x03000003 
#define VERSION_INFO	"ver. 3.0"

unsigned char md5Key[16] = 
{	/* linux - wr841n */
	0xDC, 0xD7, 0x3A, 0xA5, 0xC3, 0x95, 0x98, 0xFB, 
	0xDC, 0xF9, 0xE7, 0xF4, 0x0E, 0xAE, 0x47, 0x37
};

unsigned char mk5Key_bootloader[16] =
{	/* linux bootloader - u-boot/redboot */
	0x8C, 0xEF, 0x33, 0x5F, 0xD5, 0xC5, 0xCE, 0xFA,
	0xAC, 0x9C, 0x28, 0xDA, 0xB2, 0xE9, 0x0F, 0x42
};


unsigned char def_mac[6] = {0x00, 0x0a, 0xeb, 0x13, 0x09, 0x69};

unsigned char magicNum[MAGIC_NUM_LEN] = {0x55, 0xAA, 0x55, 0xAA, 0xF1, 0xE2, 0xD3, 0xC4, 0xE5, 0xA6, 0x6A, 0x5E, 0x4C, 0x3D, 0x2E, 0x1F, 0xAA, 0x55, 0xAA, 0x55};

