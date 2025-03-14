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

/* July 29, 2004
 *
 * This is a hacked replacement for the 'trx' utility used to create
 * wrt54g .trx firmware files.  It isn't pretty, but it does the job
 * for me.
 *
 * As an extension, you can specify a larger maximum length for the
 * .trx file using '-m'.  It will be rounded up to be a multiple of 4K.
 * NOTE: This space will be malloc()'d.
 *
 * August 16, 2004
 *
 * Sigh... Make it endian-neutral.
 *
 * TODO: Support '-b' option to specify offsets for each file.
 *
 * February 19, 2005 - mbm
 *
 * Add -a (align offset) and -b (absolute offset)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <byteswap.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <elf.h>
#include <sys/stat.h>


#include "mkimage.h"
#include "md5_interface.h"


#if 0
#define __BYTE_ORDER __LITTLE_ENDIAN

#if __BYTE_ORDER == __BIG_ENDIAN
#define STORE32_LE(X)		bswap_32(X)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define STORE32_LE(X)		(X)
#endif


#define STORE32_LE(X)		bswap_32(X)
#endif


#define CLOUD_ID_STR_LEN    33
#define BUFFER_LEN64	64


typedef struct _DEV_INFO
{
	unsigned int sw_revision;
	unsigned int platform_ver;
	unsigned int special_ver;
	unsigned int product_id;
	unsigned int product_ver;
	unsigned int add_hver;
	char model_name[BUFFER_LEN64];
	char build_date[BUFFER_LEN64];
	unsigned int build_time;
	char dev_ver[BUFFER_LEN64];
	unsigned char hw_id[CLOUD_ID_BYTE_LEN];
	unsigned char fw_id[CLOUD_ID_BYTE_LEN];
	unsigned char oem_id[CLOUD_ID_BYTE_LEN];
    unsigned char is_beta;
    unsigned char is_trans;
	unsigned char is_datecode;
    char build_spec[BUFFER_LEN64];
}DEV_INFO;


/* for 6 MTD parts, 16MB flash */
LINUX_FLASH_STRUCT flash_type1 =
{
	.appSize = 0xFA0000,	/* kernel+rootfs size */
	.bootOffset = 0x000000,	/* boot loader	*/
	.kernelOffset = 0x020000,	/* kernel */
	.rootfsOffset = 0x000000,	/* rootfsOffset is calculate in main() */
	.ispConfigOffset = 0xFC0000,
	.romfileStruct = {
			.signOffset = 0xFDE000, 
			.macOffset = 0xFDF100, 
			.pinOffset = 0xFDF200, 
			.rfpiOffset = 0xFDF300,
			.deviceIdOffset = 0xFDF400,
			.ispTagOffset = 0xFDF500,
			},
	.configOffset = 0xFE0000,	/* config */
	.dectRadioOffset = 0x000000,
	.radio2gOffset = 0xFF0000,
	.radio5gOffset = 0xFF0000,
};


/* for 7 MTD parts, 16MB flash */
LINUX_FLASH_STRUCT flash_type2 =
{
	.appSize = 0xFA0000,	/* kernel+rootfs size */
	.bootOffset = 0x000000,	/* boot loader	*/
	.kernelOffset = 0x020000,	/* kernel */
	.rootfsOffset = 0x000000,	/* rootfsOffset is calculate in main() */
	.romfileStruct = {
			.signOffset = 0xFCE000, 
			.macOffset = 0xFCF100, 
			.pinOffset = 0xFCF200, 
			.rfpiOffset = 0xFCF300,
			.deviceIdOffset = 0xFCF400,
			},
	.configOffset = 0xFD0000,	/* config */
	.dectRadioOffset = 0xFE0000,
	.radio2gOffset = 0xFF0000,
	.radio5gOffset = 0xFF0000,
};


#if 1 /* Modified by Zhao Mengqing, 2017-3-29, for TD-W9960 1.0 */
/* for 6 MTD parts, 8MB flash */
LINUX_FLASH_STRUCT flash_type3 =
{
	.appSize = 0x7B0000,	/* kernel+rootfs size */
	.bootOffset = 0x000000,	/* boot loader	*/
	.kernelOffset = 0x020000,	/* kernel */
	.rootfsOffset = 0x000000,	/* rootfsOffset is calculate in main() */
	.romfileStruct = {
			.signOffset = 0x7DE000, 
			.macOffset = 0x7DF100, 
			.pinOffset = 0x7DF200, 
			.rfpiOffset = 0x7DF300,
			.deviceIdOffset = 0x7DF400,
			},
	.configOffset = 0x7E0000,	/* config */
	.dectRadioOffset = 0x000000,
	.radio2gOffset = 0x7F0000,
	.radio5gOffset = 0x7F8000,
};
#else
/* for 8 MTD parts, 8MB flash */
LINUX_FLASH_STRUCT flash_type3 =
{
	.appSize = 0x790000,	/* kernel+rootfs size */
	.bootOffset = 0x000000,	/* boot loader	*/
	.kernelOffset = 0x020000,	/* kernel */
	.rootfsOffset = 0x000000,	/* rootfsOffset is calculate in main() */
	.romfileStruct = {
			.signOffset = 0x7DE000, 
			.macOffset = 0x7DF100, 
			.pinOffset = 0x7DF200, 
			.rfpiOffset = 0x7DF300,
			.deviceIdOffset = 0x7DF400,
			},
	.configOffset = 0x7C0000,	/* config */
	.dectRadioOffset = 0x000000,
	.radio2gOffset = 0x7B0000,
	.radio5gOffset = 0x7F0000,
};
#endif

