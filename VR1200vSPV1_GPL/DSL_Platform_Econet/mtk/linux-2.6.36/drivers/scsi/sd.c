/*
 *      sd.c Copyright (C) 1992 Drew Eckhardt
 *           Copyright (C) 1993, 1994, 1995, 1999 Eric Youngdale
 *
 *      Linux scsi disk driver
 *              Initial versions: Drew Eckhardt
 *              Subsequent revisions: Eric Youngdale
 *	Modification history:
 *       - Drew Eckhardt <drew@colorado.edu> original
 *       - Eric Youngdale <eric@andante.org> add scatter-gather, multiple 
 *         outstanding request, and other enhancements.
 *         Support loadable low-level scsi drivers.
 *       - Jirka Hanika <geo@ff.cuni.cz> support more scsi disks using 
 *         eight major numbers.
 *       - Richard Gooch <rgooch@atnf.csiro.au> support devfs.
 *	 - Torben Mathiasen <tmm@image.dk> Resource allocation fixes in 
 *	   sd_init and cleanups.
 *	 - Alex Davis <letmein@erols.com> Fix problem where partition info
 *	   not being read in sd_open. Fix problem where removable media 
 *	   could be ejected after sd_open.
 *	 - Douglas Gilbert <dgilbert@interlog.com> cleanup for lk 2.5.x
 *	 - Badari Pulavarty <pbadari@us.ibm.com>, Matthew Wilcox 
 *	   <willy@debian.org>, Kurt Garloff <garloff@suse.de>: 
 *	   Support 32k/1M disks.
 *
 *	Logging policy (needs CONFIG_SCSI_LOGGING defined):
 *	 - setting up transfer: SCSI_LOG_HLQUEUE levels 1 and 2
 *	 - end of transfer (bh + scsi_lib): SCSI_LOG_HLCOMPLETE level 1
 *	 - entering sd_ioctl: SCSI_LOG_IOCTL level 1
 *	 - entering other commands: SCSI_LOG_HLQUEUE level 3
 *	Note: when the logging level is set by the user, it must be greater
 *	than the level indicated above to trigger output.	
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/errno.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/delay.h>
#include <linux/smp_lock.h>
#include <linux/mutex.h>
#include <linux/string_helpers.h>
#include <linux/async.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_driver.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/scsicam.h>

#include "sd.h"
#include "scsi_logging.h"
#ifdef USB_AUTOMOUNT
#include <linux/proc_fs.h>
#include <linux/sched.h>
struct mutex signal_mutex;
struct task_struct *mount_task_p;
#endif
#ifdef TCSUPPORT_USB_HOST_LED
#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7520) && !defined(TCSUPPORT_CPU_MT7505)
extern unsigned int usb_dev_connected ; //usb led support
#endif
#endif


MODULE_AUTHOR("Eric Youngdale");
MODULE_DESCRIPTION("SCSI disk (sd) driver");
MODULE_LICENSE("GPL");

MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK0_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK1_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK2_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK3_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK4_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK5_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK6_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK7_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK8_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK9_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK10_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK11_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK12_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK13_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK14_MAJOR);
MODULE_ALIAS_BLOCKDEV_MAJOR(SCSI_DISK15_MAJOR);
MODULE_ALIAS_SCSI_DEVICE(TYPE_DISK);
MODULE_ALIAS_SCSI_DEVICE(TYPE_MOD);
MODULE_ALIAS_SCSI_DEVICE(TYPE_RBC);

#if !defined(CONFIG_DEBUG_BLOCK_EXT_DEVT)
#define SD_MINORS	16
#else
#define SD_MINORS	0
#endif

static int  sd_revalidate_disk(struct gendisk *);
static void sd_unlock_native_capacity(struct gendisk *disk);
static int  sd_probe(struct device *);
static int  sd_remove(struct device *);
static void sd_shutdown(struct device *);
static int sd_suspend(struct device *, pm_message_t state);
static int sd_resume(struct device *);
static void sd_rescan(struct device *);
static int sd_done(struct scsi_cmnd *);
static void sd_read_capacity(struct scsi_disk *sdkp, unsigned char *buffer);
static void scsi_disk_release(struct device *cdev);
static void sd_print_sense_hdr(struct scsi_disk *, struct scsi_sense_hdr *);
static void sd_print_result(struct scsi_disk *, int);

static DEFINE_SPINLOCK(sd_index_lock);
static DEFINE_IDA(sd_index_ida);

/* This semaphore is used to mediate the 0->1 reference get in the
 * face of object destruction (i.e. we can't allow a get on an
 * object after last put) */
static DEFINE_MUTEX(sd_ref_mutex);

static struct kmem_cache *sd_cdb_cache;
static mempool_t *sd_cdb_pool;

static const char *sd_cache_types[] = {
	"write through", "none", "write back",
	"write back, no read (daft)"
};

#ifdef USB_AUTOMOUNT
/*---------------------------------------------------------------------
 ** function mame: automount_pid_write_proc
 *
 ** description:
 *      1. receives AutoMount's PID from user space and turns it into
 *         a task_struct pointer which will be used to send signals to
 *         AutoMount later. 
 *
 ** parameters:
 *      1. buffer: the pointer pointing to the string received from user
 *      2. count: the length of the string in buffer
 *
 ** global:
 *      1. mount_task_p: the pointer pointing to the task_struct structure
 *          that is going to be used to send signals to AutoMount
 *
 ** return: int
 *
 ** call: none
 *
 ** revision:
 *      1. Trey 2010/08/16
 *---------------------------------------------------------------------*/
static int automount_pid_write_proc(struct file *file, const char *buffer,
    unsigned long count, void *data)
{
    char str_pid[16];
    pid_t mount_pid = 0;
    char *cp;
     
    if (copy_from_user(str_pid, buffer, count)){
        printk("automount_pid_write_proc failed\n");
        return -EFAULT;
    }
     
    str_pid[count] = '\0';
     
     //atoi func
    for (cp = str_pid; *cp != '\0'; cp++) {
        switch (*cp) {
        case '0' ... '9':
            mount_pid = 10*mount_pid+(*cp-'0');
            break;
        default:
            break;
        }
    }
    
    mount_task_p = find_task_by_vpid(mount_pid);

    return 0;
}
#endif

static ssize_t
sd_store_cache_type(struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count)
{
	int i, ct = -1, rcd, wce, sp;
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;
	char buffer[64];
	char *buffer_data;
	struct scsi_mode_data data;
	struct scsi_sense_hdr sshdr;
	int len;

	if (sdp->type != TYPE_DISK)
		/* no cache control on RBC devices; theoretically they
		 * can do it, but there's probably so many exceptions
		 * it's not worth the risk */
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(sd_cache_types); i++) {
		len = strlen(sd_cache_types[i]);
		if (strncmp(sd_cache_types[i], buf, len) == 0 &&
		    buf[len] == '\n') {
			ct = i;
			break;
		}
	}
	if (ct < 0)
		return -EINVAL;
	rcd = ct & 0x01 ? 1 : 0;
	wce = ct & 0x02 ? 1 : 0;
	if (scsi_mode_sense(sdp, 0x08, 8, buffer, sizeof(buffer), SD_TIMEOUT,
			    SD_MAX_RETRIES, &data, NULL))
		return -EINVAL;
	len = min_t(size_t, sizeof(buffer), data.length - data.header_length -
		  data.block_descriptor_length);
	buffer_data = buffer + data.header_length +
		data.block_descriptor_length;
	buffer_data[2] &= ~0x05;
	buffer_data[2] |= wce << 2 | rcd;
	sp = buffer_data[0] & 0x80 ? 1 : 0;

	if (scsi_mode_select(sdp, 1, sp, 8, buffer_data, len, SD_TIMEOUT,
			     SD_MAX_RETRIES, &data, &sshdr)) {
		if (scsi_sense_valid(&sshdr))
			sd_print_sense_hdr(sdkp, &sshdr);
		return -EINVAL;
	}
	revalidate_disk(sdkp->disk);
	return count;
}

static ssize_t
sd_store_manage_start_stop(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	sdp->manage_start_stop = simple_strtoul(buf, NULL, 10);

	return count;
}

static ssize_t
sd_store_allow_restart(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	if (sdp->type != TYPE_DISK)
		return -EINVAL;

	sdp->allow_restart = simple_strtoul(buf, NULL, 10);

	return count;
}

static ssize_t
sd_show_cache_type(struct device *dev, struct device_attribute *attr,
		   char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	int ct = sdkp->RCD + 2*sdkp->WCE;

	return snprintf(buf, 40, "%s\n", sd_cache_types[ct]);
}

static ssize_t
sd_show_fua(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 20, "%u\n", sdkp->DPOFUA);
}

static ssize_t
sd_show_manage_start_stop(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct scsi_device *sdp = sdkp->device;

	return snprintf(buf, 20, "%u\n", sdp->manage_start_stop);
}

static ssize_t
sd_show_allow_restart(struct device *dev, struct device_attribute *attr,
		      char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 40, "%d\n", sdkp->device->allow_restart);
}

static ssize_t
sd_show_protection_type(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 20, "%u\n", sdkp->protection_type);
}

static ssize_t
sd_show_app_tag_own(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 20, "%u\n", sdkp->ATO);
}

static ssize_t
sd_show_thin_provisioning(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);

	return snprintf(buf, 20, "%u\n", sdkp->thin_provisioning);
}

static struct device_attribute sd_disk_attrs[] = {
	__ATTR(cache_type, S_IRUGO|S_IWUSR, sd_show_cache_type,
	       sd_store_cache_type),
	__ATTR(FUA, S_IRUGO, sd_show_fua, NULL),
	__ATTR(allow_restart, S_IRUGO|S_IWUSR, sd_show_allow_restart,
	       sd_store_allow_restart),
	__ATTR(manage_start_stop, S_IRUGO|S_IWUSR, sd_show_manage_start_stop,
	       sd_store_manage_start_stop),
	__ATTR(protection_type, S_IRUGO, sd_show_protection_type, NULL),
	__ATTR(app_tag_own, S_IRUGO, sd_show_app_tag_own, NULL),
	__ATTR(thin_provisioning, S_IRUGO, sd_show_thin_provisioning, NULL),
	__ATTR_NULL,
};

static struct class sd_disk_class = {
	.name		= "scsi_disk",
	.owner		= THIS_MODULE,
	.dev_release	= scsi_disk_release,
	.dev_attrs	= sd_disk_attrs,
};

static struct scsi_driver sd_template = {
	.owner			= THIS_MODULE,
	.gendrv = {
		.name		= "sd",
		.probe		= sd_probe,
		.remove		= sd_remove,
		.suspend	= sd_suspend,
		.resume		= sd_resume,
		.shutdown	= sd_shutdown,
	},
	.rescan			= sd_rescan,
	.done			= sd_done,
};

/*
 * Device no to disk mapping:
 * 
 *       major         disc2     disc  p1
 *   |............|.............|....|....| <- dev_t
 *    31        20 19          8 7  4 3  0
 * 
 * Inside a major, we have 16k disks, however mapped non-
 * contiguously. The first 16 disks are for major0, the next
 * ones with major1, ... Disk 256 is for major0 again, disk 272 
 * for major1, ... 
 * As we stay compatible with our numbering scheme, we can reuse 
 * the well-know SCSI majors 8, 65--71, 136--143.
 */
static int sd_major(int major_idx)
{
	switch (major_idx) {
	case 0:
		return SCSI_DISK0_MAJOR;
	case 1 ... 7:
		return SCSI_DISK1_MAJOR + major_idx - 1;
	case 8 ... 15:
		return SCSI_DISK8_MAJOR + major_idx - 8;
	default:
		BUG();
		return 0;	/* shut up gcc */
	}
}

