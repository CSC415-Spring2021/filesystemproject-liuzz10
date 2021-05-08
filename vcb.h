/***************************************************************************************
* Class: CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: vcb.h
*
* Description: 
*    This is the file to hold the struct and funtions used to create a volume for the file system.
*             
*****************************************************************************************/


#ifndef _VCB_H
#define _VCB_H
#include "freeSpace.h"
#include "mfs.h"
typedef struct vcb {
    int number_of_blocks;  // number of blocks in volume
    int size_of_block;  // the size of a single block
    int total_size_in_bytes;  // the product of number_of_blocks and size_of_block
    uint64_t LBA_free_space;  // the LBA of free space
    uint64_t LBA_root_directory;  // the LBA of the root directory
    int magic_number;  // magic number: where to write what value?
    int number_of_free_blocks;  // number of free blocks
    freeSpace* free_space_ptr;  // not persistent
} vcb;

#define BLOCKSIZE 512

/*
 * @brief initializes VCB
 * @param volumesize The size of the the whole volume
 * @param blocksize The size of the the whole volume
 * @return void
 */
int initializeVCB(uint64_t volumesize, uint64_t blocksize);
vcb* bootVCB(uint64_t volumesize, uint64_t blocksize);

#endif