/* for 8 MTD parts, 16MB flash */
LINUX_FLASH_STRUCT flash_type4 =
{
	.appSize = 0xF90000,	/* kernel+rootfs size */
	.bootOffset = 0x000000,	/* boot loader	*/
	.kernelOffset = 0x020000,	/* kernel */
	.rootfsOffset = 0x000000,	/* rootfsOffset is calculate in main() */
	.romfileStruct = {
			.signOffset = 0xFDE000, 
			.macOffset = 0xFDF100, 
			.pinOffset = 0xFDF200, 
			.rfpiOffset = 0xFDF300,
			.deviceIdOffset = 0xFDF400,
			},
	.configOffset = 0xFC0000,	/* config */
	.dectRadioOffset = 0x000000,
	.radio2gOffset = 0xFB0000,
	.radio5gOffset = 0xFF0000,
};


/* for 8 MTD parts, lantiq, 16MB flash */
LINUX_FLASH_STRUCT flash_type5 =
{
	.appSize = 0xF90000,	/* kernel+rootfs size */
	.bootOffset = 0x000000,	/* boot loader	*/
	.kernelOffset = 0x020000,	/* kernel */
	.rootfsOffset = 0x000000,	/* rootfsOffset is calculate in main() */
	.romfileStruct = {
			.signOffset = 0xFDE000, 
			.macOffset = 0xFDF100, 
			.pinOffset = 0xFDF200, 
			.rfpiOffset = 0xFDF300,
			.deviceIdOffset = 0xFDF400,
			},
	.configOffset = 0xFC0000,	/* config */
	.dectRadioOffset = 0xFB0000,
	.radio2gOffset = 0x000000,
	.radio5gOffset = 0xFF0000,
};


/* for 9 MTD parts, lantiq, 16MB flash */
LINUX_FLASH_STRUCT flash_type6 =
{
	.appSize = 0xF80000, /* kernel+rootfs size */
	.bootOffset = 0x000000, /* boot loader */
	.kernelOffset = 0x020000, /* kernel */
	.rootfsOffset = 0x000000, /* rootfsOffset is calculate in main() */
	.romfileStruct = {
			.signOffset = 0xFAE000, 
			.macOffset = 0xFAF100, 
			.pinOffset = 0xFAF200, 
			.rfpiOffset = 0xFAF300,
			.deviceIdOffset = 0xFAF400,
			},
	/* 0xFB0000 is for lantiq ddr tuning offset */
	.configOffset = 0xFC0000, /* config */
	.dectRadioOffset = 0xFD0000,
	.radio2gOffset = 0xFE0000,
	.radio5gOffset = 0xFF0000,
};


static LINUX_FLASH_STRUCT l_flash_struct;
static DEV_INFO l_dev_info;

static int l_max_kernel_size = 0;
static int l_max_root_size = 0;

static int l_target_endian = -1; /* 0-big, 1-little */

#define STORE32_LE(X)		(l_target_endian ? (X) : bswap_32(X))



/**********************************************************************/

void usage(void) __attribute__ (( __noreturn__ ));

void replaceBlank(char *str);
int getStrAttrVal(char *buf, char *attr, char *value, int maxLen);
int idstrToByte(const char *pIdstr, unsigned char *pByte);
int getVersion(const char *reduced_xml_name);
int fill_buffer(char *buffer, char filename[][MAX_FILENAME_LEN], char *vmlinux, int flash_size);


void usage(void)
{
		fprintf(stderr, 
	"Usage: mkimage [OPTIONS]\n\n"
	"  -c, --mfg=FILE 			manufacture config file\n"
	"  -m, --maxkernelsize=SIZE	MAX Kernel size\n"
	"  -s, --flashsize=SIZE		Flash size\n"
	"  -e, --targetEndian=endian Target CPU endian\n"
	"  -k, --kernel=FILE		Kernel file\n"
	"  -f, --fs=FILE			Filesystem file\n"
	"  -o, --output=FILE		Output Filename prefix\n"
	"  -i, --image-path=PATH	image path\n"
	"  -p, --xmlName=FILE		reduced_data_model file\n"
	"  -d, --dualImage		    Have Dual Image(Only for 16M flash,8M up bin)\n"
	"  -t, --mtdType=type		MTD part type\n"
	"  -h, --help			    Display this message\n");

	exit(EXIT_FAILURE);
}

void replaceBlank(char *str)
{
	if (NULL == str)
	{
		fprintf(stderr, "Get NULL replace blank string\n");
		return;
	}

	while(*str != '\0')
	{
		if (' ' == *str)
		{
			*str = '_';
		}
		
		*str++;
	}

	return;
}

