/**************************************************************
Class:  CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 
* (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu 
* (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: freeSpace.h
*
* Description: the header file containing all necessary functions
* of the free space, including the ability to find free blocks
* free some blocks, and expand an existing file
*              
*
**************************************************************/
#ifndef _FREESPACE_H
#define _FREESPACE_H

#include <stdlib.h>
#include <stdio.h>
#include "fsLow.h"

typedef struct  {
	int* bitVector; //int bit vector that is the free space
	int size; //the amount of ints in the bit vector
	uint64_t LBABitVector; //LBA location of bit vector
	int blocksNeeded; //blocks needed by the bit vector
	int structSize;
	int blockCount; //amount of blocks vector is accounting for
} freeSpace;
/**
* @parameters 
*   @type int: totalBlocks, amount of blocks in the vcb
*   @type int: bytesPerBlock, how many bytes are in each block for vcb
*@return a pointer to the freeSpace struct that was initialized
* This function initializes the free space struct and bit vector, if there is none in LBA
**/
freeSpace* init_freeSpace(int totalBlocks, int bytesPerBlock); 
/**
* @parameters 
*   @type int: blockCount, amount of consecutive blocks to find that are free
*@return the LBA address of the start of the chunk of free blocks requested
* This function finds the requested amount of free blocks, and returns the address where they're located
**/
u_int64_t findMultipleBlocks(int blockCount);
/**
* @parameters 
*   @type int: startIndex, the beginning of the chunk that needs to be freed
*   @type int: count, amount of blocks to be freed
* This function frees a chunk of blocks as requested by the caller
**/
void freeSomeBits(int startIndex, int count);
/**
* @parameters 
*   @type int: fileLocation, the LBA of the file in question
*   @type int: fileBlockSize, the current size of the file
*   @type int: newBlockSize, the size of the file after it has been expanded
*@return the LBA address of the new location that the file is at
* This function expands the size of the file at the passed in LBA address. If there's not enough space,
* the file's LBA address is moved to somewhere that has the chunk needed.
**/
u_int64_t expandFreeSection(int fileLocation, int fileBlockSize, int newBlockSize);
#endif