static struct scsi_disk *__scsi_disk_get(struct gendisk *disk)
{
	struct scsi_disk *sdkp = NULL;

	if (disk->private_data) {
		sdkp = scsi_disk(disk);
		if (scsi_device_get(sdkp->device) == 0)
			get_device(&sdkp->dev);
		else
			sdkp = NULL;
	}
	return sdkp;
}

static struct scsi_disk *scsi_disk_get(struct gendisk *disk)
{
	struct scsi_disk *sdkp;

	mutex_lock(&sd_ref_mutex);
	sdkp = __scsi_disk_get(disk);
	mutex_unlock(&sd_ref_mutex);
	return sdkp;
}

static struct scsi_disk *scsi_disk_get_from_dev(struct device *dev)
{
	struct scsi_disk *sdkp;

	mutex_lock(&sd_ref_mutex);
	sdkp = dev_get_drvdata(dev);
	if (sdkp)
		sdkp = __scsi_disk_get(sdkp->disk);
	mutex_unlock(&sd_ref_mutex);
	return sdkp;
}

static void scsi_disk_put(struct scsi_disk *sdkp)
{
	struct scsi_device *sdev = sdkp->device;

	mutex_lock(&sd_ref_mutex);
	put_device(&sdkp->dev);
	scsi_device_put(sdev);
	mutex_unlock(&sd_ref_mutex);
}

static void sd_prot_op(struct scsi_cmnd *scmd, unsigned int dif)
{
	unsigned int prot_op = SCSI_PROT_NORMAL;
	unsigned int dix = scsi_prot_sg_count(scmd);

	if (scmd->sc_data_direction == DMA_FROM_DEVICE) {
		if (dif && dix)
			prot_op = SCSI_PROT_READ_PASS;
		else if (dif && !dix)
			prot_op = SCSI_PROT_READ_STRIP;
		else if (!dif && dix)
			prot_op = SCSI_PROT_READ_INSERT;
	} else {
		if (dif && dix)
			prot_op = SCSI_PROT_WRITE_PASS;
		else if (dif && !dix)
			prot_op = SCSI_PROT_WRITE_INSERT;
		else if (!dif && dix)
			prot_op = SCSI_PROT_WRITE_STRIP;
	}

	scsi_set_prot_op(scmd, prot_op);
	scsi_set_prot_type(scmd, dif);
}

/**
 * scsi_setup_discard_cmnd - unmap blocks on thinly provisioned device
 * @sdp: scsi device to operate one
 * @rq: Request to prepare
 *
 * Will issue either UNMAP or WRITE SAME(16) depending on preference
 * indicated by target device.
 **/
static int scsi_setup_discard_cmnd(struct scsi_device *sdp, struct request *rq)
{
	struct scsi_disk *sdkp = scsi_disk(rq->rq_disk);
	struct bio *bio = rq->bio;
	sector_t sector = bio->bi_sector;
	unsigned int nr_sectors = bio_sectors(bio);
	unsigned int len;
	int ret;
	struct page *page;

	if (sdkp->device->sector_size == 4096) {
		sector >>= 3;
		nr_sectors >>= 3;
	}

	rq->timeout = SD_TIMEOUT;

	memset(rq->cmd, 0, rq->cmd_len);

	page = alloc_page(GFP_ATOMIC | __GFP_ZERO);
	if (!page)
		return BLKPREP_DEFER;

	if (sdkp->unmap) {
		char *buf = page_address(page);

		rq->cmd_len = 10;
		rq->cmd[0] = UNMAP;
		rq->cmd[8] = 24;

		put_unaligned_be16(6 + 16, &buf[0]);
		put_unaligned_be16(16, &buf[2]);
		put_unaligned_be64(sector, &buf[8]);
		put_unaligned_be32(nr_sectors, &buf[16]);

		len = 24;
	} else {
		rq->cmd_len = 16;
		rq->cmd[0] = WRITE_SAME_16;
		rq->cmd[1] = 0x8; /* UNMAP */
		put_unaligned_be64(sector, &rq->cmd[2]);
		put_unaligned_be32(nr_sectors, &rq->cmd[10]);

		len = sdkp->device->sector_size;
	}

	blk_add_request_payload(rq, page, len);
	ret = scsi_setup_blk_pc_cmnd(sdp, rq);
	rq->buffer = page_address(page);
	if (ret != BLKPREP_OK) {
		__free_page(page);
		rq->buffer = NULL;
	}
	return ret;
}

static int scsi_setup_flush_cmnd(struct scsi_device *sdp, struct request *rq)
{
	rq->timeout = SD_TIMEOUT;
	rq->retries = SD_MAX_RETRIES;
	rq->cmd[0] = SYNCHRONIZE_CACHE;
	rq->cmd_len = 10;

	return scsi_setup_blk_pc_cmnd(sdp, rq);
}

static void sd_unprep_fn(struct request_queue *q, struct request *rq)
{
	if (rq->cmd_flags & REQ_DISCARD) {
		free_page((unsigned long)rq->buffer);
		rq->buffer = NULL;
	}
}

/**
 *	sd_init_command - build a scsi (read or write) command from
 *	information in the request structure.
 *	@SCpnt: pointer to mid-level's per scsi command structure that
 *	contains request and into which the scsi command is written
 *
 *	Returns 1 if successful and 0 if error (or cannot be done now).
 **/
static int sd_prep_fn(struct request_queue *q, struct request *rq)
{
	struct scsi_cmnd *SCpnt;
	struct scsi_device *sdp = q->queuedata;
	struct gendisk *disk = rq->rq_disk;
	struct scsi_disk *sdkp;
	sector_t block = blk_rq_pos(rq);
	sector_t threshold;
	unsigned int this_count = blk_rq_sectors(rq);
	int ret, host_dif;
	unsigned char protect;

	/*
	 * Discard request come in as REQ_TYPE_FS but we turn them into
	 * block PC requests to make life easier.
	 */
	if (rq->cmd_flags & REQ_DISCARD) {
		ret = scsi_setup_discard_cmnd(sdp, rq);
		goto out;
	} else if (rq->cmd_flags & REQ_FLUSH) {
		ret = scsi_setup_flush_cmnd(sdp, rq);
		goto out;
	} else if (rq->cmd_type == REQ_TYPE_BLOCK_PC) {
		ret = scsi_setup_blk_pc_cmnd(sdp, rq);
		goto out;
	} else if (rq->cmd_type != REQ_TYPE_FS) {
		ret = BLKPREP_KILL;
		goto out;
	}
	ret = scsi_setup_fs_cmnd(sdp, rq);
	if (ret != BLKPREP_OK)
		goto out;
	SCpnt = rq->special;
	sdkp = scsi_disk(disk);

	/* from here on until we're complete, any goto out
	 * is used for a killable error condition */
	ret = BLKPREP_KILL;

	SCSI_LOG_HLQUEUE(1, scmd_printk(KERN_INFO, SCpnt,
					"sd_init_command: block=%llu, "
					"count=%d\n",
					(unsigned long long)block,
					this_count));

	if (!sdp || !scsi_device_online(sdp) ||
	    block + blk_rq_sectors(rq) > get_capacity(disk)) {
		SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt,
						"Finishing %u sectors\n",
						blk_rq_sectors(rq)));
		SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt,
						"Retry with 0x%p\n", SCpnt));
		goto out;
	}

	if (sdp->changed) {
		/*
		 * quietly refuse to do anything to a changed disc until 
		 * the changed bit has been reset
		 */
		/* printk("SCSI disk has been changed. Prohibiting further I/O.\n"); */
		goto out;
	}

	/*
	 * Some SD card readers can't handle multi-sector accesses which touch
	 * the last one or two hardware sectors.  Split accesses as needed.
	 */
	threshold = get_capacity(disk) - SD_LAST_BUGGY_SECTORS *
		(sdp->sector_size / 512);

	if (unlikely(sdp->last_sector_bug && block + this_count > threshold)) {
		if (block < threshold) {
			/* Access up to the threshold but not beyond */
			this_count = threshold - block;
		} else {
			/* Access only a single hardware sector */
			this_count = sdp->sector_size / 512;
		}
	}

	SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt, "block=%llu\n",
					(unsigned long long)block));

	/*
	 * If we have a 1K hardware sectorsize, prevent access to single
	 * 512 byte sectors.  In theory we could handle this - in fact
	 * the scsi cdrom driver must be able to handle this because
	 * we typically use 1K blocksizes, and cdroms typically have
	 * 2K hardware sectorsizes.  Of course, things are simpler
	 * with the cdrom, since it is read-only.  For performance
	 * reasons, the filesystems should be able to handle this
	 * and not force the scsi disk driver to use bounce buffers
	 * for this.
	 */
	if (sdp->sector_size == 1024) {
		if ((block & 1) || (blk_rq_sectors(rq) & 1)) {
			scmd_printk(KERN_ERR, SCpnt,
				    "Bad block number requested\n");
			goto out;
		} else {
			block = block >> 1;
			this_count = this_count >> 1;
		}
	}
	if (sdp->sector_size == 2048) {
		if ((block & 3) || (blk_rq_sectors(rq) & 3)) {
			scmd_printk(KERN_ERR, SCpnt,
				    "Bad block number requested\n");
			goto out;
		} else {
			block = block >> 2;
			this_count = this_count >> 2;
		}
	}
	if (sdp->sector_size == 4096) {
		if ((block & 7) || (blk_rq_sectors(rq) & 7)) {
			scmd_printk(KERN_ERR, SCpnt,
				    "Bad block number requested\n");
			goto out;
		} else {
			block = block >> 3;
			this_count = this_count >> 3;
		}
	}
	if (rq_data_dir(rq) == WRITE) {
		if (!sdp->writeable) {
			goto out;
		}
		SCpnt->cmnd[0] = WRITE_6;
		SCpnt->sc_data_direction = DMA_TO_DEVICE;

		if (blk_integrity_rq(rq) &&
		    sd_dif_prepare(rq, block, sdp->sector_size) == -EIO)
			goto out;

	} else if (rq_data_dir(rq) == READ) {
		SCpnt->cmnd[0] = READ_6;
		SCpnt->sc_data_direction = DMA_FROM_DEVICE;
	} else {
		scmd_printk(KERN_ERR, SCpnt, "Unknown command %x\n", rq->cmd_flags);
		goto out;
	}

	SCSI_LOG_HLQUEUE(2, scmd_printk(KERN_INFO, SCpnt,
					"%s %d/%u 512 byte blocks.\n",
					(rq_data_dir(rq) == WRITE) ?
					"writing" : "reading", this_count,
					blk_rq_sectors(rq)));

	/* Set RDPROTECT/WRPROTECT if disk is formatted with DIF */
	host_dif = scsi_host_dif_capable(sdp->host, sdkp->protection_type);
	if (host_dif)
		protect = 1 << 5;
	else
		protect = 0;

	if (host_dif == SD_DIF_TYPE2_PROTECTION) {
		SCpnt->cmnd = mempool_alloc(sd_cdb_pool, GFP_ATOMIC);

		if (unlikely(SCpnt->cmnd == NULL)) {
			ret = BLKPREP_DEFER;
			goto out;
		}

		SCpnt->cmd_len = SD_EXT_CDB_SIZE;
		memset(SCpnt->cmnd, 0, SCpnt->cmd_len);
		SCpnt->cmnd[0] = VARIABLE_LENGTH_CMD;
		SCpnt->cmnd[7] = 0x18;
		SCpnt->cmnd[9] = (rq_data_dir(rq) == READ) ? READ_32 : WRITE_32;
		SCpnt->cmnd[10] = protect | ((rq->cmd_flags & REQ_FUA) ? 0x8 : 0);

		/* LBA */
		SCpnt->cmnd[12] = sizeof(block) > 4 ? (unsigned char) (block >> 56) & 0xff : 0;
		SCpnt->cmnd[13] = sizeof(block) > 4 ? (unsigned char) (block >> 48) & 0xff : 0;
		SCpnt->cmnd[14] = sizeof(block) > 4 ? (unsigned char) (block >> 40) & 0xff : 0;
		SCpnt->cmnd[15] = sizeof(block) > 4 ? (unsigned char) (block >> 32) & 0xff : 0;
		SCpnt->cmnd[16] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[17] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[18] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[19] = (unsigned char) block & 0xff;

		/* Expected Indirect LBA */
		SCpnt->cmnd[20] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[21] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[22] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[23] = (unsigned char) block & 0xff;

		/* Transfer length */
		SCpnt->cmnd[28] = (unsigned char) (this_count >> 24) & 0xff;
		SCpnt->cmnd[29] = (unsigned char) (this_count >> 16) & 0xff;
		SCpnt->cmnd[30] = (unsigned char) (this_count >> 8) & 0xff;
		SCpnt->cmnd[31] = (unsigned char) this_count & 0xff;
	} else if (block > 0xffffffff) {
		SCpnt->cmnd[0] += READ_16 - READ_6;
		SCpnt->cmnd[1] = protect | ((rq->cmd_flags & REQ_FUA) ? 0x8 : 0);
		SCpnt->cmnd[2] = sizeof(block) > 4 ? (unsigned char) (block >> 56) & 0xff : 0;
		SCpnt->cmnd[3] = sizeof(block) > 4 ? (unsigned char) (block >> 48) & 0xff : 0;
		SCpnt->cmnd[4] = sizeof(block) > 4 ? (unsigned char) (block >> 40) & 0xff : 0;
		SCpnt->cmnd[5] = sizeof(block) > 4 ? (unsigned char) (block >> 32) & 0xff : 0;
		SCpnt->cmnd[6] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[7] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[8] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[9] = (unsigned char) block & 0xff;
		SCpnt->cmnd[10] = (unsigned char) (this_count >> 24) & 0xff;
		SCpnt->cmnd[11] = (unsigned char) (this_count >> 16) & 0xff;
		SCpnt->cmnd[12] = (unsigned char) (this_count >> 8) & 0xff;
		SCpnt->cmnd[13] = (unsigned char) this_count & 0xff;
		SCpnt->cmnd[14] = SCpnt->cmnd[15] = 0;
	} else if ((this_count > 0xff) || (block > 0x1fffff) ||
		   scsi_device_protection(SCpnt->device) ||
		   SCpnt->device->use_10_for_rw) {
		if (this_count > 0xffff)
			this_count = 0xffff;

		SCpnt->cmnd[0] += READ_10 - READ_6;
		SCpnt->cmnd[1] = protect | ((rq->cmd_flags & REQ_FUA) ? 0x8 : 0);
		SCpnt->cmnd[2] = (unsigned char) (block >> 24) & 0xff;
		SCpnt->cmnd[3] = (unsigned char) (block >> 16) & 0xff;
		SCpnt->cmnd[4] = (unsigned char) (block >> 8) & 0xff;
		SCpnt->cmnd[5] = (unsigned char) block & 0xff;
		SCpnt->cmnd[6] = SCpnt->cmnd[9] = 0;
		SCpnt->cmnd[7] = (unsigned char) (this_count >> 8) & 0xff;
		SCpnt->cmnd[8] = (unsigned char) this_count & 0xff;
	} else {
		if (unlikely(rq->cmd_flags & REQ_FUA)) {
			/*
			 * This happens only if this drive failed
			 * 10byte rw command with ILLEGAL_REQUEST
			 * during operation and thus turned off
			 * use_10_for_rw.
			 */
			scmd_printk(KERN_ERR, SCpnt,
				    "FUA write on READ/WRITE(6) drive\n");
			goto out;
		}

		SCpnt->cmnd[1] |= (unsigned char) ((block >> 16) & 0x1f);
		SCpnt->cmnd[2] = (unsigned char) ((block >> 8) & 0xff);
		SCpnt->cmnd[3] = (unsigned char) block & 0xff;
		SCpnt->cmnd[4] = (unsigned char) this_count;
		SCpnt->cmnd[5] = 0;
	}
	SCpnt->sdb.length = this_count * sdp->sector_size;

	/* If DIF or DIX is enabled, tell HBA how to handle request */
	if (host_dif || scsi_prot_sg_count(SCpnt))
		sd_prot_op(SCpnt, host_dif);

	/*
	 * We shouldn't disconnect in the middle of a sector, so with a dumb
	 * host adapter, it's safe to assume that we can at least transfer
	 * this many bytes between each connect / disconnect.
	 */
	SCpnt->transfersize = sdp->sector_size;
	SCpnt->underflow = this_count << 9;
	SCpnt->allowed = SD_MAX_RETRIES;

	/*
	 * This indicates that the command is ready from our end to be
	 * queued.
	 */
	ret = BLKPREP_OK;
 out:
	return scsi_prep_return(q, rq, ret);
}