int getStrAttrVal(char *buf, char *attr, char *value, int maxLen)
{
	char *p = NULL; 
	char *q = NULL; 
	char *valStart = NULL; 
	char *valEnd = NULL;

	if ((buf == NULL) || (attr == NULL) || (value == NULL))
	{
		fprintf(stderr, "%s(): get attr \"%s\" parameter error\n", __FUNCTION__, attr);
		return -1;
	}
	
	p = strstr(buf, attr);		
	if (p == NULL)
	{
		fprintf(stderr, "%s(): cannot find attr %s\n", __FUNCTION__, attr);
		return -1;
	}

	q = strstr(p , "d=");
	p = strchr(q, '>');
	if ((q == NULL) || (p == NULL))
	{
		fprintf(stderr, "%s(): \"%s\" cannot find \"d=\" Or \">\"\n", __FUNCTION__, attr);
		return -1;
	}

	q += 2;
	
	while (isspace(*q))
	{
		q++;
	}

	/*string value with quote */
	if ((*q == '\"') || (*q == '\''))
	{
		valStart = q + 1;
		valEnd = strchr(valStart, *q);

		if ((valEnd > p) || (valEnd - valStart > maxLen))
		{
			fprintf(stderr, "%s(): \"%s\" can't find right quote Or exceed max len\n", __FUNCTION__, attr);
			return -1;
		}
	}
	else
	{
		valStart = q;

		while(!isspace(*q) && (*q != '>'))
		{
			if ((*q) == '/' && (*(q + 1) == '>'))
			{
				break;
			}
			q++;
		}
		valEnd = q;

		if (valEnd - valStart > maxLen)
		{
			fprintf(stderr, "%s(): value of attr \"%s\" exceed max length\n", __FUNCTION__, attr);
			return -1;
		}
	}
	
	memcpy(value, valStart, valEnd - valStart);

	return 0;
}

int idstrToByte(const char *pIdstr, unsigned char *pByte)
{
	int i = 0, c = 0;
	
	for (i = 0; i < 16; i++)
	{	
		if (*pIdstr >= '0' && *pIdstr <= '9')
		{
			c  = (unsigned char) (*pIdstr++ - '0'); 
		}
		else if (*pIdstr >= 'a' && *pIdstr <= 'f')
		{
			c  = (unsigned char) (*pIdstr++ - 'a') + 10;
		}
		else if (*pIdstr >= 'A' && *pIdstr <= 'F')
		{
			c  = (unsigned char) (*pIdstr++ - 'A') + 10; 
		}
		else
		{
			return -1;
		}

		c <<= 4;

		if (*pIdstr >= '0' && *pIdstr <= '9')
		{
			c |= (unsigned char) (*pIdstr++ - '0');
		}
		else if (*pIdstr >= 'a' && *pIdstr <= 'f')
		{
			c |= (unsigned char) (*pIdstr++ - 'a') + 10;
		}
		else if (*pIdstr >= 'A' && *pIdstr <= 'F')
		{
			c |= (unsigned char) (*pIdstr++ - 'A') + 10;
		}
		else
		{
			return -1;
		}

		pByte[i] = (unsigned char) c;

	}

	return 0;
}


