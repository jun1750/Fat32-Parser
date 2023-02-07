/**********************************************************************
  Module: fat32.c
  Author: Junseok Lee

 Initializes and manages FAT32 structs.

**********************************************************************/
#define _FILE_OFFSET_BITS 64

#include "fat32.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

fat32Head* createHead(int fd)
{
    /* Allocating head values */
    fat32Head * head = (fat32Head*) malloc(sizeof(fat32Head));
    head->bs = (fat32BS*) malloc(sizeof(fat32BS));
    head->fsi = (FSInfo*) malloc(sizeof(FSInfo));
    head->dir = (fat32Dir*) malloc(sizeof(fat32Dir));
    if(head == NULL || head->bs == NULL || head->fsi == NULL || head->dir == NULL) {
        perror("createHead malloc error:");
        exit(EXIT_FAILURE);
    }

    /* BOOT SECTOR init */
    bootSectorInit(fd, head);
    
    /* check BS signature bytes */
    if(head->bs->BS_BootSig != BS_Ext_BOOT_SIG || head->bs->BS_SigA != BS_SIG_A_VAL || head->bs->BS_SigB != BS_SIG_B_VAL){
        printf("boot sector signature byte error: Drive not loaded correctly.");
        exit(EXIT_FAILURE);
    }
    /* check if drive is not FAT16 */
    if(head->bs->BPB_FATSz16 != FAT32_DEFAULT || head->bs->BPB_TotSec16 != FAT32_DEFAULT || head->bs->BPB_RootEntCnt != FAT32_DEFAULT){
        printf("The drive is FAT16. Only FAT32 is supported.");
        exit(EXIT_FAILURE);
    }

    /* Directory init with the root directory cluster */ 
    dirInit(fd,head,head->bs->BPB_RootClus);

    /* FSInfo init */
    fsiInit(fd, head, head->bs->BPB_FSInfo);

    /* verify directory entry */
    if(head->dir->DIR_Attr > 0x20){
        printf("Error in reading Directory...");
        exit(EXIT_FAILURE);
    }    

    return head;
}
void bootSectorInit(int fd, fat32Head * head){
    
        /* Read boot sector */
        int length = read(fd,(void*)head->bs,sizeof(fat32BS));

		if(length < sizeof(fat32BS)){
			perror("bootSectorInit read error");
			exit(EXIT_FAILURE);
		}      

        /* null terminate strings */
        head->bs->BS_VolLab[BS_VolLab_LENGTH-1] = '\0'; 
        head->bs->BS_FilSysType[BS_FilSysType_LENGTH-1] = '\0';
}

void dirInit(int fd, fat32Head * head, uint32_t dirClusAddr){ 

    int error;

    /* Seek to directory sector */
    error = lseek(fd, dirClusAddr, SEEK_SET);

    if(error == SEEK_ERR){
		perror("bootSectorInit seek error");
		exit(EXIT_FAILURE);
	}   

    /* Read directory sector */
    error = read(fd, (void*)head->dir,sizeof(fat32Dir));
    if(error < sizeof(fat32Dir)){
		perror("bootSectorInit read error");
		exit(EXIT_FAILURE);
	}   

    /* null terminate strings */
    head->dir->DIR_Name[DIR_NAME_LENGTH-1] = '\0';

}
void fsiInit(int fd, fat32Head * head, uint32_t fsiSecNum){
    
    int error;

    /* Seek to FSInfo sector */
    error = lseek(fd, fsiSecNum*head->bs->BPB_BytesPerSec, SEEK_SET);

    if(error == SEEK_ERR){
		perror("fsiInit seek error");
		exit(EXIT_FAILURE);
	}   

    /* Read FSInfo sector */
    error = read(fd, (void*)head->fsi,sizeof(FSInfo));
    
    if(error < sizeof(FSInfo)){
		perror("fsiInit read error");
		exit(EXIT_FAILURE);
	}   

    /* Verifying the read was indeed in the FSInfo sector */
    if(head->fsi->FSI_LeadSig != FSI_LEADSIG || head->fsi->FSI_TrailSig != FSI_TRAILSIG)
    {
        printf("Error in reading FSInfo...");
        printf("\nFSI_TrailSig value: %x, FSI_LeadSig value: %x\n", head->fsi->FSI_TrailSig, head->fsi->FSI_LeadSig);
        exit(EXIT_FAILURE);
    }
}

fat32Dir *readDir(int fd, fat32Head * head, uint32_t secNum, uint32_t dirNum){

    int error;
    fat32Dir *currentDir = (fat32Dir*) malloc(sizeof(fat32Dir));

    if(currentDir == NULL) {
        perror("readDir malloc error:");
        exit(EXIT_FAILURE);
    }

    /* Seek to specified directory sector */
    error = lseek(fd, secNum + dirNum, SEEK_SET);
    
    if(error == SEEK_ERR){
		perror("readDir seek error");
		exit(EXIT_FAILURE);
	} 

    /* Read specified directory sector */
    error = read(fd,(void*)currentDir, sizeof(fat32Dir)); 
        
    if(error < sizeof(fat32Dir)){
		perror("readDir read error");
		exit(EXIT_FAILURE);
	}   

    return currentDir;
}

uint32_t * readFromOffset(int fd, fat32Head * head, uint32_t secNum, uint32_t offset){

    int error;
    uint32_t * buffer = (uint32_t*) malloc(secNum* sizeof(uint32_t));
    if(buffer == NULL) {
        perror("readFromOffset malloc error:");
        exit(EXIT_FAILURE);
    }

    /* Seek to specified sector using sector number and offset */
    error = lseek(fd, secNum*head->bs->BPB_BytesPerSec + offset, SEEK_SET);
        
    if(error == SEEK_ERR){
		perror("readFromOffset seek error");
		exit(EXIT_FAILURE);
	} 

    /* Read specified sector and offset */
    error = read(fd, buffer, OFF_READ_SZ);
            
    if(error < OFF_READ_SZ){
		perror("readFromOffset read error");
		exit(EXIT_FAILURE);
	}  

    return buffer;
}

void cleanupHead(fat32Head *h){
	free(h->bs);
    free(h->dir);
    free(h->fsi);
    free(h);
}