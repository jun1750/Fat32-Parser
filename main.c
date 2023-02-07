/**********************************************************************
   Module: main.c
   Author: Junseok Lee

   Reads and opens the FAT32 volume image. Executes the shell loop.
   Usage: ./fat32 <fat32_volume>

**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "shell.h"

int main(int argc, char *argv[]) 
{
	int fd;

	if (argc != 2) 
	{
		printf("Usage: %s <file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *file = argv[1];
 	fd = open(file, O_RDWR);
	 
	if (-1 == fd) 
	{
		perror("opening file: ");
		exit(EXIT_FAILURE);
	}

	shellLoop(fd);

	close(fd);
}