int getVersion(const char *reduced_xml_name)
{
	int fd;
	char tmp[12] = {0};
	char tmpId[33] = {0};

	#define MAX_LEN	(80 * 1024)
	char buf[MAX_LEN] = {0};

	fd = open(reduced_xml_name, O_RDONLY);

	if (fd < 0)
	{
		perror("open profile error\n");
		return -1;
	}

	while (read(fd, buf, MAX_LEN - 1) > 0)
	{
		buf[MAX_LEN - 1] = 0;

		/* Get software revision */
		char *q = strstr(buf, "X_TP_SoftwareRevision");
		char *p = strstr(q, "d=");
		char *pEnd = NULL;
		int strLen = 0;
		
		if (p != 0)
		{
			p += 2;
			strncpy(tmp, p, 10);			
			l_dev_info.sw_revision = strtoul(tmp, NULL, 16);
		}

		/* Get platform version */
		p = strstr(buf, "X_TP_PlatformVersion");
		q = strstr(p, "d=");
		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.platform_ver = strtoul(tmp, NULL, 16);  
		}

		/* Get product Id */		
		p = strstr(buf, "X_TP_ProductID");
		q = strstr(p, "d=");

		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.product_id = strtoul(tmp, NULL, 16);  
		}

		/* Get product version */		
		p = strstr(buf, "X_TP_ProductVersion");
		q = strstr(p, "d=");

		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.product_ver = strtoul(tmp, NULL, 16);  
		}

		/* Get special version */		
		p = strstr(buf, "X_TP_SpecialVersion");
		q = strstr(p, "d=");

		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.special_ver = strtoul(tmp, NULL, 16);  
		}

		
		/* Get Additional HardwareVersion */		
		memset(tmp, 0, 12);	
		if (getStrAttrVal(buf, "AdditionalHardwareVersion", tmp, 10) != 0)
		{
			close(fd);
			return -1;
		}
		l_dev_info.add_hver = strtoul(tmp, NULL, 16);

		/* Get build date */		
		if (getStrAttrVal(buf, "X_TP_BuildDate", l_dev_info.build_date, BUFFER_LEN64) != 0)
		{
			close(fd);
			return -1;
		}
		
		/* Get build time */
		p = strstr(buf, "X_TP_BuildTime");
		q = strstr(p, "d=");

		memset(tmp, 0, 12);
		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 5);
			l_dev_info.build_time = strtoul(tmp, NULL, 10);  
	
		}
		/* Get device model version */		
		if (getStrAttrVal(buf, "X_TP_DevModelVersion", l_dev_info.dev_ver, BUFFER_LEN64) != 0)
		{
			close(fd);
			return -1;
		}

		/* Get model name */		
		if (getStrAttrVal(buf, "ModelName", l_dev_info.model_name, BUFFER_LEN64) != 0)
		{
			close(fd);
			return -1;
		}

		/* Get HardwareID */		
		if (getStrAttrVal(buf, "X_TP_HardwareID", tmpId, CLOUD_ID_STR_LEN) != 0)
		{
			close(fd);
			return -1;
		}

		printf("hwid is %s\n", tmpId);

		if (idstrToByte(tmpId, l_dev_info.hw_id) != 0)
		{
			close(fd);
			return -1;
		}

		/* Get FirmwareID */	
		if (getStrAttrVal(buf, "X_TP_FirmwareID", tmpId, CLOUD_ID_STR_LEN) != 0)
		{
			close(fd);
			return -1;
		}

		printf("fwid is %s\n", tmpId);

		if (idstrToByte(tmpId, l_dev_info.fw_id) != 0)
		{
			close(fd);
			return -1;
		}		


		/* Get OemID */	
		if (getStrAttrVal(buf, "X_TP_OemID", tmpId, CLOUD_ID_STR_LEN) != 0)
		{
			close(fd);
			return -1;
		}

		printf("oemid is %s\n", tmpId);

		if (idstrToByte(tmpId, l_dev_info.oem_id) != 0)
		{
			close(fd);
			return -1;
		}

        /* Get beta info */
        p = strstr(buf, "X_TP_IsBeta");
        q = strstr(p, "d=");

        if (q != 0)
        {
            q += 2;
            strncpy(tmp, q, 10);
            l_dev_info.is_beta = strtoul(tmp, NULL, 16);
        }

        /* Get trans info */
        p = strstr(buf, "X_TP_IsTrans");
        q = strstr(p, "d=");

        if (q != 0)
        {
            q += 2;
            strncpy(tmp, q, 10);
            l_dev_info.is_trans = strtoul(tmp, NULL, 16);
        }
		
		/* Get date code info */
		p = strstr(buf, "X_TP_IsDateCode");
		q = strstr(p, "d=");

		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);
			l_dev_info.is_datecode = strtoul(tmp, NULL, 16);
		}
		
        /* Get build date */
        if (getStrAttrVal(buf, "X_TP_BuildSpec", l_dev_info.build_spec, BUFFER_LEN64) != 0)
        {
            close(fd);
            return -1;
        }

		break;
	}

	close(fd);

	return 0;
}


