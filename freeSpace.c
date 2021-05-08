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
* File: freeSpace.c
*
* Description: the c file containing code for free space manipulation
* as described in freeSpace.h
*              
*
**************************************************************/

#include "freeSpace.h"
extern freeSpace* vector;

freeSpace* init_freeSpace(int totalBlocks, int BytesPerBlock) {
	vector = malloc(512);
	vector->LBABitVector = ((sizeof(freeSpace) + 511) / 512) + 2; //size of freeSpace struct without the array
	vector->blockCount = totalBlocks;
	int integers = (totalBlocks + 31) / 32; //how many integers in the int array
	int blocksNeeded = ((integers * 32) + BytesPerBlock * 8 - 1) / (BytesPerBlock * 8); //amount of blocks needed for free space bit vector
	blocksNeeded += vector->LBABitVector; //add VCB needed block, and freeSpace struct needed blocks
	vector->size = integers; //length of the bit vector for printing or iterating
	vector->blocksNeeded = (integers + 127) / 128;
	vector->bitVector = malloc(vector->blocksNeeded * 512);
	vector->structSize = 512; //size of freeSpace guaranteed to be less than 1 block
        for (int b = 0; b < integers; b++) {
                vector->bitVector[b] = 0; //sets all of the free space array to 0
        }
	for (int n = 0; n < blocksNeeded; n++) {
		//set bits used by VCB and this bitVector to 1
		unsigned int flag = 1;
		flag = flag << n;
		vector->bitVector[0] |= flag;
	}

	LBAwrite(vector, 1, 2);
	LBAwrite(vector->bitVector, vector->blocksNeeded, vector->LBABitVector);
	return vector;
}

u_int64_t findMultipleBlocks(int blockCount) {
	int consecutiveFree = 0; 						//tracks how many free spaces have been found in a row
	int freeIndex = 0; 							//the bit index that is free, followed by more free space
	for (int n = 0; n < vector->size; n++) { 		//iterates through the ints in the bitvector
		for (int a = 0; a < 32; a++) { 				//iterates through the bits in bitvector
			if ((n * 32 + a + 1) > vector->blockCount) {	//reached end of the bitVector
				printf("ERROR: SYSTEM IS OUT OF SPACE\n");
				exit(0);
				return 0;
			}
			if ((vector->bitVector[n] & (1 << a)) == 0) {		//if vector bit is 0 aka free
				if (consecutiveFree == 0) {					//checks if this is the first free bit found, or following free bits
					freeIndex = n * 32 + a;
					consecutiveFree += 1;
				} else {
					consecutiveFree += 1;
				}
			} else {								//chain of free bits not big enough
				consecutiveFree = 0;
				freeIndex = 0;
			}
			if (consecutiveFree >= blockCount) {	//found the necessary chunk, sets all used bits to 1 and return the index of first free bit
				int count = consecutiveFree;
				int position = freeIndex;
				int freePos = position / 32;
				int freeBit = position % 32;
				while (count > 0) {
					vector->bitVector[freePos] |= 1 << freeBit;
					count -= 1;
					freeBit += 1;
					if (freeBit >= 32) {
						freePos += 1;
						freeBit = 0;
					}
				}
				LBAwrite(vector->bitVector, vector->blocksNeeded, vector->LBABitVector); //write the updates to bit vector to LBA
				return freeIndex;
			}
		}
	}
	printf("ERROR: SYSTEM IS OUT OF SPACE\n");
	exit(0);
	freeIndex = 0;
	return freeIndex; 								//bits were not found, return 0 to indicate error
}

void freeSomeBits(int startIndex, int count) {
	int index = startIndex / 32;
	int position = startIndex % 32;
	for (int n = 0; n < count; n++) { //iterates through bits to be freed
		int value = 1 << position;
		value = ~value;
		vector->bitVector[index] &= value;
		position += 1;
		if (position >= 32) {
			index += 1;
			position = 0;
		}
	}
	LBAwrite(vector->bitVector, vector->blocksNeeded, vector->LBABitVector); //write the updates to bit vector to LBA
}

u_int64_t expandFreeSection(int fileLocation, int fileBlockSize, int newBlockSize) {
	freeSomeBits(fileLocation, fileBlockSize); //frees the original bits
	u_int64_t newLBA = findMultipleBlocks(newBlockSize); //looks for the new size; if new size won't fit, it will find the proper block, otherwise it'll just find the same
	if (newLBA > 0) { //moves the data in the old location into the new LBA
		char * buf = malloc(4096 * fileBlockSize);
		LBAread(buf, fileBlockSize, fileLocation);
		LBAwrite(buf, fileBlockSize, newLBA);
		free(buf);
	}
	return newLBA;
}
