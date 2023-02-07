/**********************************************************************
   Module: shell.c
   Author: Junseok Lee

   Manages the shell command line. Supports commands INFO,
   DIR, CD and GET. The shell terminates when EOF signal 
   (CTRL + D) is received.

**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "shell.h"
#include "fat32.h"
#include <stdbool.h>

#define CMD_INFO "INFO"
#define CMD_DIR "DIR"
#define CMD_CD "CD"
#define CMD_GET "GET"
#define CMD_PUT "PUT"


void shellLoop(int fd) 
{
	int running = true;
	uint32_t curDirClus;
	char buffer[BUF_SIZE];
	char bufferRaw[BUF_SIZE];

	fileDesc = fd;

	fat32Head *h = createHead(fd);

	if (h == NULL)
		running = false;

	curDirClus = h->bs->BPB_RootClus;

	while(running) 
	{
		printf(">");
		if (fgets(bufferRaw, BUF_SIZE, stdin) == NULL) 
		{
			running = false;
			continue;
		}
		bufferRaw[strlen(bufferRaw)-1] = '\0'; /* cut new line */
		for (int i=0; i < strlen(bufferRaw)+1; i++)
			buffer[i] = toupper(bufferRaw[i]);
	
		//INFO
		if (strncmp(buffer, CMD_INFO, strlen(CMD_INFO)) == 0)
			printInfo(h);	

		//DIR
		else if (strncmp(buffer, CMD_DIR, strlen(CMD_DIR)) == 0)
			doDir(h, curDirClus, DODIR_FIRSTRUN);	


		//CD
		else if (strncmp(buffer, CMD_CD, strlen(CMD_CD)) == 0) 
			curDirClus = doCD(h, curDirClus, buffer);

		//GET
		else if (strncmp(buffer, CMD_GET, strlen(CMD_GET)) == 0) 
			doDownload(h, curDirClus, buffer);

		//PUT (BONUS, IGNORE)
		else if (strncmp(buffer, CMD_PUT, strlen(CMD_PUT)) == 0)			
			printf("Bonus marks!\n");

		else 
			printf("\nCommand not found\n");
	}
	printf("\nExited...\n");
	
	cleanupHead(h);
}

