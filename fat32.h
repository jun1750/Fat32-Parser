/**********************************************************************
  Module: fat32.h
  Author: Junseok Lee

  Purpose: Initializes and manages FAT32 structs. Reads in data to
  the fat32Head struct, fat32BS_struct, FSInfo_struct and 
  fat32Dir_struct. 

**********************************************************************/
#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>
#include <stdio.h>

/* boot sector constants */
#define BS_OEMName_LENGTH 8
#define BS_VolLab_LENGTH 11
#define BS_FilSysType_LENGTH 8 
#define BS_Ext_BOOT_SIG 0x29
#define BS_SIG_A_VAL 0x55
#define BS_SIG_B_VAL 0xAA
#define FAT32_DEFAULT 0x00

/* directory sector constants */
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define FREE_DIR   	   0xE5
#define KANJI_DIR      0x05
#define V_ARCHIVE      0x4C
#define INV_DIR	       0xF
#define INV_ARCHIVE    0x22

/* FSInfo sector constants */
#define FSI_Reserved1_LENGTH 480
#define FSI_Reserved2_LENGTH 12
#define FSI_LEADSIG 0x41615252
#define FSI_TRAILSIG 0xAA550000

/* fat32Dir  constants */
#define DIR_NAME_LENGTH 11
#define ROOT_DIR_CLUS_NUM 2

/* Cluster constants */
#define EOC 0x0FFFFFF8
#define CLUSENT_AND_OPERATOR 0x0FFFFFFF
#define FETCH_AND_OPERATOR 0xF0000000
#define FREE_CLUS_UNKNOWN 0xFFFFFFFF

/* Offset constants */
#define OFF_READ_SZ 32

/* Seek/Read error constants */
#define SEEK_ERR -1
#define READ_ERR -1

#pragma pack(push)
#pragma pack(1)

/* FAT32BS STRUCT,
   Represents a FAT32 Boot Sector */
struct fat32BS_struct {
	char BS_jmpBoot[3];	//Jump instruction to boot code.
	char BS_OEMName[BS_OEMName_LENGTH];
	uint16_t BPB_BytesPerSec; //Count of bytes per sector
	uint8_t BPB_SecPerClus; //Number of sectors per allocation unit.
	uint16_t BPB_RsvdSecCnt; //Number of reserved sectors in the Reserved region of the volume
	uint8_t BPB_NumFATs;  //count of FAT data structures on the volume
	uint16_t BPB_RootEntCnt; //FAT32 volumes, this field set to 0
	uint16_t BPB_TotSec16;  //# of 16-bit count of sectors. FAT32 volumes, this field set to 0
	uint8_t BPB_Media;	//type of meida of the volume
	uint16_t BPB_FATSz16;  //FAT size 16-bit. FAT32 volumes, this field set to 0
	uint16_t BPB_SecPerTrk; //Sectors per track
	uint16_t BPB_NumHeads; //Number of heads for interrupt 0x13
	uint32_t BPB_HiddSec; //Count of hidden sectors
	uint32_t BPB_TotSec32; //total count of sectors on the volume
	uint32_t BPB_FATSz32; //FAT32 32-bit count of sectors occupied by ONE FAT
	uint16_t BPB_ExtFlags; //External flags defined in FAT32
	uint8_t BPB_FSVerLow; //Low byte: minor revision number
	uint8_t BPB_FSVerHigh; //High byte: major revision number
	uint32_t BPB_RootClus; //cluster number of the first cluster of the root directory
	uint16_t BPB_FSInfo; //Sector # of FSINFO struct in the resvd area of the FAT32 volume (usually 1).
	uint16_t BPB_BkBootSec; //sector number in the rsvd area of the volume of a copy of the boot record.
	char BPB_reserved[12]; //Reserved for future expansion
	uint8_t BS_DrvNum;  //drive number (hard disk or floppy disk)
	uint8_t BS_Reserved1; //Reserved for windows use
	uint8_t BS_BootSig; //extended boot signature
	uint32_t BS_VolID;	//volume serial number
	char BS_VolLab[BS_VolLab_LENGTH]; //volume label
	char BS_FilSysType[BS_FilSysType_LENGTH]; //file system type, info only
	char BS_CodeReserved[420];	//reserved code
	uint8_t BS_SigA;  //boot sector sig A
	uint8_t BS_SigB;  //boot sector sig B
};
#pragma pack(pop)