/**
 *	sd_open - open a scsi disk device
 *	@inode: only i_rdev member may be used
 *	@filp: only f_mode and f_flags may be used
 *
 *	Returns 0 if successful. Returns a negated errno value in case 
 *	of error.
 *
 *	Note: This can be called from a user context (e.g. fsck(1) )
 *	or from within the kernel (e.g. as a result of a mount(1) ).
 *	In the latter case @inode and @filp carry an abridged amount
 *	of information as noted above.
 *
 *	Locking: called with bdev->bd_mutex held.
 **/
static int sd_open(struct block_device *bdev, fmode_t mode)
{
	struct scsi_disk *sdkp = scsi_disk_get(bdev->bd_disk);
	struct scsi_device *sdev;
	int retval;

	if (!sdkp)
		return -ENXIO;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp, "sd_open\n"));

	sdev = sdkp->device;

	retval = scsi_autopm_get_device(sdev);
	if (retval)
		goto error_autopm;

	/*
	 * If the device is in error recovery, wait until it is done.
	 * If the device is offline, then disallow any access to it.
	 */
	retval = -ENXIO;
	if (!scsi_block_when_processing_errors(sdev))
		goto error_out;

	if (sdev->removable || sdkp->write_prot)
		check_disk_change(bdev);

	/*
	 * If the drive is empty, just let the open fail.
	 */
	retval = -ENOMEDIUM;
	if (sdev->removable && !sdkp->media_present && !(mode & FMODE_NDELAY))
		goto error_out;

	/*
	 * If the device has the write protect tab set, have the open fail
	 * if the user expects to be able to write to the thing.
	 */
	retval = -EROFS;
	if (sdkp->write_prot && (mode & FMODE_WRITE))
		goto error_out;

	/*
	 * It is possible that the disk changing stuff resulted in
	 * the device being taken offline.  If this is the case,
	 * report this to the user, and don't pretend that the
	 * open actually succeeded.
	 */
	retval = -ENXIO;
	if (!scsi_device_online(sdev))
		goto error_out;

	if ((atomic_inc_return(&sdkp->openers) == 1) && sdev->removable) {
		if (scsi_block_when_processing_errors(sdev))
			scsi_set_medium_removal(sdev, SCSI_REMOVAL_PREVENT);
	}

	return 0;

error_out:
	scsi_autopm_put_device(sdev);
error_autopm:
	scsi_disk_put(sdkp);
	return retval;	
}

/**
 *	sd_release - invoked when the (last) close(2) is called on this
 *	scsi disk.
 *	@inode: only i_rdev member may be used
 *	@filp: only f_mode and f_flags may be used
 *
 *	Returns 0. 
 *
 *	Note: may block (uninterruptible) if error recovery is underway
 *	on this disk.
 *
 *	Locking: called with bdev->bd_mutex held.
 **/
static int sd_release(struct gendisk *disk, fmode_t mode)
{
	struct scsi_disk *sdkp = scsi_disk(disk);
	struct scsi_device *sdev = sdkp->device;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp, "sd_release\n"));

	if (atomic_dec_return(&sdkp->openers) == 0 && sdev->removable) {
		if (scsi_block_when_processing_errors(sdev))
			scsi_set_medium_removal(sdev, SCSI_REMOVAL_ALLOW);
	}

	/*
	 * XXX and what if there are packets in flight and this close()
	 * XXX is followed by a "rmmod sd_mod"?
	 */

	scsi_autopm_put_device(sdev);
	scsi_disk_put(sdkp);
	return 0;
}

static int sd_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct scsi_disk *sdkp = scsi_disk(bdev->bd_disk);
	struct scsi_device *sdp = sdkp->device;
	struct Scsi_Host *host = sdp->host;
	int diskinfo[4];

	/* default to most commonly used values */
        diskinfo[0] = 0x40;	/* 1 << 6 */
       	diskinfo[1] = 0x20;	/* 1 << 5 */
       	diskinfo[2] = sdkp->capacity >> 11;
	
	/* override with calculated, extended default, or driver values */
	if (host->hostt->bios_param)
		host->hostt->bios_param(sdp, bdev, sdkp->capacity, diskinfo);
	else
		scsicam_bios_param(bdev, sdkp->capacity, diskinfo);

	geo->heads = diskinfo[0];
	geo->sectors = diskinfo[1];
	geo->cylinders = diskinfo[2];
	return 0;
}

/**
 *	sd_ioctl - process an ioctl
 *	@inode: only i_rdev/i_bdev members may be used
 *	@filp: only f_mode and f_flags may be used
 *	@cmd: ioctl command number
 *	@arg: this is third argument given to ioctl(2) system call.
 *	Often contains a pointer.
 *
 *	Returns 0 if successful (some ioctls return postive numbers on
 *	success as well). Returns a negated errno value in case of error.
 *
 *	Note: most ioctls are forward onto the block subsystem or further
 *	down in the scsi subsystem.
 **/
static int sd_ioctl(struct block_device *bdev, fmode_t mode,
		    unsigned int cmd, unsigned long arg)
{
	struct gendisk *disk = bdev->bd_disk;
	struct scsi_device *sdp = scsi_disk(disk)->device;
	void __user *p = (void __user *)arg;
	int error;
    
	SCSI_LOG_IOCTL(1, printk("sd_ioctl: disk=%s, cmd=0x%x\n",
						disk->disk_name, cmd));

	/*
	 * If we are in the middle of error recovery, don't let anyone
	 * else try and use this device.  Also, if error recovery fails, it
	 * may try and take the device offline, in which case all further
	 * access to the device is prohibited.
	 */
	error = scsi_nonblockable_ioctl(sdp, cmd, p,
					(mode & FMODE_NDELAY) != 0);
	if (!scsi_block_when_processing_errors(sdp) || !error)
		goto out;

	/*
	 * Send SCSI addressing ioctls directly to mid level, send other
	 * ioctls to block level and then onto mid level if they can't be
	 * resolved.
	 */
	switch (cmd) {
		case SCSI_IOCTL_GET_IDLUN:
		case SCSI_IOCTL_GET_BUS_NUMBER:
			error = scsi_ioctl(sdp, cmd, p);
			break;
		default:
			error = scsi_cmd_ioctl(disk->queue, disk, mode, cmd, p);
			if (error != -ENOTTY)
				break;
			error = scsi_ioctl(sdp, cmd, p);
			break;
	}
out:
	return error;
}

