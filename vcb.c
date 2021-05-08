/*****************************************************************************
* Class: CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: vcb.c
*
* Description: 
*    This file allocates the file system's volume and gets information from the free space and directory.              
*
******************************************************************************/

#include "vcb.h"
#include "freeSpace.h"
extern freeSpace* vector; 
int initializeVCB(uint64_t volumesize, uint64_t blocksize) {
    int number_of_blocks = (volumesize + blocksize - 1) / blocksize;
    vcb *v0 = malloc(blocksize*2);
    // Read from disk
    // initialize
   v0 -> number_of_blocks = number_of_blocks;
   v0 -> size_of_block = blocksize;
   v0 -> total_size_in_bytes = v0 -> number_of_blocks * v0 -> size_of_block;
   v0 -> magic_number = 0x12345678;
   v0 -> number_of_free_blocks = v0 -> number_of_blocks;
   // initialize free space
   init_freeSpace((volumesize + blocksize - 1) / blocksize, blocksize);
   v0 -> LBA_free_space = 2;

   // initialize directory
   v0 -> LBA_root_directory = fs_init();
   LBAwrite(v0, 1, 1);

    return 1;
};

vcb* bootVCB(uint64_t volumesize, uint64_t blocksize){
    vcb *vtemp = malloc(MINBLOCKSIZE);
    vcb *v0 = malloc(MINBLOCKSIZE);
	LBAread(vtemp, 1, 1);
	if(vtemp->magic_number != 0x12345678){
		printf("Initializing VCB......\n");
		initializeVCB(blocksize*10000, blocksize);
	} else {
        vector = malloc(512);
        LBAread(vector, 1, 2);
        vector->bitVector = malloc(512 * vector->blocksNeeded);
        LBAread(vector->bitVector, vector->blocksNeeded, vector->LBABitVector);
    }
    LBAread(v0, 1, 1);
    strcpy(fs_DIR.cwd,"root/Users");
    fs_DIR.LBA_root_directory = v0->LBA_root_directory;
    free(vtemp);
    vtemp = NULL;
    return v0;
}