typedef struct fat32BS_struct fat32BS;


#pragma pack(push)
#pragma pack(1)

/* FAT32 Head struct, contains a pointer 
   to the boot sector struct, FSinfo sector
   struct and the directory struct. */
struct fat32Head {

	struct fat32BS_struct * bs;
	struct FSInfo_struct * fsi;
	struct fat32Dir_struct * dir;

};
#pragma pack(pop)
typedef struct fat32Head fat32Head;


/* FSINFO Struct
   Represents a FAT32 FSInfo Sector*/
#pragma pack(push)
#pragma pack(1)
struct FSInfo_struct {

	uint32_t FSI_LeadSig; //Signature (0x41615252)
	unsigned char FSI_Reserved1[FSI_Reserved1_LENGTH];
	uint32_t FSI_StrucSig; //Signature (0x61417272)
	uint32_t FSI_Free_Count; //Number of free clusters
	uint32_t FSI_Nxt_Free; //represents the next free cluster (assigned to last allocated cluster)
	unsigned char FSI_Reserved2[FSI_Reserved2_LENGTH];
	uint32_t FSI_TrailSig; //Signature (0xAA550000)

};
#pragma pack(pop)

typedef struct FSInfo_struct FSInfo;


/* DIrectory Struct
   Represents a FAT 32 directory */
#pragma pack(push)
#pragma pack(1)
struct fat32Dir_struct {
	char DIR_Name[DIR_NAME_LENGTH];	//8 bits + 3 chars
	uint8_t DIR_Attr; ///File attributes: ATTR_READ_ONLY = 0x01, ATTR_HIDDEN = 0x02, ATTR_SYSTEM = 0x04, ...
	uint8_t DIR_NTRes;	//8 bits used by windows NT.
	uint8_t DIR_CrtTimeTenth;	//millisecond stamp @ file creation time
	uint16_t DIR_CrtTime; //time file created (24bits)
	uint16_t DIR_CrtDate; //date file created (16bits)
	uint16_t DIR_LstAccDate; //last access date (16bits)
	uint16_t DIR_FstClusHI; //high word of this entry's first cluster number
	uint16_t DIR_WrtTime; //time of last write (16bits)
	uint16_t DIR_WrtDate; //date of last write (16bits)
	uint16_t DIR_FstClusLO; //starting cluster # in FAT (file allocation table)
	uint32_t DIR_FileSize; //32-bit DWORD, holds file size in byte (32 bits)
};
#pragma pack(pop)
typedef struct fat32Dir_struct fat32Dir;

/* Initializes a fat32 head using the given file
   descriptor. Allocates the head and initializes its boot
   sector, FSInfo sector and Directory sctor. Verifies that
   each initialization was successful and returns the head. */
fat32Head* createHead(int fd);

/* Reads and initializes the boot sector. */
void bootSectorInit(int fd, fat32Head * head);

/* Reads and initializes the directory sector */
void dirInit(int fd, fat32Head * head, uint32_t dirClusAddr);

/* Reads and initializes the FSInfo sector */
void fsiInit(int fd, fat32Head * head, uint32_t fsiSecNum);

/* Reads a directory using the given sector number (sectorNum),
   and a directory number (dirNum). Returns a malloc'd 
   fat32Dir pointer */
fat32Dir *readDir(int fd, fat32Head * head, uint32_t sectorNum, uint32_t dirNum);

/* Reads a directory using the given sector number (sectorNum),
   and an offseet number (offset). Returns a malloc'd 
   uint32_t pointer */
uint32_t * readFromOffset(int fd, fat32Head * head, uint32_t secNum, uint32_t offset);

/* Deallocates head and all its pointers  */
void cleanupHead(fat32Head *h);

#endif