static void set_media_not_present(struct scsi_disk *sdkp)
{
	sdkp->media_present = 0;
	sdkp->capacity = 0;
	sdkp->device->changed = 1;
}

/**
 *	sd_media_changed - check if our medium changed
 *	@disk: kernel device descriptor 
 *
 *	Returns 0 if not applicable or no change; 1 if change
 *
 *	Note: this function is invoked from the block subsystem.
 **/
static int sd_media_changed(struct gendisk *disk)
{
	struct scsi_disk *sdkp = scsi_disk(disk);
	struct scsi_device *sdp = sdkp->device;
	struct scsi_sense_hdr *sshdr = NULL;
	int retval;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp, "sd_media_changed\n"));

	if (!sdp->removable)
		return 0;

	/*
	 * If the device is offline, don't send any commands - just pretend as
	 * if the command failed.  If the device ever comes back online, we
	 * can deal with it then.  It is only because of unrecoverable errors
	 * that we would ever take a device offline in the first place.
	 */
	if (!scsi_device_online(sdp)) {
		set_media_not_present(sdkp);
		retval = 1;
		goto out;
	}

	/*
	 * Using TEST_UNIT_READY enables differentiation between drive with
	 * no cartridge loaded - NOT READY, drive with changed cartridge -
	 * UNIT ATTENTION, or with same cartridge - GOOD STATUS.
	 *
	 * Drives that auto spin down. eg iomega jaz 1G, will be started
	 * by sd_spinup_disk() from sd_revalidate_disk(), which happens whenever
	 * sd_revalidate() is called.
	 */
	retval = -ENODEV;

	if (scsi_block_when_processing_errors(sdp)) {
		sshdr  = kzalloc(sizeof(*sshdr), GFP_KERNEL);
		retval = scsi_test_unit_ready(sdp, SD_TIMEOUT, SD_MAX_RETRIES,
					      sshdr);
	}

	/*
	 * Unable to test, unit probably not ready.   This usually
	 * means there is no disc in the drive.  Mark as changed,
	 * and we will figure it out later once the drive is
	 * available again.
	 */
	if (retval || (scsi_sense_valid(sshdr) &&
		       /* 0x3a is medium not present */
		       sshdr->asc == 0x3a)) {
		set_media_not_present(sdkp);
		retval = 1;
		goto out;
	}

	/*
	 * For removable scsi disk we have to recognise the presence
	 * of a disk in the drive. This is kept in the struct scsi_disk
	 * struct and tested at open !  Daniel Roche (dan@lectra.fr)
	 */
	sdkp->media_present = 1;

	retval = sdp->changed;
	sdp->changed = 0;
out:
	if (retval != sdkp->previous_state)
		sdev_evt_send_simple(sdp, SDEV_EVT_MEDIA_CHANGE, GFP_KERNEL);
	sdkp->previous_state = retval;
	kfree(sshdr);
	return retval;
}

static int sd_sync_cache(struct scsi_disk *sdkp)
{
	int retries, res;
	struct scsi_device *sdp = sdkp->device;
	struct scsi_sense_hdr sshdr;

	if (!scsi_device_online(sdp))
		return -ENODEV;


	for (retries = 3; retries > 0; --retries) {
		unsigned char cmd[10] = { 0 };

		cmd[0] = SYNCHRONIZE_CACHE;
		/*
		 * Leave the rest of the command zero to indicate
		 * flush everything.
		 */
		res = scsi_execute_req(sdp, cmd, DMA_NONE, NULL, 0, &sshdr,
				       SD_TIMEOUT, SD_MAX_RETRIES, NULL);
		if (res == 0)
			break;
	}

	if (res) {
		sd_print_result(sdkp, res);
		if (driver_byte(res) & DRIVER_SENSE)
			sd_print_sense_hdr(sdkp, &sshdr);
	}

	if (res)
		return -EIO;
	return 0;
}

static void sd_rescan(struct device *dev)
{
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);

	if (sdkp) {
		revalidate_disk(sdkp->disk);
		scsi_disk_put(sdkp);
	}
}


#ifdef CONFIG_COMPAT
/* 
 * This gets directly called from VFS. When the ioctl 
 * is not recognized we go back to the other translation paths. 
 */
static int sd_compat_ioctl(struct block_device *bdev, fmode_t mode,
			   unsigned int cmd, unsigned long arg)
{
	struct scsi_device *sdev = scsi_disk(bdev->bd_disk)->device;

	/*
	 * If we are in the middle of error recovery, don't let anyone
	 * else try and use this device.  Also, if error recovery fails, it
	 * may try and take the device offline, in which case all further
	 * access to the device is prohibited.
	 */
	if (!scsi_block_when_processing_errors(sdev))
		return -ENODEV;
	       
	if (sdev->host->hostt->compat_ioctl) {
		int ret;

		ret = sdev->host->hostt->compat_ioctl(sdev, cmd, (void __user *)arg);

		return ret;
	}

	/* 
	 * Let the static ioctl translation table take care of it.
	 */
	return -ENOIOCTLCMD; 
}
#endif

static const struct block_device_operations sd_fops = {
	.owner			= THIS_MODULE,
	.open			= sd_open,
	.release		= sd_release,
	.ioctl			= sd_ioctl,
	.getgeo			= sd_getgeo,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= sd_compat_ioctl,
#endif
	.media_changed		= sd_media_changed,
	.revalidate_disk	= sd_revalidate_disk,
	.unlock_native_capacity	= sd_unlock_native_capacity,
};

static unsigned int sd_completed_bytes(struct scsi_cmnd *scmd)
{
	u64 start_lba = blk_rq_pos(scmd->request);
	u64 end_lba = blk_rq_pos(scmd->request) + (scsi_bufflen(scmd) / 512);
	u64 bad_lba;
	int info_valid;

	if (scmd->request->cmd_type != REQ_TYPE_FS)
		return 0;

	info_valid = scsi_get_sense_info_fld(scmd->sense_buffer,
					     SCSI_SENSE_BUFFERSIZE,
					     &bad_lba);
	if (!info_valid)
		return 0;

	if (scsi_bufflen(scmd) <= scmd->device->sector_size)
		return 0;

	if (scmd->device->sector_size < 512) {
		/* only legitimate sector_size here is 256 */
		start_lba <<= 1;
		end_lba <<= 1;
	} else {
		/* be careful ... don't want any overflows */
		u64 factor = scmd->device->sector_size / 512;
		do_div(start_lba, factor);
		do_div(end_lba, factor);
	}

	/* The bad lba was reported incorrectly, we have no idea where
	 * the error is.
	 */
	if (bad_lba < start_lba  || bad_lba >= end_lba)
		return 0;

	/* This computation should always be done in terms of
	 * the resolution of the device's medium.
	 */
	return (bad_lba - start_lba) * scmd->device->sector_size;
}

/**
 *	sd_done - bottom half handler: called when the lower level
 *	driver has completed (successfully or otherwise) a scsi command.
 *	@SCpnt: mid-level's per command structure.
 *
 *	Note: potentially run from within an ISR. Must not block.
 **/