void printInfo(fat32Head *h){

	if(h == NULL) printf("failed print");
	else{

		/* --- Device Info ---- */

		printf("\n---- Device Info ----");

		printf("\nOEM Name: %s", h->bs->BS_OEMName);

		printf("\nLabel: %s", h->bs->BS_VolLab);

		printf("\nFile System Type: %s", h->bs->BS_FilSysType); 

		printf("\nMedia Type: 0x%X", h->bs->BPB_Media);

		//fixed vs removable media 
    	if(h->bs->BPB_Media == MEDIA_FIXED){
        	printf(" (fixed)");
    	}
		else if(h->bs->BS_DrvNum == MEDIA_REMVBLE){
        	printf(" (removable)\n");
    	}		
		
		/* DETERMINE THE COUNT OF SECTORS IN THE DATA REGION, ensure it's in FAT32 range */

		//count of sectors occupied by root directory
		uint32_t rootDirSectors = ((h->bs->BPB_RootEntCnt * INT_32) + (h->bs->BPB_BytesPerSec -1)) / h->bs->BPB_BytesPerSec;

		//count of sectors in the data region
		uint32_t dataSec = h->bs->BPB_TotSec32 - (h->bs->BPB_RsvdSecCnt + (h->bs->BPB_NumFATs * h->bs->BPB_FATSz32) + rootDirSectors);
		
		//count of clusters 
		uint32_t countOfClusters = dataSec / h->bs->BPB_SecPerClus;

		if(countOfClusters < FAT12_NUMCLUS){
			printf("volume is FAT12. The supported volume is FAT32.");
			exit(EXIT_FAILURE);
		}
		else if(countOfClusters < FAT16_NUMCLUS){
			printf("volume is FAT16. The supported volume is FAT32.");
			exit(EXIT_FAILURE);
		}

		/* Calculate total size in Bytes, GB and MB */ 		
		uint64_t totSizeB = (uint64_t) h->bs->BPB_TotSec32 * (uint64_t) h->bs->BPB_BytesPerSec; //total size in bytes 
		int totSizeMB =  totSizeB / M_UNIT;
		double totSizeGB = (double)totSizeB / G_UNIT;  

		printf("\nSize: %lu bytes (%dMB, %.4gGB)",totSizeB, totSizeMB, totSizeGB); 

		printf("\nDrive Number: %u", h->bs->BS_DrvNum);

		/* Drive type determination */ 
    	if(h->bs->BS_DrvNum == HARD_DRV){
        	printf(" (hard disk)\n");
    	}
		else if(h->bs->BS_DrvNum == FLOPPY_DRV){
        	printf(" (floppy disk)\n");
    	}
		/* --- Device Info END ---- */


		/* --- Geometry ---- */
		printf("\n\n---- Device Info ----");
		printf("\nBytes per Sector: %u", h->bs->BPB_BytesPerSec);
		printf("\nSectors per Cluster: %u", h->bs->BPB_SecPerClus);
		printf("\nTotal Sectors: %u", h->bs->BPB_TotSec32);
		printf("\nGeom: Sectors per Track: %u", h->bs->BPB_SecPerTrk);
		printf("\nGeom: Heads: %u", h->bs->BPB_NumHeads);
		printf("\nHidden Sectors: %u", h->bs->BPB_HiddSec);
		/* --- Geometry END ---- */


		/* --- FS Info ---- */
		printf("\n\n---- FS Info ----");
		printf("\nVolume ID: %s", getVolumeID(h)); 
		printf("\nVersion: %u:%u", h->bs->BPB_FSVerHigh,h->bs->BPB_FSVerLow);
		printf("\nReserved Sectors: %u", h->bs->BPB_RsvdSecCnt);
		printf("\nNumber of FATs: %u", h->bs->BPB_NumFATs);
		printf("\nFAT Size: %u", h->bs->BPB_FATSz32);
		
		/*Checking Mirroed bit */
		/*If 7th bit is 0 is yes, 1 means no (only one FAT active) */
		uint16_t mirBit = ((h->bs->BPB_ExtFlags) >> MIR_CHECK_BIT) & NOT_MIR;

		printf("\nMirrored FAT: %u", mirBit);
		if ((mirBit) == IS_MIR){
			printf(" (yes)");
		}
		else if((mirBit) == NOT_MIR){
			printf(" (no)");
		}
		
		printf("\nBoot Sector Backup Sector No: %u\n", h->bs->BPB_BkBootSec);
		/* --- FS Info END ---- */		

	} 
}

char * getVolumeID(fat32Head *h){
	uint32_t firstSecOfClus = getFirstSectorOfClus(h,h->bs->BPB_RootClus);
	fat32Dir * firstDir =readDir(fileDesc,h,firstSecOfClus,FIRST_DIR_INDEX);
	char * volID = h->dir->DIR_Name;

	if(firstDir != NULL){
		strcpy(volID,firstDir->DIR_Name);
		free(firstDir);
	}
	
	return volID;


}