int fill_buffer(char *buffer, char filename[][MAX_FILENAME_LEN], char *vmlinux, int flash_size)
{
	FILE *in;
	size_t n;
	size_t boot_len, kernel_len, fs_len, mfg_len;
	uint32_t cur_len;
	char *buf = buffer;
	unsigned long maxlen = flash_size;
	struct _LINUX_FILE_TAG *kernel_tag, *boot_tag;
	unsigned long image_len;
	int vm_fd;
	Elf32_Ehdr ehdr; 
	Elf32_Shdr shdr;
	
	/* 1) bootloader */
	boot_tag = (struct _LINUX_FILE_TAG *)buf;
	
	cur_len = BOOTLOADER_TAG_LEN;
	
	if (!(in= fopen(filename[BOOT], "r"))) {
		fprintf(stderr, "can not open \"%s\" for reading\n", filename[BOOT]);
		usage();
	}
	fprintf(stderr, "bootloader is 0x%x\n", cur_len);
	n = fread(buf + cur_len, 1, maxlen - cur_len, in);
	if (!feof(in)) {
		fprintf(stderr, "fread failure or file \"%s\" too large\n", filename[BOOT]);
		fclose(in);
		return EXIT_FAILURE;
	}	
	fclose(in);

	/* �������������֮ǰnΪ816645������֮��n��Ϊ816648 */
	if (n & (ROUND-1)) {
		memset(buf + cur_len + n, 0, ROUND - (n & (ROUND-1)));
		n += ROUND - (n & (ROUND-1));
	}

	boot_len = n;
	
	if (boot_len > (l_flash_struct.kernelOffset - l_flash_struct.bootOffset))
	{
		fprintf(stderr, "boot size is %x, too large %x\n", boot_len, (l_flash_struct.kernelOffset - l_flash_struct.bootOffset));
		return EXIT_FAILURE;
	}
	/* end read bootloader */

	/* 2) linux kernel */
	cur_len = l_flash_struct.kernelOffset + BOOTLOADER_TAG_LEN;
	kernel_tag = (struct _LINUX_FILE_TAG *) (buf + cur_len);

	cur_len += TAG_LEN;

	if (!(in= fopen(filename[KERNEL], "r"))) {
		fprintf(stderr, "can not open \"%s\" for reading\n", filename[KERNEL]);
		usage();
	}
	fprintf(stderr, "kernel is 0x%x\n", cur_len);
	n = fread(buf + cur_len, 1, maxlen - cur_len, in);
	if (!feof(in)) {
		fprintf(stderr, "fread failure or file \"%s\" too large\n", filename[KERNEL]);
		fclose(in);
		return EXIT_FAILURE;
	}	
	fclose(in);

	if (n & (ROUND-1)) {
		memset(buf + cur_len + n, 0, ROUND - (n & (ROUND-1)));
		n += ROUND - (n & (ROUND-1));
	}

	kernel_len = n;

	/* use dynamic value by yangxv, 2011.11.20 */
	/* if (kernel_len >= MAX_KERNEL_SIZE) */
	if (kernel_len > l_max_kernel_size)
	{
		fprintf(stderr, "kernel size is %x, too large\n", kernel_len);
		return EXIT_FAILURE;
	}
	/* end read linux kernel */

	/* 3) rootfs */
	cur_len = l_flash_struct.rootfsOffset + BOOTLOADER_TAG_LEN;
	
	if (!(in= fopen(filename[FS], "r"))) {
		fprintf(stderr, "can not open \"%s\" for reading\n", filename[FS]);
		usage();
	}

	fprintf(stderr, "rootfs is 0x%x\n", cur_len);
	n = fread(buf + cur_len, 1, maxlen - cur_len, in);
	if (!feof(in)) {
		fprintf(stderr, "fread failure or file \"%s\" too large\n", filename[FS]);
		fclose(in);
		return EXIT_FAILURE;
	}	
	fclose(in);

	if (n & (ROUND-1)) {
		memset(buf + cur_len + n, 0, ROUND - (n & (ROUND-1)));
		n += ROUND - (n & (ROUND-1));
	}

	fs_len = n;

	l_max_root_size = l_flash_struct.appSize - l_max_kernel_size;
	if (fs_len > l_max_root_size)
	{
		fprintf(stderr, "rootfs size is %x, too large\n", fs_len);
		return EXIT_FAILURE;
	}
	/* end read rootfs */

	/* 4) manufacture config */	
	if (!(in= fopen(filename[MFG], "r"))) {
		fprintf(stderr, "there is no \"%s\" for reading\n", filename[MFG]);
	}
	else
	{
		cur_len = BOOTLOADER_TAG_LEN + l_flash_struct.configOffset;
		fprintf(stderr, "MFG_conf.bin is 0x%x\n", cur_len);
		n = fread(buf + cur_len, 1, maxlen - cur_len, in);
		if (!feof(in)) {
			fprintf(stderr, "fread failure or file \"%s\" too large\n", filename[MFG]);
			fclose(in);
			return EXIT_FAILURE;
		}	
		fclose(in);
		
		if (n & (ROUND-1)) {
			memset(buf + cur_len + n, 0, ROUND - (n & (ROUND-1)));
			n += ROUND - (n & (ROUND-1));
		}
		
		mfg_len = n;
		if (mfg_len > 0x10000)
		{
			fprintf(stderr, "MFG_conf.bin size is %x, too large\n", mfg_len);
			return EXIT_FAILURE;
		}

	}
	/*read manufacture config*/

	/* 5) kernel tag */
	if (!(vm_fd = open(vmlinux, O_RDONLY))) {
		fprintf(stderr, "can not open \"%s\" for reading\n", optarg);
		usage();
	}
	
	read(vm_fd, &ehdr, sizeof(ehdr)); 
	read(vm_fd, &shdr, sizeof(shdr));
	
	close(vm_fd);				
	 
	printf("Entry Point: %#X\n", STORE32_LE(ehdr.e_entry));
	printf("Text Addr: %#X\n", STORE32_LE(shdr.sh_addr));

	kernel_tag->tagVersion = STORE32_LE(TAG_VERSION);
	memcpy(kernel_tag->magicNum, magicNum, MAGIC_NUM_LEN);

	kernel_tag->productId = STORE32_LE(l_dev_info.product_id);
	kernel_tag->productVer = STORE32_LE(l_dev_info.product_ver);

	kernel_tag->swRevision = STORE32_LE(l_dev_info.sw_revision);
	kernel_tag->platformVer = STORE32_LE(l_dev_info.platform_ver);
	kernel_tag->addHver = STORE32_LE(l_dev_info.add_hver);

	kernel_tag->specialVer = STORE32_LE(l_dev_info.special_ver);

	memcpy(kernel_tag->hardwareId, l_dev_info.hw_id, CLOUD_ID_BYTE_LEN);
	memcpy(kernel_tag->firmwareId, l_dev_info.fw_id, CLOUD_ID_BYTE_LEN);
	memcpy(kernel_tag->oemId, l_dev_info.oem_id, CLOUD_ID_BYTE_LEN);

	memset(kernel_tag->imageValidToken, 0, TOKEN_LEN);
	
	kernel_tag->kernelTextAddr = (shdr.sh_addr);
	kernel_tag->kernelEntryPoint = (ehdr.e_entry);

	image_len = l_flash_struct.appSize;
	kernel_tag->totalImageLen = STORE32_LE(image_len);

	kernel_tag->bootAddress = STORE32_LE(0x0);
	kernel_tag->bootLen = STORE32_LE(0x0);

	kernel_tag->kernelAddress = STORE32_LE(TAG_LEN);
	kernel_tag->kernelLen = STORE32_LE(kernel_len);

	kernel_tag->rootfsAddress = STORE32_LE(l_flash_struct.rootfsOffset - l_flash_struct.kernelOffset);
	kernel_tag->rootfsLen = STORE32_LE(fs_len);

	kernel_tag->binCrc32 = STORE32_LE(calc_crc32((const char *)kernel_tag + TAG_LEN, l_flash_struct.appSize - TAG_LEN));

	memset(kernel_tag->sig, 0, SIG_LEN);
	memset(kernel_tag->resSig, 0, SIG_LEN);
	
	/* deleted by yangxv,
	 * do md5 checksum caculate when signature firmware
	 */
#if 0
	unsigned char md5[MD5_HASH_SIZE];

	memcpy(kernel_tag->imageValidToken, md5Key, 16);
	md5_make_digest(md5, (unsigned char*)kernel_tag, image_len);
	memcpy(kernel_tag->imageValidToken, md5, MD5_HASH_SIZE);
	/* end fill kernel tag */
#endif

#if 0
	/* 5) bootloader tag */
	int up_boot_image_len = l_flash_struct.configOffset + BOOTLOADER_TAG_LEN;

	/* some value use are same as kernel tag */
	memcpy(boot_tag, kernel_tag, TAG_LEN);
	
	boot_tag->totalImageLen = STORE32_LE(up_boot_image_len);
	boot_tag->bootloaderAddress	= STORE32_LE(0x0);
	boot_tag->bootloaderLen = STORE32_LE(boot_len);

	memcpy(boot_tag->imageValidationToken, mk5Key_bootloader, 16);
	md5_make_digest(md5, (unsigned char*)boot_tag, up_boot_image_len);
	memcpy(boot_tag->imageValidationToken, md5, MD5_HASH_SIZE);
	/* end fill boot tag */
#endif
	/* 5) fill default mac and pin */
	memcpy(buf + BOOTLOADER_TAG_LEN + l_flash_struct.romfileStruct.macOffset, def_mac, 6);
	memset(buf + BOOTLOADER_TAG_LEN + l_flash_struct.romfileStruct.pinOffset, 0, 8);
	/* end fill mac and pin */


    /* 6) bootloader tag */
    int up_boot_image_len 
    	= l_flash_struct.appSize + (l_flash_struct.kernelOffset - l_flash_struct.bootOffset) + BOOTLOADER_TAG_LEN;

    /* some value use are same as kernel tag */
    memcpy(boot_tag, kernel_tag, TAG_LEN);

    boot_tag->totalImageLen = STORE32_LE(up_boot_image_len);
    boot_tag->bootAddress     = STORE32_LE(0x0);
    boot_tag->bootLen = STORE32_LE(boot_len);

#if 0
    memcpy(boot_tag->imageValidToken, mk5Key_bootloader, 16);
    md5_make_digest(md5, (unsigned char*)boot_tag, up_boot_image_len);
    memcpy(boot_tag->imageValidToken, md5, MD5_HASH_SIZE);
#endif
    /* end fill boot tag */
}