static int sd_done(struct scsi_cmnd *SCpnt)
{
	int result = SCpnt->result;
	unsigned int good_bytes = result ? 0 : scsi_bufflen(SCpnt);
	struct scsi_sense_hdr sshdr;
	struct scsi_disk *sdkp = scsi_disk(SCpnt->request->rq_disk);
	int sense_valid = 0;
	int sense_deferred = 0;

	if (SCpnt->request->cmd_flags & REQ_DISCARD) {
		if (!result)
			scsi_set_resid(SCpnt, 0);
		return good_bytes;
	}

	if (result) {
		sense_valid = scsi_command_normalize_sense(SCpnt, &sshdr);
		if (sense_valid)
			sense_deferred = scsi_sense_is_deferred(&sshdr);
	}
#ifdef CONFIG_SCSI_LOGGING
	SCSI_LOG_HLCOMPLETE(1, scsi_print_result(SCpnt));
	if (sense_valid) {
		SCSI_LOG_HLCOMPLETE(1, scmd_printk(KERN_INFO, SCpnt,
						   "sd_done: sb[respc,sk,asc,"
						   "ascq]=%x,%x,%x,%x\n",
						   sshdr.response_code,
						   sshdr.sense_key, sshdr.asc,
						   sshdr.ascq));
	}
#endif
	if (driver_byte(result) != DRIVER_SENSE &&
	    (!sense_valid || sense_deferred))
		goto out;

	switch (sshdr.sense_key) {
	case HARDWARE_ERROR:
	case MEDIUM_ERROR:
		good_bytes = sd_completed_bytes(SCpnt);
		break;
	case RECOVERED_ERROR:
		good_bytes = scsi_bufflen(SCpnt);
		break;
	case NO_SENSE:
		/* This indicates a false check condition, so ignore it.  An
		 * unknown amount of data was transferred so treat it as an
		 * error.
		 */
		scsi_print_sense("sd", SCpnt);
		SCpnt->result = 0;
		memset(SCpnt->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
		break;
	case ABORTED_COMMAND: /* DIF: Target detected corruption */
	case ILLEGAL_REQUEST: /* DIX: Host detected corruption */
		if (sshdr.asc == 0x10)
			good_bytes = sd_completed_bytes(SCpnt);
		break;
	default:
		break;
	}
 out:
	if (rq_data_dir(SCpnt->request) == READ && scsi_prot_sg_count(SCpnt))
		sd_dif_complete(SCpnt, good_bytes);

	if (scsi_host_dif_capable(sdkp->device->host, sdkp->protection_type)
	    == SD_DIF_TYPE2_PROTECTION && SCpnt->cmnd != SCpnt->request->cmd) {

		/* We have to print a failed command here as the
		 * extended CDB gets freed before scsi_io_completion()
		 * is called.
		 */
		if (result)
			scsi_print_command(SCpnt);

		mempool_free(SCpnt->cmnd, sd_cdb_pool);
		SCpnt->cmnd = NULL;
		SCpnt->cmd_len = 0;
	}

	return good_bytes;
}

static int media_not_present(struct scsi_disk *sdkp,
			     struct scsi_sense_hdr *sshdr)
{

	if (!scsi_sense_valid(sshdr))
		return 0;
	/* not invoked for commands that could return deferred errors */
	if (sshdr->sense_key != NOT_READY &&
	    sshdr->sense_key != UNIT_ATTENTION)
		return 0;
	if (sshdr->asc != 0x3A) /* medium not present */
		return 0;

	set_media_not_present(sdkp);
	return 1;
}

/*
 * spinup disk - called only in sd_revalidate_disk()
 */
static void
sd_spinup_disk(struct scsi_disk *sdkp)
{
	unsigned char cmd[10];
	unsigned long spintime_expire = 0;
	int retries, spintime;
	unsigned int the_result;
	struct scsi_sense_hdr sshdr;
	int sense_valid = 0;

	spintime = 0;

	/* Spin up drives, as required.  Only do this at boot time */
	/* Spinup needs to be done for module loads too. */
	do {
		retries = 0;

		do {
			cmd[0] = TEST_UNIT_READY;
			memset((void *) &cmd[1], 0, 9);

			the_result = scsi_execute_req(sdkp->device, cmd,
						      DMA_NONE, NULL, 0,
						      &sshdr, SD_TIMEOUT,
						      SD_MAX_RETRIES, NULL);

			/*
			 * If the drive has indicated to us that it
			 * doesn't have any media in it, don't bother
			 * with any more polling.
			 */
			if (media_not_present(sdkp, &sshdr))
				/* 
				 * brief	some USB devices may react slowly, 
				 *			wait 0.5 second when the USB device 
				 *			is not ready before try again, and 
				 * 			the return is commented to allow 
				 *			the next try.
				 * modified by meizaihong, 2016-07-14
				 */
			{
#if 0
				return;
#else
				sd_printk(KERN_NOTICE, sdkp, "media is not present, wait for 0.5 seconds\n");
				msleep(500);
			} else {
				/* 
				 * brief	media_not_present() will change the 
				 * 			status of the USB device when it's 
				 * 			not ready. We reach here means the 
				 * 			device is ready, so we should update 
				 * 			the status of the device, sd_media_changed()
				 * 			can do it for us.
				 * added by meizaihong, 2016-07-14
				 */
				sd_printk(KERN_NOTICE, sdkp, "call sd_media_changed(), set sdkp->media_present to 1");
				sd_media_changed(sdkp->disk);
#endif
			}

			if (the_result)
				sense_valid = scsi_sense_valid(&sshdr);
			retries++;
		} while (retries < 10 &&  /* change from 3 to 10, try 10 times when device is not present, meizaihong, 2016-07-14 */
			 (!scsi_status_is_good(the_result) ||
			  ((driver_byte(the_result) & DRIVER_SENSE) &&
			  sense_valid && sshdr.sense_key == UNIT_ATTENTION)));
		
		/* 
		 * brief	We delete the return in the previous while loop, so we
		 * 			should check if the USB device is still not present now. 
		 * 			Check sshdr here, if media_not_present() returns 1, which 
		 *			means the USB device is not ready, we should return here.
		 * added by meizaihong, 2016-07-14
		 */
#if 1
		if (media_not_present(sdkp, &sshdr)) {
			sd_printk(KERN_NOTICE, sdkp, "we have tried 10 times, "
				"but the USB device is still not ready, just return here!\n");
			return;
		}
#endif
		if ((driver_byte(the_result) & DRIVER_SENSE) == 0) {
			/* no sense, TUR either succeeded or failed
			 * with a status error */
			if(!spintime && !scsi_status_is_good(the_result)) {
				sd_printk(KERN_NOTICE, sdkp, "Unit Not Ready\n");
				sd_print_result(sdkp, the_result);
			}
			break;
		}
					
		/*
		 * The device does not want the automatic start to be issued.
		 */
		if (sdkp->device->no_start_on_add)
			break;

		if (sense_valid && sshdr.sense_key == NOT_READY) {
			if (sshdr.asc == 4 && sshdr.ascq == 3)
				break;	/* manual intervention required */
			if (sshdr.asc == 4 && sshdr.ascq == 0xb)
				break;	/* standby */
			if (sshdr.asc == 4 && sshdr.ascq == 0xc)
				break;	/* unavailable */
			/*
			 * Issue command to spin up drive when not ready
			 */
			if (!spintime) {
				sd_printk(KERN_NOTICE, sdkp, "Spinning up disk...");
				cmd[0] = START_STOP;
				cmd[1] = 1;	/* Return immediately */
				memset((void *) &cmd[2], 0, 8);
				cmd[4] = 1;	/* Start spin cycle */
				if (sdkp->device->start_stop_pwr_cond)
					cmd[4] |= 1 << 4;
				scsi_execute_req(sdkp->device, cmd, DMA_NONE,
						 NULL, 0, &sshdr,
						 SD_TIMEOUT, SD_MAX_RETRIES,
						 NULL);
				spintime_expire = jiffies + 100 * HZ;
				spintime = 1;
			}
			/* Wait 1 second for next try */
			msleep(1000);
			printk(".");

		/*
		 * Wait for USB flash devices with slow firmware.
		 * Yes, this sense key/ASC combination shouldn't
		 * occur here.  It's characteristic of these devices.
		 */
		} else if (sense_valid &&
				sshdr.sense_key == UNIT_ATTENTION &&
				sshdr.asc == 0x28) {
			if (!spintime) {
				spintime_expire = jiffies + 5 * HZ;
				spintime = 1;
			}
			/* Wait 1 second for next try */
			msleep(1000);
		} else {
			/* we don't understand the sense code, so it's
			 * probably pointless to loop */
			if(!spintime) {
				sd_printk(KERN_NOTICE, sdkp, "Unit Not Ready\n");
				sd_print_sense_hdr(sdkp, &sshdr);
			}
			break;
		}
				
	} while (spintime && time_before_eq(jiffies, spintime_expire));

	if (spintime) {
		if (scsi_status_is_good(the_result))
			printk("ready\n");
		else
			printk("not responding...\n");
	}
}


/*
 * Determine whether disk supports Data Integrity Field.
 */
static void sd_read_protection_type(struct scsi_disk *sdkp, unsigned char *buffer)
{
	struct scsi_device *sdp = sdkp->device;
	u8 type;

	if (scsi_device_protection(sdp) == 0 || (buffer[12] & 1) == 0)
		return;

	type = ((buffer[12] >> 1) & 7) + 1; /* P_TYPE 0 = Type 1 */

	if (type == sdkp->protection_type || !sdkp->first_scan)
		return;

	sdkp->protection_type = type;

	if (type > SD_DIF_TYPE3_PROTECTION) {
		sd_printk(KERN_ERR, sdkp, "formatted with unsupported "	\
			  "protection type %u. Disabling disk!\n", type);
		sdkp->capacity = 0;
		return;
	}

	if (scsi_host_dif_capable(sdp->host, type))
		sd_printk(KERN_NOTICE, sdkp,
			  "Enabling DIF Type %u protection\n", type);
	else
		sd_printk(KERN_NOTICE, sdkp,
			  "Disabling DIF Type %u protection\n", type);
}

static void read_capacity_error(struct scsi_disk *sdkp, struct scsi_device *sdp,
			struct scsi_sense_hdr *sshdr, int sense_valid,
			int the_result)
{
	sd_print_result(sdkp, the_result);
	if (driver_byte(the_result) & DRIVER_SENSE)
		sd_print_sense_hdr(sdkp, sshdr);
	else
		sd_printk(KERN_NOTICE, sdkp, "Sense not available.\n");

	/*
	 * Set dirty bit for removable devices if not ready -
	 * sometimes drives will not report this properly.
	 */
	if (sdp->removable &&
	    sense_valid && sshdr->sense_key == NOT_READY)
		sdp->changed = 1;

	/*
	 * We used to set media_present to 0 here to indicate no media
	 * in the drive, but some drives fail read capacity even with
	 * media present, so we can't do that.
	 */
	sdkp->capacity = 0; /* unknown mapped to zero - as usual */
}

#define RC16_LEN 32
#if RC16_LEN > SD_BUF_SIZE
#error RC16_LEN must not be more than SD_BUF_SIZE
#endif

#define READ_CAPACITY_RETRIES_ON_RESET	10

static int read_capacity_16(struct scsi_disk *sdkp, struct scsi_device *sdp,
						unsigned char *buffer)
{
	unsigned char cmd[16];
	struct scsi_sense_hdr sshdr;
	int sense_valid = 0;
	int the_result;
	int retries = 3, reset_retries = READ_CAPACITY_RETRIES_ON_RESET;
	unsigned int alignment;
	unsigned long long lba;
	unsigned sector_size;

	do {
		memset(cmd, 0, 16);
		cmd[0] = SERVICE_ACTION_IN;
		cmd[1] = SAI_READ_CAPACITY_16;
		cmd[13] = RC16_LEN;
		memset(buffer, 0, RC16_LEN);

		the_result = scsi_execute_req(sdp, cmd, DMA_FROM_DEVICE,
					buffer, RC16_LEN, &sshdr,
					SD_TIMEOUT, SD_MAX_RETRIES, NULL);

		if (media_not_present(sdkp, &sshdr))
			return -ENODEV;

		if (the_result) {
			sense_valid = scsi_sense_valid(&sshdr);
			if (sense_valid &&
			    sshdr.sense_key == ILLEGAL_REQUEST &&
			    (sshdr.asc == 0x20 || sshdr.asc == 0x24) &&
			    sshdr.ascq == 0x00)
				/* Invalid Command Operation Code or
				 * Invalid Field in CDB, just retry
				 * silently with RC10 */
				return -EINVAL;
			if (sense_valid &&
			    sshdr.sense_key == UNIT_ATTENTION &&
			    sshdr.asc == 0x29 && sshdr.ascq == 0x00)
				/* Device reset might occur several times,
				 * give it one more chance */
				if (--reset_retries > 0)
					continue;
		}
		retries--;

	} while (the_result && retries);

	if (the_result) {
		sd_printk(KERN_NOTICE, sdkp, "READ CAPACITY(16) failed\n");
		read_capacity_error(sdkp, sdp, &sshdr, sense_valid, the_result);
		return -EINVAL;
	}

	sector_size = get_unaligned_be32(&buffer[8]);
	lba = get_unaligned_be64(&buffer[0]);

	sd_read_protection_type(sdkp, buffer);

	if ((sizeof(sdkp->capacity) == 4) && (lba >= 0xffffffffULL)) {
		sd_printk(KERN_ERR, sdkp, "Too big for this kernel. Use a "
			"kernel compiled with support for large block "
			"devices.\n");
		sdkp->capacity = 0;
		return -EOVERFLOW;
	}

	/* Logical blocks per physical block exponent */
	sdkp->hw_sector_size = (1 << (buffer[13] & 0xf)) * sector_size;

	/* Lowest aligned logical block */
	alignment = ((buffer[14] & 0x3f) << 8 | buffer[15]) * sector_size;
	blk_queue_alignment_offset(sdp->request_queue, alignment);
	if (alignment && sdkp->first_scan)
		sd_printk(KERN_NOTICE, sdkp,
			  "physical block alignment offset: %u\n", alignment);

	if (buffer[14] & 0x80) { /* TPE */
		struct request_queue *q = sdp->request_queue;

		sdkp->thin_provisioning = 1;
		q->limits.discard_granularity = sdkp->hw_sector_size;
		q->limits.max_discard_sectors = 0xffffffff;

		if (buffer[14] & 0x40) /* TPRZ */
			q->limits.discard_zeroes_data = 1;

		queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, q);
	}

	sdkp->capacity = lba + 1;
	return sector_size;
}

static int read_capacity_10(struct scsi_disk *sdkp, struct scsi_device *sdp,
						unsigned char *buffer)
{
	unsigned char cmd[16];
	struct scsi_sense_hdr sshdr;
	int sense_valid = 0;
	int the_result;
	int retries = 3, reset_retries = READ_CAPACITY_RETRIES_ON_RESET;
	sector_t lba;
	unsigned sector_size;

	do {
		cmd[0] = READ_CAPACITY;
		memset(&cmd[1], 0, 9);
		memset(buffer, 0, 8);

		the_result = scsi_execute_req(sdp, cmd, DMA_FROM_DEVICE,
					buffer, 8, &sshdr,
					SD_TIMEOUT, SD_MAX_RETRIES, NULL);

		if (media_not_present(sdkp, &sshdr))
			return -ENODEV;

		if (the_result) {
			sense_valid = scsi_sense_valid(&sshdr);
			if (sense_valid &&
			    sshdr.sense_key == UNIT_ATTENTION &&
			    sshdr.asc == 0x29 && sshdr.ascq == 0x00)
				/* Device reset might occur several times,
				 * give it one more chance */
				if (--reset_retries > 0)
					continue;
		}
		retries--;

	} while (the_result && retries);

	if (the_result) {
		sd_printk(KERN_NOTICE, sdkp, "READ CAPACITY failed\n");
		read_capacity_error(sdkp, sdp, &sshdr, sense_valid, the_result);
		return -EINVAL;
	}

	sector_size = get_unaligned_be32(&buffer[4]);
	lba = get_unaligned_be32(&buffer[0]);

	if ((sizeof(sdkp->capacity) == 4) && (lba == 0xffffffff)) {
		sd_printk(KERN_ERR, sdkp, "Too big for this kernel. Use a "
			"kernel compiled with support for large block "
			"devices.\n");
		sdkp->capacity = 0;
		return -EOVERFLOW;
	}

	sdkp->capacity = lba + 1;
	sdkp->hw_sector_size = sector_size;
	return sector_size;
}

