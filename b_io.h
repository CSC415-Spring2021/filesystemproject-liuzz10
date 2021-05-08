/***************************************************************************************
* Class: CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: b_io.h
*
* Description: This file include declarations of 5 main functions that will be called by shell.
*
*****************************************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>

/****************************************************
* @parameters 
*   @type char: file name 
*   @type int: flags: O_WRONLY, O_RDONLY, O_CREAT, O_TRUNC
* @return
*   @type int: file descriptor
* This function reads bytes from a file to caller's buffer.
****************************************************/
int b_open (char * filename, int flags);

/****************************************************
* @parameters 
*   @type int: file descriptor
*   @type char*: the caller's buffer (destination of read)
*   @type int: how many numbers of bytes to read
* @return
*   @type int: number of bytes read
* This function reads bytes from a file to caller's buffer.
****************************************************/
int b_read (int fd, char * buffer, int count);

/****************************************************
* @parameters 
*   @type int: file descriptor
*   @type char*: the caller's buffer (source of write)
*   @type int: how many numbers of bytes to write
* @return
*   @type int: number of bytes write
* This function writes bytes from caller's buffer to a file
****************************************************/
int b_write (int fd, char * buffer, int count);

/****************************************************
* @parameters 
*   @type int: file descriptor
*   @type off_t: the offset
*   @type int: flag SEEK_SET, SEEK_CUR, SEEK_END
* @return
*   @type int: number of bytes write
* This function looks for a certain position from where to write or read
****************************************************/
int b_seek (int fd, off_t offset, int whence);

/****************************************************
* @parameters 
*   @type int: file descriptor
* @return
*   @void
* This function closes the file and reset fcb
****************************************************/
void b_close (int fd);

#endif