void doDir(fat32Head *h, uint32_t curDirClus, int firstRun){

	int i, j;
	int initial = firstRun;

	if(firstRun) {
		printf("DIRECTORY LISTING\n");
	}
	/* Get first data sector address of cluster*/
	uint32_t firstSecOfClus = getFirstSectorOfClus(h,curDirClus);	

	/* Traversing each directory in every cluster*/
	for(i=0; i < h->bs->BPB_SecPerClus; i+= h->bs->BPB_BytesPerSec){

		for(j=0; j< h->bs->BPB_BytesPerSec; j+=sizeof(fat32Dir)){ 

			/* Get current directory in current sector */
			fat32Dir * curDir;
			curDir = readDir(fileDesc,h,firstSecOfClus+i,j);

			/* remove white space & null terminate string of directory name */
			formatDirectory(curDir);

			/* show volume ID in the initial run */
			if(initial){
				printf("VOL_ID: %s\n", getVolumeID(h));
				initial = false;
			}

			/* Exit if current directory address is zero */
			if(curDir->DIR_Name[0] == ADDR_ZERO){
				if(curDir) {
					free(curDir);
				}
           	 	break;
        	}

			/* current directory is a DIRECTORY */
			if((curDir->DIR_Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY && curDir->DIR_Name[0] != FREE_DIR && 
					curDir->DIR_Name[0] != KANJI_DIR && checkName(curDir->DIR_Name)){
			            
				char *dirName = curDir->DIR_Name;

				if(curDir->DIR_Attr == ATTR_DIRECTORY || curDir->DIR_FileSize == ADDR_ZERO){
					printf("<%s>\t\t%u\n",dirName, curDir->DIR_FileSize);
				}
				else{
					//special types of files
					printf("%s\t\t%u\n", curDir->DIR_Name ,curDir->DIR_FileSize);
				}
			}
			/* current directory is a FILE */
			else if ((curDir->DIR_Attr & ATTR_ARCHIVE) == ATTR_ARCHIVE && curDir->DIR_Name[0] != FREE_DIR && 
						curDir->DIR_Name[0] != KANJI_DIR && checkName(curDir->DIR_Name)) {

				if (curDir->DIR_Attr == ATTR_ARCHIVE){
                    printf("%s\t\t%u\n", curDir->DIR_Name ,curDir->DIR_FileSize);
                }
				
				//support for valid file types with attributes not equal to ATTR_ARCHIVE
				else if(curDir[i].DIR_Attr != ADDR_ZERO && curDir[i].DIR_Attr != ATTR_READ_ONLY && 
							curDir[i].DIR_Attr != ATTR_HIDDEN && curDir[i].DIR_Attr != ATTR_SYSTEM && 
							curDir[i].DIR_Attr != ATTR_VOLUME_ID && curDir[i].DIR_Attr != ATTR_DIRECTORY && 
							curDir[i].DIR_Attr != INV_DIR && curDir[i].DIR_Attr != INV_ARCHIVE){

						printf("%s\t\t%u\n",curDir->DIR_Name, curDir->DIR_FileSize);

				}
            }

			/* below is not needed for our implementation, but added for extra file name support */
			/* Special Cases: Long Name */
			else if ((curDir[i].DIR_Name[0] != FREE_DIR) && checkName(curDir->DIR_Name) && 
            			(curDir[i].DIR_Attr == ATTR_READ_ONLY || curDir[i].DIR_Attr == ATTR_DIRECTORY || 
				 		 curDir[i].DIR_Attr == ATTR_ARCHIVE || curDir[i].DIR_Attr == ATTR_HIDDEN || curDir->DIR_Attr == V_ARCHIVE)){ 
				
				printf("%s\t\t%u\n",curDir->DIR_Name, curDir->DIR_FileSize);

			}
			/* Extraneous valid special cases */
			else if(curDir[i].DIR_Attr != ADDR_ZERO && curDir[i].DIR_Attr != ATTR_READ_ONLY && 
					curDir[i].DIR_Attr != ATTR_HIDDEN && curDir[i].DIR_Attr != ATTR_SYSTEM &&
					curDir[i].DIR_Attr != ATTR_VOLUME_ID && curDir[i].DIR_Attr != ATTR_DIRECTORY && 
					curDir[i].DIR_Attr != INV_DIR && curDir[i].DIR_Attr != INV_ARCHIVE && checkName(curDir->DIR_Name)){
				
				printf("%s\t\t%u\n",curDir->DIR_Name, curDir->DIR_FileSize);

			}

			/* deallocate current directory */
			if(curDir != NULL) free(curDir);
    	}

		/* Search File allocation table for next cluster chain, if exists */
		uint32_t nextClus = getNextClus(h,curDirClus);

		/* Recursively move on to the next cluster */
		if (nextClus < EOC) {
			doDir(h, nextClus,DODIR_NONFIRSTRUN);
		}

		/* Print Amount of bytes free on the first run */
		if(firstRun) {
			printf("----Bytes Free: %lu \n", getFreeSpace(h)); 
			printf("----DONE\n");
		}

	}

}

uint32_t doCD(fat32Head *h, uint32_t curDirClus, char buffer[BUF_SIZE]){

	bool isRoot = false;
	char preFmtBuf[BUF_SIZE];
	
	strcpy(preFmtBuf, buffer);

	/* token out the argument file name from the buffer */
	char* arg = strtok(preFmtBuf, SPACE_CHAR);
	arg = strtok(NULL, SPACE_CHAR);

	/* to use as root directory check */
	isRoot = (curDirClus == h->bs->BPB_RootClus);

	/* no directory specified */
	if(arg == NULL){
		printf("Error: folder not found\n");
		return curDirClus;
	}

	/* Get first data location of cluster */
	uint32_t firstSecOfClus = getFirstSectorOfClus(h,curDirClus);

	int i,j;
	for(i=0; i < h->bs->BPB_SecPerClus; i+= h->bs->BPB_BytesPerSec){
		
		for(j=0; j< h->bs->BPB_BytesPerSec; j+=sizeof(fat32Dir)){
			/* read each sector for directory */
			fat32Dir * dir;
			dir = readDir(fileDesc,h,firstSecOfClus+i,j);

			/* Exit if current directory address is zero */
			if(dir->DIR_Name[0] == ADDR_ZERO){
				if(dir) {
					free(dir);
				}
				break;
			}

			/* remove white space & null terminate string of directory name */
			formatDirectory(dir);

			if(dir->DIR_Attr == ATTR_DIRECTORY){
				
				/* cd "." */
				if(strcmp(arg, DOT) == 0){
					//current directory
					if(!isRoot){
						free(dir);
						return curDirClus;
					}
					else{
						printf("Error: Folder not found.\n");
						free(dir);
						return curDirClus;
					}
					
					
				}
				/* cd ".." */
				else if(strcmp(dir->DIR_Name, DOTDOT ) == 0 && strcmp(dir->DIR_Name,arg) == 0){
					if(!isRoot){
						/*previous directory*/

						if(dir->DIR_FstClusLO == ADDR_ZERO && dir->DIR_FstClusHI == ADDR_ZERO){
							free(dir);
							return ROOT_DIR_CLUS_NUM;
						}

						/* move to directory stored in ".." */
						uint32_t newClus = ADDR_ZERO;
						newClus = newClus | dir->DIR_FstClusHI << HEX_TEN;
						newClus = newClus | dir->DIR_FstClusLO;
						free(dir);

						return newClus;
					}
					else{
						printf("Error: Folder not found.\n");
						return curDirClus;
					}
				}
				else if(strcmp(dir->DIR_Name, arg) == 0)
				{
					/* FOUND DIRECTORY MATCH, 	
					move to directory stored in the directory */ 
					uint32_t newClus = ADDR_ZERO;
					newClus = newClus | dir->DIR_FstClusHI << 16;
					newClus = newClus | dir->DIR_FstClusLO;
					free(dir);
					
					return newClus;
				}
							
			}
			free(dir);

		}
	}

	/* Fetch next cluster */
	uint32_t nextClus = getNextClus(h,curDirClus);

	if (nextClus < EOC) {
		nextClus = doCD(h, nextClus,buffer);
		return nextClus;
	}
	
	return curDirClus;

}

void doDownload(fat32Head *h, uint32_t curDirClus, char buffer[BUF_SIZE]){

	/* acquire GET file name argument */
	char* arg = strtok(buffer, SPACE_CHAR);
	arg = strtok(NULL, SPACE_CHAR);

	/* no directory specified */
	if(arg == NULL){
		printf("Error: File not found\n");
		return;
	}

	size_t check;
	fat32Dir * dir = h->dir;
	uint32_t firstSecOfClus = getFirstSectorOfClus(h,curDirClus);

	/* set seek to first sector of cluster */
	check = lseek(fileDesc, firstSecOfClus, SEEK_SET); 
	    if(check == SEEK_ERR){
		perror("doDownload seek error");
		exit(EXIT_FAILURE);
	}   
	
	int i;
	for(i=0; i < h->bs->BPB_BytesPerSec; i+=sizeof(fat32Dir)){

		/* read in current directory */
		check = read(fileDesc, dir, sizeof(fat32Dir));
		if( check < SINGLE_READ){
			perror("doDownload read error");
			exit(EXIT_FAILURE);
		}

		/* process file name */
		formatDirectory(dir); 

		if(dir->DIR_Attr != ATTR_DIRECTORY){
			
			if(strcmp(dir->DIR_Name, arg) == 0)
			{
				/* FOUND FILE MATCH */
				
				/* get file cluster number */
				uint32_t newClus = ADDR_ZERO;
				newClus = newClus | dir->DIR_FstClusHI << HEX_TEN;
				newClus = newClus | dir->DIR_FstClusLO;
				
				/*  file cluster number found, download the file */

				//destination file name			
				char * dest = arg; 

				/*open output file to read/write,
				  creating file if file DNE*/
				int outFD = open(dest, O_CREAT|O_RDWR|O_TRUNC, FILE_PERMISSION);  

				if(outFD == -1){
					perror("Out file descriptor error");
					exit(EXIT_FAILURE);
				}

				writeFile(h,newClus,outFD,dir->DIR_FileSize);

				close(outFD);

				printf("\nDone.\n");
				return;
			}					 
		}
	}
	printf("Error: File not found\n");

}

void writeFile(fat32Head *h, uint32_t clusNum, int outFD, int fileSize){

	/* Stop writing File if end of data is reached */  		
	if(fileSize < 0) return;

	size_t error;	
	uint32_t bytesPerClus = h->bs->BPB_SecPerClus * h->bs->BPB_BytesPerSec; /* cluster size in bytes */
	uint32_t firstSecOfClus = getFirstSectorOfClus(h,clusNum);
	
	/* byte array to write memory, allocated to cluster size */
	uint32_t * str = malloc(bytesPerClus); 
						
	if(str == NULL){
		perror("writeFile malloc error");
		exit(EXIT_FAILURE);
	}	
					
	/* seek to the data portion of cluster */
	error = lseek(fileDesc, (off_t)firstSecOfClus, SEEK_SET);

	if( error == -1 ){
		perror("writefile seek");
		exit(EXIT_FAILURE);
	}
					
	/* last cluster, only write remaining data, 
	   instead of the whole cluster */
	if(fileSize <= bytesPerClus){

		/* read in current cluster, only the remaining bytes
		   Instead of passing whole cluster size as the size, pass 
		   in the remaining data in the last cluster as the size */
		error = read(fileDesc,(void*)str, fileSize);

		if(error == -1){
			perror("writefile read error");
			exit(EXIT_FAILURE);
		}

		//write current cluster with size of remaining bytes	
		error = write(outFD, (void*)str, fileSize); 							

		if(error == -1){
			perror("writefile write error");
			exit(EXIT_FAILURE);
		}

		//deallocate byte array
		free(str);

		return;

	}

	/* Not Last Cluster, so read data in the entire current cluster */ 
    error = read(fileDesc,(void*)str, bytesPerClus);

	if(error == -1){
		perror("writefile read error");
		exit(EXIT_FAILURE);
	} 

	/* write data in the current cluster */ 
	error = write(outFD, (void*)str, bytesPerClus); 


	if(error == -1){
		perror("writefile write error");
		exit(EXIT_FAILURE);
	}

	//deallocate byte array					
	free(str);

	/* Fetch next cluster */
	uint32_t nextClus = getNextClus(h,clusNum);

	/* Recursively write the file again for the next cluster */
	if (nextClus < EOC) { 	
		writeFile(h, nextClus, outFD, fileSize-bytesPerClus);
	}
			
}

uint32_t getNextClus(fat32Head *h, uint32_t clusNum){
	
	uint32_t fatOffset = clusNum * OFFSET_MULTIPLIER;
	uint32_t thisFatSecNum  = h->bs->BPB_RsvdSecCnt +  (fatOffset / h->bs->BPB_BytesPerSec);
	uint32_t thisFatEntOffset = fatOffset % h->bs->BPB_BytesPerSec;

	uint32_t * buffer = readFromOffset(fileDesc,h, thisFatSecNum, thisFatEntOffset);
			
	uint32_t nextClus = buffer[0];
	nextClus = nextClus & CLUSENT_AND_OPERATOR;
	nextClus = nextClus & FETCH_AND_OPERATOR;
	nextClus = nextClus | (buffer[0] & CLUSENT_AND_OPERATOR);

	if(buffer!= NULL) free(buffer);

	return nextClus;
}

uint32_t getFirstSectorOfClus(fat32Head *h, uint32_t clusterNumber){
	
	uint32_t rootDirSectors = ((h->bs->BPB_RootEntCnt * INT_32) + (h->bs->BPB_BytesPerSec - 1)) / h->bs->BPB_BytesPerSec;
	uint32_t firstDataSector = h->bs->BPB_RsvdSecCnt + (h->bs->BPB_NumFATs * h->bs->BPB_FATSz32) + rootDirSectors;
	uint32_t firstSectorOfCluster = ((clusterNumber - 2) * h->bs->BPB_SecPerClus) + firstDataSector;
	
	return firstSectorOfCluster * h->bs->BPB_BytesPerSec;
}

void formatDirectory(fat32Dir* dir)
{
	//DIRECTORIES
	if (dir->DIR_Attr == ATTR_DIRECTORY){

		char * newName = dir->DIR_Name;
		int i,j;

		//Copy name exclusing space
		for(i=0,j=0; i<DIR_NAME_LENGTH; i++,j++){
			if(!isspace(dir->DIR_Name[i])){ 

				newName[j] = dir->DIR_Name[i];
			}
			else{
				j--;
			}
		}
		newName[j] = '\0';

		//remove remaining spaces as precautionary measure
		removeSpace(newName, dir->DIR_Name);
	}

	//FILES
	else if (dir->DIR_Attr == ATTR_ARCHIVE){
		
		//replace first space with dot and remove the remaining spaces
		char *space = strstr(dir->DIR_Name,SPACE_CHAR);
		if (space && isspace(*space) ){
			*space = '.';
		}

		//space not found in the length of name
		if(space >= dir->DIR_Name + DIR_NAME_LENGTH){
			char * src = dir->DIR_Name;
			addDot(src, dir->DIR_Name);
			*(space+1) = '\0';			
		}	

		char * src = dir->DIR_Name;
		removeSpace(src, dir->DIR_Name);

		//add null terminator
		if(space && space+EXT_DIST != NULL){
			*(space+EXT_DIST) = '\0';
		}
    }
	
	else {
		//replace space with dot
		char *space = strstr(dir->DIR_Name,SPACE_CHAR);
		if ((space) && isspace(*space) && isalpha(*(space+1))){
			if(space <= (dir->DIR_Name + DIR_NAME_LENGTH-1))
				*space = '.';
			else{
				char * src = dir->DIR_Name;
				addDot(src, dir->DIR_Name);
			}
				
		}
				
		//replace next space with null terminator
		space = strstr(dir->DIR_Name,SPACE_CHAR);
		if ((space) && isspace(*space)){
			*space = '\0';
		}

		//add null terminator
		if(space && space+EXT_DIST != NULL){
			*(space+EXT_DIST) = '\0';
		}
		
	}

}

void removeSpace(char* source, char* dest){
	 do {
        while (isspace(*dest)) {
            ++dest;
        }
    }while ((*source++ = *dest++));
}

void addDot(char* source, char* dest){
	 int len =  strlen(source);
	 int i = len-EXT_DIST+1;
	 char cur = '.', temp;

	 if(len > DIR_NAME_LENGTH) i = DIR_NAME_LENGTH-EXT_DIST+1;
	
	 for(; i< len+1; i++){
		 temp = source[i];
		 source[i] = cur;
		 cur = temp;
	 }
}

uint64_t getFreeSpace(fat32Head * h){

	uint32_t numFreeClus = h->fsi->FSI_Free_Count;
	uint64_t freeSecBytes = 0;

	if(numFreeClus == FREE_CLUS_UNKNOWN){
		printf("error: free count unknown\n");
		return freeSecBytes;
	}
	
	else{
		uint64_t totFreeSec = (uint64_t) numFreeClus* (uint64_t)h->bs->BPB_SecPerClus;
		freeSecBytes =  totFreeSec * (uint64_t) h->bs->BPB_BytesPerSec;
		return freeSecBytes;
	}
}

int checkName(char s[])
{
    unsigned char cur;

    while (( cur = *s ) && (isalnum(cur) || (cur >= VALID_ASCII_START && cur <= VALID_ASCII_END)) ) ++s;

    return *s == '\0'; 
}