static int sd_try_rc16_first(struct scsi_device *sdp)
{
	if (sdp->host->max_cmd_len < 16)
		return 0;
	if (sdp->scsi_level > SCSI_SPC_2)
		return 1;
	if (scsi_device_protection(sdp))
		return 1;
	return 0;
}

/*
 * read disk capacity
 */
static void
sd_read_capacity(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int sector_size;
	struct scsi_device *sdp = sdkp->device;
	sector_t old_capacity = sdkp->capacity;

	if (sd_try_rc16_first(sdp)) {
		sector_size = read_capacity_16(sdkp, sdp, buffer);
		if (sector_size == -EOVERFLOW)
			goto got_data;
		if (sector_size == -ENODEV)
			return;
		if (sector_size < 0)
			sector_size = read_capacity_10(sdkp, sdp, buffer);
		if (sector_size < 0)
			return;
	} else {
		sector_size = read_capacity_10(sdkp, sdp, buffer);
		if (sector_size == -EOVERFLOW)
			goto got_data;
		if (sector_size < 0)
			return;
		if ((sizeof(sdkp->capacity) > 4) &&
		    (sdkp->capacity > 0xffffffffULL)) {
			int old_sector_size = sector_size;
			sd_printk(KERN_NOTICE, sdkp, "Very big device. "
					"Trying to use READ CAPACITY(16).\n");
			sector_size = read_capacity_16(sdkp, sdp, buffer);
			if (sector_size < 0) {
				sd_printk(KERN_NOTICE, sdkp,
					"Using 0xffffffff as device size\n");
				sdkp->capacity = 1 + (sector_t) 0xffffffff;
				sector_size = old_sector_size;
				goto got_data;
			}
		}
	}

	/* Some devices are known to return the total number of blocks,
	 * not the highest block number.  Some devices have versions
	 * which do this and others which do not.  Some devices we might
	 * suspect of doing this but we don't know for certain.
	 *
	 * If we know the reported capacity is wrong, decrement it.  If
	 * we can only guess, then assume the number of blocks is even
	 * (usually true but not always) and err on the side of lowering
	 * the capacity.
	 */
	if (sdp->fix_capacity ||
	    (sdp->guess_capacity && (sdkp->capacity & 0x01))) {
		sd_printk(KERN_INFO, sdkp, "Adjusting the sector count "
				"from its reported value: %llu\n",
				(unsigned long long) sdkp->capacity);
		--sdkp->capacity;
	}

got_data:
	if (sector_size == 0) {
		sector_size = 512;
		sd_printk(KERN_NOTICE, sdkp, "Sector size 0 reported, "
			  "assuming 512.\n");
	}

	if (sector_size != 512 &&
	    sector_size != 1024 &&
	    sector_size != 2048 &&
	    sector_size != 4096 &&
	    sector_size != 256) {
		sd_printk(KERN_NOTICE, sdkp, "Unsupported sector size %d.\n",
			  sector_size);
		/*
		 * The user might want to re-format the drive with
		 * a supported sectorsize.  Once this happens, it
		 * would be relatively trivial to set the thing up.
		 * For this reason, we leave the thing in the table.
		 */
		sdkp->capacity = 0;
		/*
		 * set a bogus sector size so the normal read/write
		 * logic in the block layer will eventually refuse any
		 * request on this device without tripping over power
		 * of two sector size assumptions
		 */
		sector_size = 512;
	}
	blk_queue_logical_block_size(sdp->request_queue, sector_size);

	{
		char cap_str_2[10], cap_str_10[10];
		u64 sz = (u64)sdkp->capacity << ilog2(sector_size);

		string_get_size(sz, STRING_UNITS_2, cap_str_2,
				sizeof(cap_str_2));
		string_get_size(sz, STRING_UNITS_10, cap_str_10,
				sizeof(cap_str_10));

		if (sdkp->first_scan || old_capacity != sdkp->capacity) {
			sd_printk(KERN_NOTICE, sdkp,
				  "%llu %d-byte logical blocks: (%s/%s)\n",
				  (unsigned long long)sdkp->capacity,
				  sector_size, cap_str_10, cap_str_2);

			if (sdkp->hw_sector_size != sector_size)
				sd_printk(KERN_NOTICE, sdkp,
					  "%u-byte physical blocks\n",
					  sdkp->hw_sector_size);
		}
	}

	/* Rescale capacity to 512-byte units */
	if (sector_size == 4096)
		sdkp->capacity <<= 3;
	else if (sector_size == 2048)
		sdkp->capacity <<= 2;
	else if (sector_size == 1024)
		sdkp->capacity <<= 1;
	else if (sector_size == 256)
		sdkp->capacity >>= 1;

	blk_queue_physical_block_size(sdp->request_queue, sdkp->hw_sector_size);
	sdkp->device->sector_size = sector_size;
}

/* called with buffer of length 512 */
static inline int
sd_do_mode_sense(struct scsi_device *sdp, int dbd, int modepage,
		 unsigned char *buffer, int len, struct scsi_mode_data *data,
		 struct scsi_sense_hdr *sshdr)
{
	return scsi_mode_sense(sdp, dbd, modepage, buffer, len,
			       SD_TIMEOUT, SD_MAX_RETRIES, data,
			       sshdr);
}

/*
 * read write protect setting, if possible - called only in sd_revalidate_disk()
 * called with buffer of length SD_BUF_SIZE
 */
static void
sd_read_write_protect_flag(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int res;
	struct scsi_device *sdp = sdkp->device;
	struct scsi_mode_data data;
	int old_wp = sdkp->write_prot;

	set_disk_ro(sdkp->disk, 0);
	if (sdp->skip_ms_page_3f) {
		sd_printk(KERN_NOTICE, sdkp, "Assuming Write Enabled\n");
		return;
	}

	if (sdp->use_192_bytes_for_3f) {
		res = sd_do_mode_sense(sdp, 0, 0x3F, buffer, 192, &data, NULL);
	} else {
		/*
		 * First attempt: ask for all pages (0x3F), but only 4 bytes.
		 * We have to start carefully: some devices hang if we ask
		 * for more than is available.
		 */
		res = sd_do_mode_sense(sdp, 0, 0x3F, buffer, 4, &data, NULL);

		/*
		 * Second attempt: ask for page 0 When only page 0 is
		 * implemented, a request for page 3F may return Sense Key
		 * 5: Illegal Request, Sense Code 24: Invalid field in
		 * CDB.
		 */
		if (!scsi_status_is_good(res))
			res = sd_do_mode_sense(sdp, 0, 0, buffer, 4, &data, NULL);

		/*
		 * Third attempt: ask 255 bytes, as we did earlier.
		 */
		if (!scsi_status_is_good(res))
			res = sd_do_mode_sense(sdp, 0, 0x3F, buffer, 255,
					       &data, NULL);
	}

	if (!scsi_status_is_good(res)) {
		sd_printk(KERN_WARNING, sdkp,
			  "Test WP failed, assume Write Enabled\n");
	} else {
		sdkp->write_prot = ((data.device_specific & 0x80) != 0);
		set_disk_ro(sdkp->disk, sdkp->write_prot);
		if (sdkp->first_scan || old_wp != sdkp->write_prot) {
			sd_printk(KERN_NOTICE, sdkp, "Write Protect is %s\n",
				  sdkp->write_prot ? "on" : "off");
			sd_printk(KERN_DEBUG, sdkp,
				  "Mode Sense: %02x %02x %02x %02x\n",
				  buffer[0], buffer[1], buffer[2], buffer[3]);
		}
	}
}

/*
 * sd_read_cache_type - called only from sd_revalidate_disk()
 * called with buffer of length SD_BUF_SIZE
 */
static void
sd_read_cache_type(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int len = 0, res;
	struct scsi_device *sdp = sdkp->device;

	int dbd;
	int modepage;
	struct scsi_mode_data data;
	struct scsi_sense_hdr sshdr;
	int old_wce = sdkp->WCE;
	int old_rcd = sdkp->RCD;
	int old_dpofua = sdkp->DPOFUA;

	if (sdp->skip_ms_page_8)
		goto defaults;

	if (sdp->type == TYPE_RBC) {
		modepage = 6;
		dbd = 8;
	} else {
		modepage = 8;
		dbd = 0;
	}

	/* cautiously ask */
	res = sd_do_mode_sense(sdp, dbd, modepage, buffer, 4, &data, &sshdr);

	if (!scsi_status_is_good(res))
		goto bad_sense;

	if (!data.header_length) {
		modepage = 6;
		sd_printk(KERN_ERR, sdkp, "Missing header in MODE_SENSE response\n");
	}

	/* that went OK, now ask for the proper length */
	len = data.length;

	/*
	 * We're only interested in the first three bytes, actually.
	 * But the data cache page is defined for the first 20.
	 */
	if (len < 3)
		goto bad_sense;
	if (len > 20)
		len = 20;

	/* Take headers and block descriptors into account */
	len += data.header_length + data.block_descriptor_length;
	if (len > SD_BUF_SIZE)
		goto bad_sense;

	/* Get the data */
	res = sd_do_mode_sense(sdp, dbd, modepage, buffer, len, &data, &sshdr);

	if (scsi_status_is_good(res)) {
		int offset = data.header_length + data.block_descriptor_length;

		if (offset >= SD_BUF_SIZE - 2) {
			sd_printk(KERN_ERR, sdkp, "Malformed MODE SENSE response\n");
			goto defaults;
		}

		if ((buffer[offset] & 0x3f) != modepage) {
			sd_printk(KERN_ERR, sdkp, "Got wrong page\n");
			goto defaults;
		}

		if (modepage == 8) {
			sdkp->WCE = ((buffer[offset + 2] & 0x04) != 0);
			sdkp->RCD = ((buffer[offset + 2] & 0x01) != 0);
		} else {
			sdkp->WCE = ((buffer[offset + 2] & 0x01) == 0);
			sdkp->RCD = 0;
		}

		sdkp->DPOFUA = (data.device_specific & 0x10) != 0;
		if (sdkp->DPOFUA && !sdkp->device->use_10_for_rw) {
			sd_printk(KERN_NOTICE, sdkp,
				  "Uses READ/WRITE(6), disabling FUA\n");
			sdkp->DPOFUA = 0;
		}

		if (sdkp->first_scan || old_wce != sdkp->WCE ||
		    old_rcd != sdkp->RCD || old_dpofua != sdkp->DPOFUA)
			sd_printk(KERN_NOTICE, sdkp,
				  "Write cache: %s, read cache: %s, %s\n",
				  sdkp->WCE ? "enabled" : "disabled",
				  sdkp->RCD ? "disabled" : "enabled",
				  sdkp->DPOFUA ? "supports DPO and FUA"
				  : "doesn't support DPO or FUA");

		return;
	}

bad_sense:
	if (scsi_sense_valid(&sshdr) &&
	    sshdr.sense_key == ILLEGAL_REQUEST &&
	    sshdr.asc == 0x24 && sshdr.ascq == 0x0)
		/* Invalid field in CDB */
		sd_printk(KERN_NOTICE, sdkp, "Cache data unavailable\n");
	else
		sd_printk(KERN_ERR, sdkp, "Asking for cache data failed\n");

defaults:
	sd_printk(KERN_ERR, sdkp, "Assuming drive cache: write through\n");
	sdkp->WCE = 0;
	sdkp->RCD = 0;
	sdkp->DPOFUA = 0;
}

