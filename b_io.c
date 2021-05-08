/***************************************************************************************
* Class: CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: b_io.c
*
* Description: This file include 5 main functions that will be called by shell:
*			   b_open, b_read, b_write, b_seek, b_close
*			   b_open: opens a file and initialize fcb
*              b_read: read bytes from a file to caller's buffer
*			   b_write: write bytes from caller's buffer to a file
*			   b_seek: look for a certain position from where to write or read
*			   b_close: close the file and reset fcb
*
*****************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include <stdbool.h>		// for write buffer indicator
#include "fsLow.h"  		// import MINBLOCKSIZE 512
#include "mfs.h"
#include <stdlib.h>
#include "freeSpace.h"

#define MAXFCBS 20
#define GETMOREBLOCKS 10

typedef struct b_fcb
	{
	char * fileName;	// file name
	int fcbStatus;		// -1 means available, 1 means not available
	int readWriteFlags;	// RDONLY, WRONLY, RW
	uint64_t startingLBA;	//the LBA of the first block
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	bool writeBufferNonEmpty;	// track if we need to flush write buffer
	off_t cursor;		// the cursor that tracks # bytes we are at in the file
	off_t fileSize;		// the current size of the file
	blkcnt_t fileBlocksAllocated;	// total number of the blocks that the file allocated (usually allocated a bit more than the file is actually taking)
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

char write_Buffer[MINBLOCKSIZE]; // Stack Allocated Array and Counter


// helper function to initialize our file system
void b_init ()
{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].fcbStatus = -1;
		}
		
	startup = 1;
}


// helper function to get a free FCB element, intialize fcbStatus
int b_getFCB ()
{
	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].fcbStatus == -1)
		{
			fcbArray[i].fcbStatus = 1;
			return i;
		}
	}
	return -1;
}


// helper function to write the last remaning part in buffer to the file (usually called in b_close or b_seek)
int copy_last_write_buffer(int fd) {
	// Copy remaining bytes from the write buffer to the file
	// When the indicator is true, we know that there are some remaining to write
	if (fcbArray[fd].writeBufferNonEmpty) {
		// Convert fileSize to how many blocks the file is taking
		int cursorInDisk = fcbArray[fd].fileSize / MINBLOCKSIZE;
		// check if all allocated blocks have been used
		if (cursorInDisk == fcbArray[fd].fileBlocksAllocated) { // if all have been used
			int res = expandFreeSection(fcbArray[fd].startingLBA, fcbArray[fd].fileBlocksAllocated, fcbArray[fd].fileBlocksAllocated + GETMOREBLOCKS);
			if (res == -1) {
				printf("ERROR: Write failed");
				return -1;
			}
			// Update LBA and blocks allocated in fcb
			fcbArray[fd].startingLBA = res;
			fcbArray[fd].fileBlocksAllocated += 10;	
		}

		LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].startingLBA + cursorInDisk);

		// update
		fcbArray[fd].fileSize += fcbArray[fd].index;
		fcbArray[fd].writeBufferNonEmpty = false;
	}
	// update a few meta info about the file to directory
	setFileSize(fcbArray[fd].fileName, fcbArray[fd].fileSize);
	setFileBlocks(fcbArray[fd].fileName, fcbArray[fd].fileBlocksAllocated);
	setFileLBA(fcbArray[fd].fileName, fcbArray[fd].startingLBA);
	return 0;
}


// Interface to open a buffered file
int b_open (char * filename, int flags)
{
	int fd;
	if (startup == 0) b_init();  //Initialize our system
	
	// check if all 20 spots are used at the same time
	fd = b_getFCB();				// find an available spot, get our own file descriptor
	if (fd  == -1) {
		return -1;						// no spot available, all 20 files are used, not able to open
	}				
	
	// allocate our buffer
	fcbArray[fd].buf = malloc (MINBLOCKSIZE);
	if (fcbArray[fd].buf  == NULL)
	{
		// very bad, we can not allocate our buffer
		fcbArray[fd].fcbStatus = -1; 	// free FCB
		return -1;
	}
	
	// initialize other fcb properties
	fcbArray[fd].fileName = filename; // save the filename
	fcbArray[fd].buflen = 0; 			// have not read anything yet
	fcbArray[fd].index = 0;			// have not read anything yet
	fcbArray[fd].writeBufferNonEmpty = false;	// track if we need to flush write buffer
	fcbArray[fd].cursor = 0;

	// Get file meta data from directory
	int res = getFileLBA(filename, flags);
	// Error check
	if (res == UINT_MAX) {
		printf("ERROR: Failed to open.\n");
		return -1;
	}
	fcbArray[fd].startingLBA = res; 
	fcbArray[fd].fileBlocksAllocated = getBlocks(filename);
	fcbArray[fd].fileSize = getFileSize(filename);

	// O_RDWR: we do not support read currently
	if ((flags & O_ACCMODE) == O_RDWR) {
		printf("Do not support read and write.\n");
		return -1;
	
	// O_RDONLY
	} else if ((flags & O_ACCMODE) == O_RDONLY) {
		fcbArray[fd].readWriteFlags = O_RDONLY;
	
	// O_WRONLY
	} else if ((flags & O_ACCMODE) == O_WRONLY) {
		fcbArray[fd].readWriteFlags = O_WRONLY;
		// Reset the fcb buffer if the file is not empty
		if (fcbArray[fd].fileSize > 0) {
			// Load the LBA block that the cursor is currently at to the buffer
			// and update fcb mata data accordingly
			fcbArray[fd].cursor = fcbArray[fd].fileSize;
			fcbArray[fd].fileSize = fcbArray[fd].fileSize / MINBLOCKSIZE * MINBLOCKSIZE;
			int cursorInDisk = fcbArray[fd].cursor / MINBLOCKSIZE;
			LBAread (fcbArray[fd].buf, 1, fcbArray[fd].startingLBA + cursorInDisk);
			fcbArray[fd].index = fcbArray[fd].cursor - cursorInDisk * MINBLOCKSIZE;
			fcbArray[fd].buflen = 1 * MINBLOCKSIZE;
			fcbArray[fd].writeBufferNonEmpty = true;
		}
	} else {
		printf("Unrecognized access mode.\n");
		return -1;
	}

	return (fd);
}


// Interface to write a buffer	
int b_write (int fd, char * buffer, int count)
	{
	int bytesNeededToFull;		// how many bytes are left in my buffer to fill up the buffer
	int pointer;				// how many bytes have been processed in the call's buffer
	int loadToBuffer;			// how many bytes is going to be load to buffer	
	int cursorInDisk;			// track which block we are writing (this is <= blocks allocated)

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return -1; 					//invalid file descriptor
		}
		
	if (fcbArray[fd].fcbStatus == -1)		//File not open for this descriptor
		{
		return -1;
		}	
	
	// If the file is not for write, stop and return error
	if (fcbArray[fd].readWriteFlags != O_WRONLY) {
		printf("ERROR: No access to write.\n");
		return -1;
	}

	// Initialize the pointer pointing to the first byte in caller's buffer
	pointer = 0;
	

	// Writing to the file from caller's buffer until reaching the end
	while (count > 0) {
		cursorInDisk = fcbArray[fd].cursor / MINBLOCKSIZE;		
		bytesNeededToFull = MINBLOCKSIZE - fcbArray[fd].index;		// number of bytes needed to fill the buffer
		
		// get the number of bytes going to be load to the buffer from caller's buffer
		// it's min(count, bytesNeededToFull)
		if (count < bytesNeededToFull) {
			loadToBuffer = count;		// when the buffer will not be filled up
		} else {
			loadToBuffer = bytesNeededToFull;		// when the buffer will be filled up, then we can write
		}

		// load bytes to buffer from caller's buffer
		memcpy (fcbArray[fd].buf + fcbArray[fd].index, buffer + pointer, loadToBuffer);

		// update a few things
		pointer += loadToBuffer;
		count -= loadToBuffer;
		fcbArray[fd].index += loadToBuffer;

		// write bytes from buffer to the file ONLY when the buffer is full
		if (fcbArray[fd].index == MINBLOCKSIZE) {

			// check if all allocated blocks have been used
			// if yes, call free space manager to get 10 more blocks
			if (cursorInDisk == fcbArray[fd].fileBlocksAllocated) { 
				u_int64_t res = expandFreeSection(fcbArray[fd].startingLBA, fcbArray[fd].fileBlocksAllocated, fcbArray[fd].fileBlocksAllocated + GETMOREBLOCKS);
				if (res == 0) {
					printf("ERROR: Write failed");
					return -1;
				}

				// Update LBA and blocks allocated in fcb
				fcbArray[fd].startingLBA = res;
				fcbArray[fd].fileBlocksAllocated += 10;	
			}

			LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].startingLBA + cursorInDisk);
			fcbArray[fd].cursor += MINBLOCKSIZE;
			fcbArray[fd].fileSize += MINBLOCKSIZE;
			fcbArray[fd].index = 0;

			// Update the directory LBA after getting a new LBA for where the file is located in case of a crash before the b_close
			setFileSize(fcbArray[fd].fileName, fcbArray[fd].fileSize);
			setFileBlocks(fcbArray[fd].fileName, fcbArray[fd].fileBlocksAllocated);
			setFileLBA(fcbArray[fd].fileName, fcbArray[fd].startingLBA);
		}
	}

	// Update indicator to indicate that the write buffer is holding some bytes that haven't been written
	if (fcbArray[fd].index != 0) {
		fcbArray[fd].writeBufferNonEmpty = true;
	}

	return pointer;
	}



// Interface to read a buffer
int b_read (int fd, char * buffer, int count)
	{
	int bytesRead;				// for our reads
	int bytesReturned;			// what we will return
	int part1, part2, part3;	// holds the three potential copy lengths
	int numberOfBlocksToCopy;	// holds the number of whole blocks that are needed
	int remainingBytesInMyBuffer;	// holds how many bytes are left in my buffer	
	int remainingBytesToRead;	// = file size - bytesRead
	int cursorInDisk;			// Which block we are reading from disk

	if (startup == 0) b_init();  //Initialize our system

	// check if fd is valid
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	if (fcbArray[fd].fcbStatus == -1)		//File not open for this descriptor
		{
		return -1;
		}	
		
	// If the file is not for read, stop and return error
	if (fcbArray[fd].readWriteFlags != O_RDONLY) {
		printf("ERROR: No access to read.\n");
		return -1;
	}

	// To handle EOF - reset count as the remaining bytes
	remainingBytesToRead = fcbArray[fd].fileSize - fcbArray[fd].cursor;
	if (count > remainingBytesToRead) {
		count = remainingBytesToRead;
	}
	
	// number of bytes available to copy from buffer
	remainingBytesInMyBuffer = fcbArray[fd].buflen - fcbArray[fd].index;	
	
	// Part 1 is The first copy of data which will be from the current buffer
	// It will be the lesser of the requested amount or the number of bytes that remain in the buffer
	
	if (remainingBytesInMyBuffer >= count)  	//we have enough in buffer
		{
		part1 = count;		// completely buffered (the requested amount is smaller than what remains)
		part2 = 0;
		part3 = 0;			// Do not need anything from the "next" buffer
		}
	else
		{
		part1 = remainingBytesInMyBuffer;
		
		// Part 1 is not enough - set part 3 to how much more is needed
		part3 = count - remainingBytesInMyBuffer;
		
		// The following calculates how many 512 bytes chunks need to be copied to
		// the callers buffer from the count of what is left to copy
		numberOfBlocksToCopy = part3 / MINBLOCKSIZE;
		part2 = numberOfBlocksToCopy * MINBLOCKSIZE; 
		
		// Reduce part 3 by the number of bytes that can be copied in chunks
		// Part 3 at this point must be less than the block size
		part3 = part3 - part2; // This would be equivalent to part3 % MINBLOCKSIZE		
		}

	if (part1 > 0)	// memcpy part 1
		{
		memcpy (buffer, fcbArray[fd].buf + fcbArray[fd].index, part1);
		fcbArray[fd].index = fcbArray[fd].index + part1;
		fcbArray[fd].cursor += part1;
		}
		
	if (part2 > 0) 	//blocks to copy direct to callers buffer
		{
		cursorInDisk = fcbArray[fd].cursor / MINBLOCKSIZE;
		// LBAread always returns 0, we will assume it succeeds
		LBAread (buffer+part1, numberOfBlocksToCopy, fcbArray[fd].startingLBA + cursorInDisk); 
		bytesRead = MINBLOCKSIZE * numberOfBlocksToCopy;
		part2 = bytesRead;
		fcbArray[fd].cursor += part2;
		}
		
	if (part3 > 0)	//We need to refill our buffer to copy more bytes to user
		{
		int cursorInDisk = fcbArray[fd].cursor / MINBLOCKSIZE;
		// Read 1 block into our buffer
		// LBAread always returns 0, we will assume it succeeds
		LBAread (fcbArray[fd].buf, 1, fcbArray[fd].startingLBA + cursorInDisk);
		bytesRead = MINBLOCKSIZE * 1;

		// update
		fcbArray[fd].index = 0;
		fcbArray[fd].buflen = bytesRead; 

		if (bytesRead < part3)
			part3 = bytesRead;
			
		if (part3 > 0)
			{
			memcpy (buffer+part1+part2, fcbArray[fd].buf + fcbArray[fd].index, part3);
			fcbArray[fd].index = fcbArray[fd].index + part3;
			}
		fcbArray[fd].cursor += part3;	
		}
	bytesReturned = part1 + part2 + part3;
	return (bytesReturned);	
	}


// Interface to seek a specific position in file, we will move the cursor accordingly
int b_seek(int fd, off_t offset, int whence){
	switch (whence)
	{
	// Set the cursor equal to the offset
	case SEEK_SET:
		// Error check: offset out of the range
		if (offset > fcbArray[fd].fileSize) {
			printf("ERROR: offset is invalid.\n");
			return -1;
		}
		fcbArray[fd].cursor = offset;
		break;
	
	// Set the cursor offset-bytes away from the current position
	case SEEK_CUR:
		// Error check: offset out of the range
		if (fcbArray[fd].cursor + offset > fcbArray[fd].fileSize) {
			printf("ERROR: offset is invalid.\n");
			return -1;
		}
		fcbArray[fd].cursor += offset;	
		break;
	
	// Set the cursor offset-bytes away from the end of the file
	case SEEK_END:
		// Error check: offset out of the range
		if (fcbArray[fd].fileSize + offset > fcbArray[fd].fileSize) {
			printf("ERROR: offset is invalid.\n");
			return -1;
		}
		fcbArray[fd].cursor = fcbArray[fd].fileSize + offset;
		break;
	}

	// If there's remaining bytes in write buffer, write to LBA
	if (fcbArray[fd].readWriteFlags == O_WRONLY) {
		if (copy_last_write_buffer(fd) < 0) {
			printf("ERROR: seek failed.\n");
			return -1;
		};
	}

	// Load the block that the cursor is currently at from LBA to the buffer
	int cursorInDisk = fcbArray[fd].cursor / MINBLOCKSIZE;
	LBAread (fcbArray[fd].buf, 1, fcbArray[fd].startingLBA + cursorInDisk);
	fcbArray[fd].index = fcbArray[fd].cursor - cursorInDisk * MINBLOCKSIZE;
	fcbArray[fd].buflen = 1 * MINBLOCKSIZE;
	
	return offset;
}


// Interface to Close the file	
void b_close (int fd)
{
	if (copy_last_write_buffer(fd) < 0) {
		printf("ERROR: close failed.\n");
	};

	// reset fcb values
	fcbArray[fd].buflen = 0;
	fcbArray[fd].index = 0;
	fcbArray[fd].fileBlocksAllocated = 0;
	fcbArray[fd].fileSize = 0;
	fcbArray[fd].cursor = 0;

	free (fcbArray[fd].buf);			// free the associated buffer
	fcbArray[fd].buf = NULL;			// Safety First
	fcbArray[fd].fcbStatus = -1;			// return this FCB to list of available FCB's 
}