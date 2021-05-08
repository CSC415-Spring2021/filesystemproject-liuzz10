/***********************************************************************************
* Class:  CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: dir.h
*
* Description:
*   Declaring the internal helper function for implementing the file system(under_score_names), and some 
<<<<<<< HEAD
* interface for b_io.c for accessing the metadata of the file(carmelNames)
=======
* 	interface for b_io.c for accessing the metadata
* 	of the file(carmelNames)
*
>>>>>>> 95e0e4a4856ac9bb450b49a713054667ae385fb2
**************************************************************************************/

#ifndef _DIR_H
#define _DIR_H
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include "fsLow.h"
#include "freeSpace.h"
#ifndef DIR_MAXLENGTH
#define DIR_MAXLENGTH 4096
#endif

/****************************************************
*  struct type to hold inodes which contains the meta
*  date of the file or directory
****************************************************/

typedef struct{
	off_t     fs_size;    		/* total size, in bytes */
	blksize_t fs_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  fs_blocks;  		/* number of 512B blocks allocated */
	time_t    fs_accesstime;   	/* time of last access */
	time_t    fs_modtime;   	/* time of last modification */
	time_t    fs_createtime;   	/* time of last status change */
	unsigned char fs_entry_type; /* entry type , file or directory*/
	uint64_t  fs_address;  /* inode address to find the actual data */
  int       fs_accessmode;      /* access mode */
}fs_inode;


/****************************************************
* struct type to hold the directory entry info
* this struct maintain the logical internal structure
* (tree) of the directory/file system
****************************************************/
typedef struct{
	char de_name[256]; /* name of the direcctory entry */
	uint32_t de_inode; /* inode number of current directory */
	uint32_t de_dotdot_inode; /* inode number of parent direcotry */
}fs_de;

/****************************************************
* struct type to hold the directory info. It is use 
* for locating and accessing the inodes and directory 
* entries. 
****************************************************/
typedef struct {
	uint64_t d_de_start_location;		/* Starting LBA of directory entries */ 
	blkcnt_t d_de_blocks; /* number of blocks for directory entries */
	uint64_t d_inode_start_location; /* Starting 
	LBA of the inodes */
	blkcnt_t d_inode_blocks; /* number of blocks for inodes */
	uint32_t d_num_inodes; /* number of inodes */
	uint32_t d_num_DEs; /* number of directory entry */
	fs_inode * d_inodes; /* inodes, not pertain */
	fs_de * d_dir_ents; /* DEs, not pertain */
}fs_directory;


/****************************************************
struct to hold the global varibale with LBA root
directory and the cwd
****************************************************/
typedef struct{
	// the current cwd
	 char cwd[DIR_MAXLENGTH + 1];
	// the LBA address of struct directory
	 uint64_t LBA_root_directory;
}DirInfo;

/****************************************************
struct to hold the splited of the path
****************************************************/
typedef struct{
	int length;
	char **dir_names;
}splitDIR;

/****************************************************
struct for string stacks 
****************************************************/
typedef struct{
	int capacity;
	int top; // point to the postion after the top
	char **strings;
}stringStack;


/****************************************************
* @parameters 
*   @type fs_directory*: pointer to a  
* @return
*   @type uint_32: the next available position of 
*	  	directory entry, return UINT_MAX as failed
*	Return the next available position of directory
*	entries
****************************************************/
uint32_t find_free_dir_ent(fs_directory *directory);

/****************************************************
* @parameters 
*   @type fs_directory*: directory
* @return
*   @type int: 0 is success, 1 is fail
* Reload the directory info from disk(LBA);	****************************************************/
int reload_directory(fs_directory *directory);

/****************************************************
* @parameters 
*   @type fs_directory*: directory
* @return
*   @void
*  Destructor of the fs_directory to free allocated
*  memory
****************************************************/
void free_directory(fs_directory* directory);

/****************************************************
* @parameters 
*   @type fs_directory*: directory
* @return
*   @type int: 0 is success, 1 is fail
* Written directory info back to LBA
****************************************************/
int write_directory(fs_directory *directory);

/****************************************************
* @parameters 
*   @type splitDIR*: a struct hold splited path info
* @return
*   @type uint_32: a number represent the position
*      of the directory entry, return UINT32_MAX, it 
*		   means fail
* Returned the position of the a given path which
* hold by the splitDIR object, UINT_MAX means fail
****************************************************/
uint32_t find_DE_pos(splitDIR *spdir);

/****************************************************
* @parameters 
*   @type uint32_t: position of the parent directory
*                   entry
*   @type char*: name of the new directory (to be addded)
* @return
*   @type int: 1 is duplicate, 0 is not
* Checked whether the given file/directory are duplicated *under the given parent pos.
****************************************************/
int is_duplicated_dir(uint32_t parent_de_pos, const char* name);