/*
 * The ATO bit indicates whether the DIF application tag is available
 * for use by the operating system.
 */
static void sd_read_app_tag_own(struct scsi_disk *sdkp, unsigned char *buffer)
{
	int res, offset;
	struct scsi_device *sdp = sdkp->device;
	struct scsi_mode_data data;
	struct scsi_sense_hdr sshdr;

	if (sdp->type != TYPE_DISK)
		return;

	if (sdkp->protection_type == 0)
		return;

	res = scsi_mode_sense(sdp, 1, 0x0a, buffer, 36, SD_TIMEOUT,
			      SD_MAX_RETRIES, &data, &sshdr);

	if (!scsi_status_is_good(res) || !data.header_length ||
	    data.length < 6) {
		sd_printk(KERN_WARNING, sdkp,
			  "getting Control mode page failed, assume no ATO\n");

		if (scsi_sense_valid(&sshdr))
			sd_print_sense_hdr(sdkp, &sshdr);

		return;
	}

	offset = data.header_length + data.block_descriptor_length;

	if ((buffer[offset] & 0x3f) != 0x0a) {
		sd_printk(KERN_ERR, sdkp, "ATO Got wrong page\n");
		return;
	}

	if ((buffer[offset + 5] & 0x80) == 0)
		return;

	sdkp->ATO = 1;

	return;
}

/**
 * sd_read_block_limits - Query disk device for preferred I/O sizes.
 * @disk: disk to query
 */
static void sd_read_block_limits(struct scsi_disk *sdkp)
{
	struct request_queue *q = sdkp->disk->queue;
	unsigned int sector_sz = sdkp->device->sector_size;
	const int vpd_len = 64;
	unsigned char *buffer = kmalloc(vpd_len, GFP_KERNEL);

	if (!buffer ||
	    /* Block Limits VPD */
	    scsi_get_vpd_page(sdkp->device, 0xb0, buffer, vpd_len))
		goto out;

	blk_queue_io_min(sdkp->disk->queue,
			 get_unaligned_be16(&buffer[6]) * sector_sz);
	blk_queue_io_opt(sdkp->disk->queue,
			 get_unaligned_be32(&buffer[12]) * sector_sz);

	/* Thin provisioning enabled and page length indicates TP support */
	if (sdkp->thin_provisioning && buffer[3] == 0x3c) {
		unsigned int lba_count, desc_count, granularity;

		lba_count = get_unaligned_be32(&buffer[20]);
		desc_count = get_unaligned_be32(&buffer[24]);

		if (lba_count) {
			q->limits.max_discard_sectors =
				lba_count * sector_sz >> 9;

			if (desc_count)
				sdkp->unmap = 1;
		}

		granularity = get_unaligned_be32(&buffer[28]);

		if (granularity)
			q->limits.discard_granularity = granularity * sector_sz;

		if (buffer[32] & 0x80)
			q->limits.discard_alignment =
				get_unaligned_be32(&buffer[32]) & ~(1 << 31);
	}

 out:
	kfree(buffer);
}

/**
 * sd_read_block_characteristics - Query block dev. characteristics
 * @disk: disk to query
 */
static void sd_read_block_characteristics(struct scsi_disk *sdkp)
{
	unsigned char *buffer;
	u16 rot;
	const int vpd_len = 64;

	buffer = kmalloc(vpd_len, GFP_KERNEL);

	if (!buffer ||
	    /* Block Device Characteristics VPD */
	    scsi_get_vpd_page(sdkp->device, 0xb1, buffer, vpd_len))
		goto out;

	rot = get_unaligned_be16(&buffer[4]);

	if (rot == 1)
		queue_flag_set_unlocked(QUEUE_FLAG_NONROT, sdkp->disk->queue);

 out:
	kfree(buffer);
}

static int sd_try_extended_inquiry(struct scsi_device *sdp)
{
	/*
	 * Although VPD inquiries can go to SCSI-2 type devices,
	 * some USB ones crash on receiving them, and the pages
	 * we currently ask for are for SPC-3 and beyond
	 */
	if (sdp->scsi_level > SCSI_SPC_2)
		return 1;
	return 0;
}

/**
 *	sd_revalidate_disk - called the first time a new disk is seen,
 *	performs disk spin up, read_capacity, etc.
 *	@disk: struct gendisk we care about
 **/
static int sd_revalidate_disk(struct gendisk *disk)
{
	struct scsi_disk *sdkp = scsi_disk(disk);
	struct scsi_device *sdp = sdkp->device;
	unsigned char *buffer;
	unsigned ordered;

	SCSI_LOG_HLQUEUE(3, sd_printk(KERN_INFO, sdkp,
				      "sd_revalidate_disk\n"));

	/*
	 * If the device is offline, don't try and read capacity or any
	 * of the other niceties.
	 */
	if (!scsi_device_online(sdp))
		goto out;

	buffer = kmalloc(SD_BUF_SIZE, GFP_KERNEL);
	if (!buffer) {
		sd_printk(KERN_WARNING, sdkp, "sd_revalidate_disk: Memory "
			  "allocation failure.\n");
		goto out;
	}

	sd_spinup_disk(sdkp);

	/*
	 * Without media there is no reason to ask; moreover, some devices
	 * react badly if we do.
	 */
	if (sdkp->media_present) {
		sd_read_capacity(sdkp, buffer);

		if (sd_try_extended_inquiry(sdp)) {
			sd_read_block_limits(sdkp);
			sd_read_block_characteristics(sdkp);
		}

		sd_read_write_protect_flag(sdkp, buffer);
		sd_read_cache_type(sdkp, buffer);
		sd_read_app_tag_own(sdkp, buffer);
	}

	sdkp->first_scan = 0;

	/*
	 * We now have all cache related info, determine how we deal
	 * with ordered requests.  Note that as the current SCSI
	 * dispatch function can alter request order, we cannot use
	 * QUEUE_ORDERED_TAG_* even when ordered tag is supported.
	 */
	if (sdkp->WCE)
		ordered = sdkp->DPOFUA
			? QUEUE_ORDERED_DRAIN_FUA : QUEUE_ORDERED_DRAIN_FLUSH;
	else
		ordered = QUEUE_ORDERED_DRAIN;

	blk_queue_ordered(sdkp->disk->queue, ordered);

	set_capacity(disk, sdkp->capacity);
	kfree(buffer);

 out:
	return 0;
}

/**
 *	sd_unlock_native_capacity - unlock native capacity
 *	@disk: struct gendisk to set capacity for
 *
 *	Block layer calls this function if it detects that partitions
 *	on @disk reach beyond the end of the device.  If the SCSI host
 *	implements ->unlock_native_capacity() method, it's invoked to
 *	give it a chance to adjust the device capacity.
 *
 *	CONTEXT:
 *	Defined by block layer.  Might sleep.
 */
static void sd_unlock_native_capacity(struct gendisk *disk)
{
	struct scsi_device *sdev = scsi_disk(disk)->device;

	if (sdev->host->hostt->unlock_native_capacity)
		sdev->host->hostt->unlock_native_capacity(sdev);
}

/**
 *	sd_format_disk_name - format disk name
 *	@prefix: name prefix - ie. "sd" for SCSI disks
 *	@index: index of the disk to format name for
 *	@buf: output buffer
 *	@buflen: length of the output buffer
 *
 *	SCSI disk names starts at sda.  The 26th device is sdz and the
 *	27th is sdaa.  The last one for two lettered suffix is sdzz
 *	which is followed by sdaaa.
 *
 *	This is basically 26 base counting with one extra 'nil' entry
 *	at the beginning from the second digit on and can be
 *	determined using similar method as 26 base conversion with the
 *	index shifted -1 after each digit is computed.
 *
 *	CONTEXT:
 *	Don't care.
 *
 *	RETURNS:
 *	0 on success, -errno on failure.
 */
static int sd_format_disk_name(char *prefix, int index, char *buf, int buflen)
{
	const int base = 'z' - 'a' + 1;
	char *begin = buf + strlen(prefix);
	char *end = buf + buflen;
	char *p;
	int unit;

	p = end - 1;
	*p = '\0';
	unit = base;
	do {
		if (p == begin)
			return -EINVAL;
		*--p = 'a' + (index % unit);
		index = (index / unit) - 1;
	} while (index >= 0);

	memmove(begin, p, end - p);
	memcpy(buf, prefix, strlen(prefix));

	return 0;
}

/*
 * The asynchronous part of sd_probe
 */
static void sd_probe_async(void *data, async_cookie_t cookie)
{
	struct scsi_disk *sdkp = data;
	struct scsi_device *sdp;
	struct gendisk *gd;
	u32 index;
	struct device *dev;

	sdp = sdkp->device;
	gd = sdkp->disk;
	index = sdkp->index;
	dev = &sdp->sdev_gendev;

	if (index < SD_MAX_DISKS) {
		gd->major = sd_major((index & 0xf0) >> 4);
		gd->first_minor = ((index & 0xf) << 4) | (index & 0xfff00);
		gd->minors = SD_MINORS;
	}
	gd->fops = &sd_fops;
	gd->private_data = &sdkp->driver;
	gd->queue = sdkp->device->request_queue;

	/* defaults, until the device tells us otherwise */
	sdp->sector_size = 512;
	sdkp->capacity = 0;
	sdkp->media_present = 1;
	sdkp->write_prot = 0;
	sdkp->WCE = 0;
	sdkp->RCD = 0;
	sdkp->ATO = 0;
	sdkp->first_scan = 1;

	sd_revalidate_disk(gd);

	blk_queue_prep_rq(sdp->request_queue, sd_prep_fn);
	blk_queue_unprep_rq(sdp->request_queue, sd_unprep_fn);

	gd->driverfs_dev = &sdp->sdev_gendev;
	gd->flags = GENHD_FL_EXT_DEVT;
	if (sdp->removable)
		gd->flags |= GENHD_FL_REMOVABLE;

	add_disk(gd);
	sd_dif_config_host(sdkp);

	sd_revalidate_disk(gd);
	sd_printk(KERN_NOTICE, sdkp, "Attached SCSI %sdisk\n",
		  sdp->removable ? "removable " : "");
	scsi_autopm_put_device(sdp);
	put_device(&sdkp->dev);
#ifdef USB_AUTOMOUNT   
    if(mount_task_p != NULL)
    {
        mutex_lock(&signal_mutex);
        send_sig(SIGUSR1, mount_task_p, 0);
        mutex_unlock(&signal_mutex);
    }
#endif 
#ifdef TCSUPPORT_USB_HOST_LED
#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7520) && !defined(TCSUPPORT_CPU_MT7505)
	usb_dev_connected ++; //usb led support
#endif
#endif

}

/**
 *	sd_probe - called during driver initialization and whenever a
 *	new scsi device is attached to the system. It is called once
 *	for each scsi device (not just disks) present.
 *	@dev: pointer to device object
 *
 *	Returns 0 if successful (or not interested in this scsi device 
 *	(e.g. scanner)); 1 when there is an error.
 *
 *	Note: this function is invoked from the scsi mid-level.
 *	This function sets up the mapping between a given 
 *	<host,channel,id,lun> (found in sdp) and new device name 
 *	(e.g. /dev/sda). More precisely it is the block device major 
 *	and minor number that is chosen here.
 *
 *	Assume sd_attach is not re-entrant (for time being)
 *	Also think about sd_attach() and sd_remove() running coincidentally.
 **/


