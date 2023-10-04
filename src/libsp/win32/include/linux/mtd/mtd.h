
/* [bombur]: this is a fake! */

#ifndef __MTD_MTD_H__
#define __MTD_MTD_H__

struct erase_info_user {
	unsigned long start;
	unsigned long length;
};

struct mtd_info_user {
	unsigned char type;
	unsigned long flags;
	unsigned long size;	 // Total size of the MTD
	unsigned long erasesize;
	unsigned long oobblock;  // Size of OOB blocks (e.g. 512)
	unsigned long oobsize;   // Amount of OOB data per block (e.g. 16)
	unsigned long ecctype;
	unsigned long eccsize;
};

struct region_info_user {
	unsigned long offset;		/* At which this region starts, from the beginning of the MTD */
	unsigned long erasesize;		/* For this region */
	unsigned long numblocks;		/* Number of blocks in this region */
	unsigned long regionindex;
};


#define MEMGETINFO             1
#define MEMERASE               2
#define MEMWRITEOOB            3
#define MEMREADOOB             4
#define MEMLOCK                5
#define MEMUNLOCK              6
#define MEMGETREGIONCOUNT	   7
#define MEMGETREGIONINFO	   8


#endif /* __MTD_MTD_H__ */
