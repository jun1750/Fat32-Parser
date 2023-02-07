/**********************************************************************
   Module: shell.h
   Author: Junseok Lee

   Purpose: Manages the shell command line. 
   Prints information using command INFO, lists all contents in the 
   current directory using command DIR, moves to a specified directory 
   using command CD and downloads a specified file using command GET. 
   The shell terminates when EOF signal (CTRL + D) is received.

**********************************************************************/
#ifndef SHELL_H
#define SHELL_H

#include "fat32.h"

#define BUF_SIZE 256
#define BYTE_IN_BITS 8

#define MEDIA_FIXED 0xF8
#define MEDIA_REMVBLE 0xF0

#define INT_32 32
#define FAT12_NUMCLUS 4085
#define FAT16_NUMCLUS 65525

#define MIR_CHECK_BIT 7
#define IS_MIR 0
#define NOT_MIR 1

#define HARD_DRV 0x80
#define FLOPPY_DRV 0x00

#define M_UNIT 1000000 
#define G_UNIT 1000000000 

#define ADDR_ZERO 0x00
#define HEX_TEN 0x10

#define DOT "."
#define DOTDOT ".."

#define SPACE_CHAR " "

#define FIRST_DIR_INDEX 0
#define DODIR_FIRSTRUN 1
#define DODIR_NONFIRSTRUN 0

#define SINGLE_READ 1
#define FILE_PERMISSION 0644

#define OFFSET_MULTIPLIER 4

#define NULL_TERM '\0'
#define EXT_DIST 4
#define VALID_ASCII_START 33
#define VALID_ASCII_END 126

/* Manages the main shell loop. Supports commands INFO,
   DIR, CD and GET. The shell loop ends only when EOF signal 
   (CTRL + D) is received. */
void shellLoop(int fd);

/* Prints Device information, Geometry information and FS Info
   information for the inserted volume */
void printInfo(fat32Head *h);

/* Reads root directory cluster data and acquires volume ID */
char * getVolumeID(fat32Head *h);

/* Manages the dir command. Lists all valid files and directories 
   contained in the current folder */ 
void doDir(fat32Head *h, uint32_t curDirClus, int firstRun);

/* Performs and manages the cd command. Switches to the directory 
   specified in the shell's command line, if the directory exists
   in the shell's current directory. */
uint32_t doCD(fat32Head *h, uint32_t curDirClus, char buffer[BUF_SIZE]);

/* Performs and manages the get command. Finds the matching file in the current
   directory with the file name specified from the command line, and downloads 
   that file to an output file of the same name in the user's current path in 
   their terminal. Uses helper recursive function writeFile to complete the download. */
void doDownload(fat32Head *h, uint32_t curDirClus, char buffer[BUF_SIZE]);

/* Recursive read and write to output file for every cluster.
   When last cluster is reached, it writes using the remaining
   data in that last cluster instead of the size of the cluster.  */
void writeFile(fat32Head *h, uint32_t clusNum, int outFD, int fileSize);

/* Calculates the next cluster of the FAT, by calculating the offset, this sector 
   number in the FAT, this fat ent offset.  */
uint32_t getNextClus(fat32Head *h, uint32_t clusNum);

/* Calculates the location of the first cluster of the FAT, using math and 
   variables given by the FAT32 white paper.  */
uint32_t getFirstSectorOfClus(fat32Head *h, uint32_t clusterNumber);

/* remove white spaces in directory and file names,
   formats file name extensions periods (e.g., .txt), 
   and adds null terminate string of directory name. */
void formatDirectory(fat32Dir* dir);

/* Removes Spaces from the string <source>, and
   stores the result in the string <dest> */
void removeSpace(char* source, char* dest);

/* Adds a dot before the extension in the string from 
   <source>, and stores the result in the string <dest> */
void addDot(char* source, char* dest);

/* Calculates the total space (in bytes) free in the volume. */
uint64_t getFreeSpace(fat32Head * h);

/* Checks directory/file name to ensure the name is valid.
   A name is valid if the name contains alphabets, numbers, or
   is any standard keyboard character in ASCII */
int checkName( char s[] );

/* File descriptor to the opened volume */
int fileDesc;

#endif