static int sd_probe(struct device *dev)
{
	struct scsi_device *sdp = to_scsi_device(dev);
	struct scsi_disk *sdkp;
	struct gendisk *gd;
	int index;
	int error;

	error = -ENODEV;
	if (sdp->type != TYPE_DISK && sdp->type != TYPE_MOD && sdp->type != TYPE_RBC)
		goto out;

	SCSI_LOG_HLQUEUE(3, sdev_printk(KERN_INFO, sdp,
					"sd_attach\n"));

	error = -ENOMEM;
	sdkp = kzalloc(sizeof(*sdkp), GFP_KERNEL);
	if (!sdkp)
		goto out;

	gd = alloc_disk(SD_MINORS);
	if (!gd)
		goto out_free;

	do {
		if (!ida_pre_get(&sd_index_ida, GFP_KERNEL))
			goto out_put;

		spin_lock(&sd_index_lock);
		error = ida_get_new(&sd_index_ida, &index);
		spin_unlock(&sd_index_lock);
	} while (error == -EAGAIN);

	if (error)
		goto out_put;

	error = sd_format_disk_name("sd", index, gd->disk_name, DISK_NAME_LEN);
	if (error)
		goto out_free_index;

	sdkp->device = sdp;
	sdkp->driver = &sd_template;
	sdkp->disk = gd;
	sdkp->index = index;
	atomic_set(&sdkp->openers, 0);
	sdkp->previous_state = 1;

	if (!sdp->request_queue->rq_timeout) {
		if (sdp->type != TYPE_MOD)
			blk_queue_rq_timeout(sdp->request_queue, SD_TIMEOUT);
		else
			blk_queue_rq_timeout(sdp->request_queue,
					     SD_MOD_TIMEOUT);
	}

	device_initialize(&sdkp->dev);
	sdkp->dev.parent = dev;
	sdkp->dev.class = &sd_disk_class;
	dev_set_name(&sdkp->dev, dev_name(dev));

	if (device_add(&sdkp->dev))
		goto out_free_index;

	get_device(dev);
	dev_set_drvdata(dev, sdkp);

	get_device(&sdkp->dev);	/* prevent release before async_schedule */
	async_schedule(sd_probe_async, sdkp);

	return 0;

 out_free_index:
	spin_lock(&sd_index_lock);
	ida_remove(&sd_index_ida, index);
	spin_unlock(&sd_index_lock);
 out_put:
	put_disk(gd);
 out_free:
	kfree(sdkp);
 out:
	return error;
}

/**
 *	sd_remove - called whenever a scsi disk (previously recognized by
 *	sd_probe) is detached from the system. It is called (potentially
 *	multiple times) during sd module unload.
 *	@sdp: pointer to mid level scsi device object
 *
 *	Note: this function is invoked from the scsi mid-level.
 *	This function potentially frees up a device name (e.g. /dev/sdc)
 *	that could be re-used by a subsequent sd_probe().
 *	This function is not called when the built-in sd driver is "exit-ed".
 **/
static int sd_remove(struct device *dev)
{
	struct scsi_disk *sdkp;

	sdkp = dev_get_drvdata(dev);
	scsi_autopm_get_device(sdkp->device);

	async_synchronize_full();
	blk_queue_prep_rq(sdkp->device->request_queue, scsi_prep_fn);
	blk_queue_unprep_rq(sdkp->device->request_queue, NULL);
	device_del(&sdkp->dev);
	del_gendisk(sdkp->disk);
	sd_shutdown(dev);

	mutex_lock(&sd_ref_mutex);
	dev_set_drvdata(dev, NULL);
	put_device(&sdkp->dev);
	mutex_unlock(&sd_ref_mutex);
#ifdef USB_AUTOMOUNT
    if(mount_task_p != NULL)
    {
        mutex_lock(&signal_mutex);
        send_sig(SIGUSR2, mount_task_p, 0);
        mutex_unlock(&signal_mutex);
        //msleep(500); /* let AutoMount can deal with umount right after SIGUSR2 is sent*/
    } 
#endif
#ifdef TCSUPPORT_USB_HOST_LED
#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7520) && !defined(TCSUPPORT_CPU_MT7505)
    if(usb_dev_connected > 0)
        usb_dev_connected --;    //usb led support
#endif
#endif
	return 0;
}

/**
 *	scsi_disk_release - Called to free the scsi_disk structure
 *	@dev: pointer to embedded class device
 *
 *	sd_ref_mutex must be held entering this routine.  Because it is
 *	called on last put, you should always use the scsi_disk_get()
 *	scsi_disk_put() helpers which manipulate the semaphore directly
 *	and never do a direct put_device.
 **/
static void scsi_disk_release(struct device *dev)
{
	struct scsi_disk *sdkp = to_scsi_disk(dev);
	struct gendisk *disk = sdkp->disk;
	
	spin_lock(&sd_index_lock);
	ida_remove(&sd_index_ida, sdkp->index);
	spin_unlock(&sd_index_lock);

	disk->private_data = NULL;
	put_disk(disk);
	put_device(&sdkp->device->sdev_gendev);

	kfree(sdkp);
}

static int sd_start_stop_device(struct scsi_disk *sdkp, int start)
{
	unsigned char cmd[6] = { START_STOP };	/* START_VALID */
	struct scsi_sense_hdr sshdr;
	struct scsi_device *sdp = sdkp->device;
	int res;

	if (start)
		cmd[4] |= 1;	/* START */

	if (sdp->start_stop_pwr_cond)
		cmd[4] |= start ? 1 << 4 : 3 << 4;	/* Active or Standby */

	if (!scsi_device_online(sdp))
		return -ENODEV;

	res = scsi_execute_req(sdp, cmd, DMA_NONE, NULL, 0, &sshdr,
			       SD_TIMEOUT, SD_MAX_RETRIES, NULL);
	if (res) {
		sd_printk(KERN_WARNING, sdkp, "START_STOP FAILED\n");
		sd_print_result(sdkp, res);
		if (driver_byte(res) & DRIVER_SENSE)
			sd_print_sense_hdr(sdkp, &sshdr);
	}

	return res;
}

/*
 * Send a SYNCHRONIZE CACHE instruction down to the device through
 * the normal SCSI command structure.  Wait for the command to
 * complete.
 */
static void sd_shutdown(struct device *dev)
{
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);

	if (!sdkp)
		return;         /* this can happen */

	if (sdkp->WCE) {
		sd_printk(KERN_NOTICE, sdkp, "Synchronizing SCSI cache\n");
		sd_sync_cache(sdkp);
	}

	if (system_state != SYSTEM_RESTART && sdkp->device->manage_start_stop) {
		sd_printk(KERN_NOTICE, sdkp, "Stopping disk\n");
		sd_start_stop_device(sdkp, 0);
	}

	scsi_disk_put(sdkp);
}

static int sd_suspend(struct device *dev, pm_message_t mesg)
{
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);
	int ret = 0;

	if (!sdkp)
		return 0;	/* this can happen */

	if (sdkp->WCE) {
		sd_printk(KERN_NOTICE, sdkp, "Synchronizing SCSI cache\n");
		ret = sd_sync_cache(sdkp);
		if (ret)
			goto done;
	}

	if ((mesg.event & PM_EVENT_SLEEP) && sdkp->device->manage_start_stop) {
		sd_printk(KERN_NOTICE, sdkp, "Stopping disk\n");
		ret = sd_start_stop_device(sdkp, 0);
	}

done:
	scsi_disk_put(sdkp);
	return ret;
}

static int sd_resume(struct device *dev)
{
	struct scsi_disk *sdkp = scsi_disk_get_from_dev(dev);
	int ret = 0;

	if (!sdkp->device->manage_start_stop)
		goto done;

	sd_printk(KERN_NOTICE, sdkp, "Starting disk\n");
	ret = sd_start_stop_device(sdkp, 1);

done:
	scsi_disk_put(sdkp);
	return ret;
}

/**
 *	init_sd - entry point for this driver (both when built in or when
 *	a module).
 *
 *	Note: this function registers this driver with the scsi mid-level.
 **/
static int __init init_sd(void)
{
	int majors = 0, i, err;
#ifdef USB_AUTOMOUNT
    struct proc_dir_entry *automount_proc;

/* 1. mount_task_p: a pointer which will (later) point to the task_struct 
 *      structure that stands for AutoMount's PID. The pointer will be used
 *      by send_sig() to send SIGUSR1 or SIGUSR2 to AutoMount.
 * 2. signal_mutex: a mutex used to prevent more than one signals are sent
 *      at the same time.
 * 3. automount_proc: used when AutoMount write its PID to
 *      "tc3162/automount_pid", automount_pid_write_proc() will be
 *      triggered to transform the PID to the task_struct structure which
 *      is pointed by mount_task_p. Then, mount_task_p can be used to send
 *      signals to AutoMount.  --Trey 2010/08/17
 */
    mount_task_p = NULL;
    mutex_init(&signal_mutex);
    automount_proc = create_proc_entry("tc3162/automount_pid", 0, NULL);
    automount_proc->write_proc = automount_pid_write_proc;
#endif
	SCSI_LOG_HLQUEUE(3, printk("init_sd: sd driver entry point\n"));

	for (i = 0; i < SD_MAJORS; i++)
		if (register_blkdev(sd_major(i), "sd") == 0)
			majors++;

	if (!majors)
		return -ENODEV;

	err = class_register(&sd_disk_class);
	if (err)
		goto err_out;

	err = scsi_register_driver(&sd_template.gendrv);
	if (err)
		goto err_out_class;

	sd_cdb_cache = kmem_cache_create("sd_ext_cdb", SD_EXT_CDB_SIZE,
					 0, 0, NULL);
	if (!sd_cdb_cache) {
		printk(KERN_ERR "sd: can't init extended cdb cache\n");
		goto err_out_class;
	}

	sd_cdb_pool = mempool_create_slab_pool(SD_MEMPOOL_SIZE, sd_cdb_cache);
	if (!sd_cdb_pool) {
		printk(KERN_ERR "sd: can't init extended cdb pool\n");
		goto err_out_cache;
	}

	return 0;

err_out_cache:
	kmem_cache_destroy(sd_cdb_cache);

err_out_class:
	class_unregister(&sd_disk_class);
err_out:
	for (i = 0; i < SD_MAJORS; i++)
		unregister_blkdev(sd_major(i), "sd");
	return err;
}

/**
 *	exit_sd - exit point for this driver (when it is a module).
 *
 *	Note: this function unregisters this driver from the scsi mid-level.
 **/
static void __exit exit_sd(void)
{
	int i;

	SCSI_LOG_HLQUEUE(3, printk("exit_sd: exiting sd driver\n"));

	mempool_destroy(sd_cdb_pool);
	kmem_cache_destroy(sd_cdb_cache);

	scsi_unregister_driver(&sd_template.gendrv);
	class_unregister(&sd_disk_class);

	for (i = 0; i < SD_MAJORS; i++)
		unregister_blkdev(sd_major(i), "sd");
}

module_init(init_sd);
module_exit(exit_sd);

static void sd_print_sense_hdr(struct scsi_disk *sdkp,
			       struct scsi_sense_hdr *sshdr)
{
	sd_printk(KERN_INFO, sdkp, " ");
	scsi_show_sense_hdr(sshdr);
	sd_printk(KERN_INFO, sdkp, " ");
	scsi_show_extd_sense(sshdr->asc, sshdr->ascq);
}

static void sd_print_result(struct scsi_disk *sdkp, int result)
{
	sd_printk(KERN_INFO, sdkp, " ");
	scsi_show_result(result);
}