/****************************************************
* @parameters 
*   @type const char*: path to be splited 
* @return
*   @type splitDIR*: a pointer point to a struct splitDIR
*                    which hold the splited path
* Spliting the given path into seperate names of each
* level of directory, the delimeter is '/'
****************************************************/
splitDIR* split_dir(const char *name);

/****************************************************
* @parameters 
*   @type splitDIR*: the splited path info
* @return
*   @type void
* Destructor to free the allocated memory of strut type  *splitDIR allocated
****************************************************/
void free_split_dir(splitDIR *spdir);

/****************************************************
* @parameters 
*   @type char*: the string represent a relitive path
* @return
*   @type char*: the absoluted path
* Transformed the relative path(of cwd) to the absolute
* path (from "root/") 
****************************************************/
char *get_absolute_path(char* argv);

/****************************************************
* @parameters 
*   @type char*: the string represent the absolute path
* @return
*   @type int: 1 is the File type, 0 is not
* Checking whether a given path is a File or not
****************************************************/
int is_File(char *fullpath);

/****************************************************
* @parameters 
*   @type char*: the string represent the absolute path
* @return
*   @type int: 1 is the Dir type, 0 is not
* Checking whether a given path is a Dir or not
****************************************************/
int is_Dir(char *fullpath);

/****************************************************
* @parameters 
*   @type char*: the string represent the absolute path
* @return
*   @type char*: the parent path of the given path
* Getting the parent path of the given path
****************************************************/
char *get_parent_path(char* absolute_path);

/****************************************************
* @parameters 
*   @type char*: the string represent the file
*		@type int: flags of accessmode 			
* @return
*   @type uint64_t: LBA location of the file
* Returned the LBA of the file to do read. UINT_MAX 
* means fail.
****************************************************/
uint64_t getFileLBA(const char *filename, int flags);

/****************************************************
* @parameters 
*   @type char*: the string represent the file		
* @return
*   @type uint64_t: the allocated block of the file
* Returned the number of the allocated blocks of file
****************************************************/
blkcnt_t getBlocks(const char *filename);

/****************************************************
* @parameters 
*   @type char*: the string represent the file		
* @return
*   @type off_t: the size of the file
* Returned the actual size of file
****************************************************/
off_t getFileSize(const char *filename);


/****************************************************
* @parameters 
*   @type char*: the string represent the file
*		@type off_t: the size of the file		
* @return
*   @type int: 1 means success, 0 means fail
* Setting the size of the file (for b_io.c usage)
****************************************************/
int setFileSize(const char *filename, off_t filesize);

/****************************************************
* @parameters 
*   @type char*: the string represent the file
*		@type blkcnt_t: the allocated block to the file		
* @return
*   @type int: 1 means success, 0 means fail
* Setting the number of allocated block of the file 
* (for b_io.c usage)
****************************************************/
int setFileBlocks(const char *filename, blkcnt_t count);

/****************************************************
* @parameters 
*   @type char*: the string represent the file
*		@type uint64_t: the LBA address of the file		
* @return
*   @type int: 1 means success, 0 means fail
* Setting the address of the file (for b_io.c usage)
****************************************************/
int setFileLBA(const char *filename, uint64_t Address);

/****************************************************
 * !!This funtion is not used in system yet.
* @parameters 
*   @type uint32_t: the inode number(position)	
* @return
*   @type int: 1 means success, 0 means fail
* Updating the accesstime of the inode
****************************************************/
int updateAccessTime(uint32_t inode);

/****************************************************
* @parameters 
*   @type uint32_t: the inode number(position)	
* @return
*   @type int: 1 means success, 0 means fail
* Updating the modification time of the inode
****************************************************/
int updateModTime(uint32_t inode);

/****************************************************
* @parameters 
*   @type int: the capacity of the stack
* @return
*   @type stringStack*: the pointer to the stack
* Initial a stack holding strings
****************************************************/
stringStack* initStack(int capacity);

/****************************************************
* @parameters 
*   @type stringStack*: the stack
*		@type char*: the string to be pushed
* @return
*   @type int: 0 means success, 1 is fail
* Pushing a string into stack 
****************************************************/
int pushIntoStack(stringStack* stack, char* string);

/****************************************************
* @parameters 
*		@type stringStack*: the stack
* @return
*   @type char*: the string poped out, NULL as fail
* Poping out the top item in the stack
****************************************************/
char* popFromStack(stringStack* stack);


/****************************************************
* @parameters 
*		@type stringStack*: the stack
* @return
*   @type int: 0 is success, 1 is failed
* Deconstructor of the stack
****************************************************/
int free_stack(stringStack* stack);

/****************************************************
* @parameters 
*		@type time_t: time to be displayed
* @return
*   @type char*: formatted ouoput of the time
* Formater of the time (for command ls)
****************************************************/
char* display_time(time_t t); 

/****************************************************
*  helper function to format accessmode output, test only
****************************************************/
void print_accessmode(int access_mode, int file_type); 

DirInfo fs_DIR;
#endif