int main(int argc, char **argv)
{
	FILE *out = stdout;
	char *ofn = NULL;
	char *buf;
	char filename[FILE_NUM][MAX_FILENAME_LEN] = {0, 0, 0};
	
	char vmlinux_name[MAX_FILENAME_LEN] = {0};
	char reduced_xml_name[MAX_FILENAME_LEN] = {0};
	char image_path[MAX_FILENAME_LEN] = {0};

	int c = 0;
	int flash_size = 0;

	char prefix[MAX_FILENAME_LEN] = {0};
	char bin_file[MAX_FILENAME_LEN] = {0};
	char update_file[MAX_FILENAME_LEN] = {0};
	char update_file_boot[MAX_FILENAME_LEN] = {0};
	char tag_kernel[MAX_FILENAME_LEN] = {0};

	char image_name_prefix[MAX_FILENAME_LEN] = {0};
    char image_name_suffix[MAX_FILENAME_LEN] = {0};

	int dual_image = 0;
	int mtd_type = 0;

	fprintf(stderr, "YangXv <yangxu@tp-link.com.cn> modified from trx for make image\n");

	static const struct option arg_options[] = {
		{"mfg",				required_argument, 	0, 'c'},
		{"maxkernelsize",	required_argument,	0, 'm'},
		{"flashsize",	required_argument,	0, 's'},
		{"targetEndian",required_argument,	0, 'e'},
		{"boot", 		required_argument,	0, 'b'},
		{"kernel",		required_argument,	0, 'k'},
		{"fs",			required_argument,	0, 'f'},
		{"output",		required_argument,	0, 'o'},
		{"xmlName",		required_argument,	0, 'p'},
		{"imagep",		required_argument,	0, 'i'},
		{"vmlinux",		required_argument,	0, 'v'},
		{"dualImage",	required_argument,	0, 'd'},
		{"mtdType",		required_argument,	0, 't'},
		{"help",		no_argument,		0, 'h'},
		{0, 0, 0, 0}
	};	
	
	/* get options */
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "c:m:s:e:b:k:f:o:p:i:v:d:t:h", arg_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
		case 'm':
			sscanf(optarg, "%X", &l_max_kernel_size);
			break;			
		case 's':
			sscanf(optarg, "%X", &flash_size);
			break;
		case 'e':
			sscanf(optarg, "%d", &l_target_endian);
			break;
		case 'b':
			strncpy(filename[BOOT], optarg, 
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'k':
			strncpy(filename[KERNEL], optarg, 
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'f':
			strncpy(filename[FS], optarg, 
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'c':
			strncpy(filename[MFG], optarg, 
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'v':
			strncpy(vmlinux_name, optarg, 
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'o':
			strncpy(prefix, optarg, 
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'p':
			strncpy(reduced_xml_name, optarg,
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'i':
			strncpy(image_path, optarg,
				strlen(optarg) > MAX_FILENAME_LEN?MAX_FILENAME_LEN:strlen(optarg));
			break;
		case 'd':
			dual_image = 1;
			break;
		case 't':
			sscanf(optarg, "%d", &mtd_type);
			break;
		case 'h':
			usage();
			break;
		default:
			break;
		}
	}

	if (filename[BOOT] == 0 || filename[KERNEL] == 0 || filename[FS] == 0
		|| image_path[0] == 0  || vmlinux_name[0] == 0)
	{
		fprintf(stderr, "None file name\n");
		return;
	}
	
	if(filename[MFG] == 0)
	{
		fprintf(stderr, "None manufacture config\n");
	}

	if (-1 == l_target_endian)
	{
		fprintf(stderr, "Must set cpu endian\n");
		return;		
	}

	/* malloc image buffer */
	if (flash_size == 0)
	{
		flash_size = 0x800000;
	}

	if (!(buf = malloc(flash_size))) 
	{
		fprintf(stderr, "malloc failed\n");
		return EXIT_FAILURE;
	}
	memset(buf, 0xFF, flash_size);

	/* add by yangxv for accept dynamic max kernel size, 2011.11.20 */
	if (l_max_kernel_size == 0)
	{
		fprintf(stderr, "None MAX kernel size\n");
		return EXIT_FAILURE;
	}

	switch(mtd_type)
	{
	case 1:
		l_flash_struct = flash_type1;
		break;
	case 2:
		l_flash_struct = flash_type2;
		break;
	case 3:
		l_flash_struct = flash_type3;
		break;
	case 4:
		l_flash_struct = flash_type4;
		break;
	case 5:
		l_flash_struct = flash_type5;
		break;
	case 6:
		l_flash_struct = flash_type6;
		break;
	default:
		fprintf(stderr, "None MTD type\n");
		return;		
	}

	l_flash_struct.rootfsOffset = l_flash_struct.kernelOffset + l_max_kernel_size;
	fprintf(stderr, "appSize	is %x\n", l_flash_struct.appSize);
	fprintf(stderr, "bootOffset	is %x\n", l_flash_struct.bootOffset);
	fprintf(stderr, "kernelOffset	is %x\n", l_flash_struct.kernelOffset);
	fprintf(stderr, "rootfsOffset   is %x\n", l_flash_struct.rootfsOffset);
	/* end added */

	if (getVersion(reduced_xml_name) < 0)
	{
		fprintf(stderr, "get version error!\n");
		return -1;
	}

	replaceBlank(l_dev_info.model_name);
	
	if (prefix[0] == 0)
	{
		strcpy(prefix, l_dev_info.model_name);
	}

	fprintf(stderr, "ModelName   is %s\n", l_dev_info.model_name);
    fprintf(stderr, "BuildSpec   is %s\n", l_dev_info.build_spec);
	fprintf(stderr, "product_id  is 0x%08x\n", l_dev_info.product_id);
	fprintf(stderr, "product_ver is 0x%08x\n", l_dev_info.product_ver);
	fprintf(stderr, "swRevision  is 0x%08x\n", l_dev_info.sw_revision);
	fprintf(stderr, "platformVer is 0x%08x\n", l_dev_info.platform_ver);
	fprintf(stderr, "addHwVer    is 0x%08x\n", l_dev_info.add_hver);
	fprintf(stderr, "specilaVer  is 0x%08x\n", l_dev_info.special_ver);
	fprintf(stderr, "devModelVer is v%lu\n", strtoul(l_dev_info.dev_ver, 0, 0));
	fprintf(stderr, "buildDate   is %s\n", l_dev_info.build_date);
	fprintf(stderr, "buildTime   is %d\n", l_dev_info.build_time);
    fprintf(stderr, "isBeta      is %d\n", l_dev_info.is_beta);
    fprintf(stderr, "isTrans     is %d\n\n", l_dev_info.is_trans);
	fprintf(stderr, "isDateCode     is %d\n\n", l_dev_info.is_datecode);
	
	fill_buffer(buf, filename, vmlinux_name, flash_size);

	if (l_dev_info.is_datecode==1)
	{
		sprintf(image_name_prefix, "%s/%s(%s)v%d",
					image_path, 
					(prefix[0] == 0) ? l_dev_info.model_name : prefix, 
					l_dev_info.build_spec,
					strtoul(l_dev_info.dev_ver, 0, 0));
	}
	else
	{
		sprintf(image_name_prefix, "%s/%s(%s)v%d_%d.%d.0_%d.%d.%d",
					image_path, prefix, 
                    l_dev_info.build_spec,
					strtoul(l_dev_info.dev_ver, 0, 0),
					(l_dev_info.sw_revision >> 8) & 0xff, 
					l_dev_info.sw_revision & 0xff,
					(l_dev_info.platform_ver >> 16) & 0xff,
					(l_dev_info.platform_ver >> 8) & 0xff,
					l_dev_info.platform_ver & 0xff);
	}
	
    if (l_dev_info.is_datecode==1)
	{
		sprintf(image_name_suffix, "Datecode%s%02u%02u", l_dev_info.build_date, l_dev_info.build_time/3600, (l_dev_info.build_time%3600)/60);
	}
	else
	{
		sprintf(image_name_suffix, "%s", l_dev_info.is_trans? "_trans": (l_dev_info.is_beta? "_beta":""));
	}

	sprintf(bin_file, "%s_flash(%s)%s.bin", image_name_prefix, l_dev_info.build_date, image_name_suffix);
	sprintf(update_file, "%s_up(%s)%s.bin", image_name_prefix, l_dev_info.build_date, image_name_suffix);
	if (l_dev_info.is_datecode==1)
	{
		sprintf(update_file_boot, "%s_%s.bin", image_name_prefix, image_name_suffix);
	}
	else
	{
		sprintf(update_file_boot, "%s_up_boot(%s)%s.bin", image_name_prefix, l_dev_info.build_date, image_name_suffix);
	}
    if (prefix[0] == 0)
    {
	    sprintf(tag_kernel, "%s_tag_kernel(%s).bin", image_name_prefix, l_dev_info.build_date);
    }
    else
    {
        sprintf(tag_kernel, "%s/%s_tag_kernel.bin", image_path, prefix);
    }
	

#if FULL_BUILD
	/* create image */
    if (l_dev_info.is_beta == 0 && l_dev_info.is_trans == 0 && l_dev_info.is_datecode == 0)
    {
	    if (!(out = fopen(bin_file, "w"))) {
	    	fprintf(stderr, "can not open \"%s\" for writing\n", bin_file);
	    	usage();
	    }

	    if (!fwrite(buf + BOOTLOADER_TAG_LEN, flash_size, 1, out) || fflush(out)) {
	    	fprintf(stderr, "fwrite failed\n");
	    	return EXIT_FAILURE;
	    }
	    fclose(out);
    }
	/* end create */

	/* only provide update firmware with bootloader 
	 * yangxv, 2013.05.07
	 */
#if 0
	/* create update firmware without bootloader */
	if (!(out = fopen(update_file, "w"))) {
		fprintf(stderr, "can not open \"%s\" for writing\n", update_file);
		usage();
	}

	if (!fwrite(buf + l_flash_struct.kernelOffset + BOOTLOADER_TAG_LEN, l_flash_struct.configOffset - l_flash_struct.kernelOffset, 1, out) || fflush(out)) {
		fprintf(stderr, "fwrite failed\n");
		return EXIT_FAILURE;
	}

	fclose(out);
	/* end create */
#endif

	/* create update firmware with bootloader */
	if (!(out = fopen(update_file_boot, "w"))) {
		fprintf(stderr, "can not open \"%s\" for writing\n", update_file_boot);
		usage();
	}

	int up_boot_len = l_flash_struct.appSize + (l_flash_struct.kernelOffset - l_flash_struct.bootOffset) + BOOTLOADER_TAG_LEN;

	if (!fwrite(buf, up_boot_len, 1, out) || fflush(out)) {
		fprintf(stderr, "fwrite failed\n");
		return EXIT_FAILURE;
	}

	fclose(out);
	/* end create */

	if (1 == dual_image) 
	{
		sprintf(bin_file, "%s_flash_%dM(%s)%s.bin", image_name_prefix, flash_size/0x100000, l_dev_info.build_date, 
                image_name_suffix);
		
		if (!(out = fopen(bin_file, "w"))) {
			fprintf(stderr, "can not open \"%s\" for writing\n", bin_file);
			usage();
		}

		if (!fwrite(buf + BOOTLOADER_TAG_LEN, flash_size, 1, out) ||
			!fwrite(buf + BOOTLOADER_TAG_LEN, flash_size, 1, out) || fflush(out)) {
			fprintf(stderr, "fwrite failed\n");
			return EXIT_FAILURE;
		}

		fclose(out);
	}

#else
	/* create kernel with tag */
	if (!(out = fopen(tag_kernel, "w"))) {
		fprintf(stderr, "can not open \"%s\" for writing\n", tag_kernel);
		usage();
	}

    if (!fwrite(buf + l_flash_struct.kernelOffset + BOOTLOADER_TAG_LEN, l_flash_struct.rootfsOffset - l_flash_struct.kernelOffset, 1, out) || fflush(out)) {
		fprintf(stderr, "fwrite failed\n");
		return EXIT_FAILURE;
	}
    /*end create */
#endif /* FULL_BUILD */
 
	free(buf);
	
	return EXIT_SUCCESS;
}
