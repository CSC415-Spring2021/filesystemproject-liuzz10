/***********************************************************************************
* Class: CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: mfs.c
*
* Description:
*   This is the implementation of mfs.h for the filesystem interface
*
**************************************************************************************/

#include <string.h>
#include "freeSpace.h"
#include "fsLow.h"
#include "mfs.h"
#include "vcb.h"
/****************************************************
* 
*                    root
*                      |
*                    Users
*                      |
*            Admin   Guest  Group
*
*   Initial the directory which has basic structure
* Note that root directory cannoed be changed
****************************************************/
uint64_t fs_init(){
    int blockCountInode = (MIN_DE_NUM * sizeof(fs_inode) + MINBLOCKSIZE - 1) / MINBLOCKSIZE;
    int actualNumInode = (blockCountInode * MINBLOCKSIZE) / sizeof(fs_inode);
    int blockCountDE = (actualNumInode * sizeof(fs_de) + MINBLOCKSIZE - 1) / MINBLOCKSIZE;
    /* the size of the inode is bigger than the size of DE, so we use the actual number of inodes to
    allocated the memory */
    fs_inode *inodes = malloc(blockCountInode * MINBLOCKSIZE);

    fs_de *dir_ents = malloc(blockCountDE * MINBLOCKSIZE);
    uint64_t LBA_inodes = findMultipleBlocks(blockCountInode);
    //Initialize inodes
    for(int i = 0; i < actualNumInode; ++i){
        if(i < 5){
            (inodes + i)->fs_entry_type = DT_DIR;
        }else{
            (inodes + i)->fs_entry_type = DT_REG;
        }
        (inodes + i)->fs_size = sizeof(fs_de);
        (inodes + i)->fs_blksize = 0;
        (inodes + i)->fs_blocks = 0;
        (inodes + i)->fs_accesstime = time(NULL);
        (inodes + i)->fs_modtime = time(NULL);
        (inodes + i)->fs_createtime = time(NULL);
        (inodes + i)->fs_address = ULLONG_MAX;
        (inodes + i)->fs_accessmode = 0777;
    }
    LBAwrite(inodes,blockCountInode,LBA_inodes);
    //Initialize directory entries(DE)
    for(int i = 0; i < actualNumInode; ++i){
        if( i == 0){
            strcpy((dir_ents + i)->de_name, "root");
            (dir_ents + i)->de_dotdot_inode = 0;
        }else if(i == 1){
            strcpy((dir_ents + i)->de_name, "Users");
            (dir_ents + i)->de_dotdot_inode = 0;
        }else if(i == 2){
            strcpy((dir_ents + i)->de_name, "Admin");
            (dir_ents + i)->de_dotdot_inode = 1;
        }else if(i == 3){
            strcpy((dir_ents + i)->de_name, "Guest");
            (dir_ents + i)->de_dotdot_inode = 1;
        }else if(i == 4){
            strcpy((dir_ents + i)->de_name, "Group");
            (dir_ents + i)->de_dotdot_inode = 1;
        }else{
            strcpy((dir_ents + i)->de_name, "uninitialized");
            (dir_ents + i)->de_dotdot_inode = UINT_MAX;
        }
        (dir_ents + i)->de_inode = i;
    }
    uint64_t LBA_dir_ents = findMultipleBlocks(blockCountDE);

    LBAwrite(dir_ents, blockCountDE, LBA_dir_ents);
 
    fs_directory *directory = (fs_directory*)malloc(sizeof(fs_directory));

    //Init the directory struct 
    directory->d_de_start_location = LBA_dir_ents;
    directory->d_de_blocks = blockCountDE;
    directory->d_inode_start_location = LBA_inodes;
    directory->d_inode_blocks = blockCountInode;
    directory->d_num_inodes = actualNumInode;
    directory->d_num_DEs = actualNumInode;
    uint64_t LBA_root_directory = findMultipleBlocks(1);
    LBAwrite(directory, 1, LBA_root_directory);
    fs_DIR.LBA_root_directory = LBA_root_directory;
    free(inodes);
    free(dir_ents);
    free(directory);
    inodes = NULL;
    dir_ents = NULL;
    directory = NULL;
    return LBA_root_directory;
}

int fs_mkdir(const char *pathname, mode_t mode){
    int success_status = 1;
    char *abslpath = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    // construct the full path of the given dir name
    strcpy(abslpath, pathname);
    abslpath = get_absolute_path(abslpath);
    //reload directory for LBA and get the pos of next
    //direcotry entry
    fs_directory* directory = malloc(MINBLOCKSIZE);
    LBAread(directory, 1, fs_DIR.LBA_root_directory);
    reload_directory(directory);
    uint32_t free_dir_ent = find_free_dir_ent(directory);
    splitDIR *spdir = split_dir(abslpath);
    char *new_dir_name = malloc(sizeof(char)*256);
    strcpy(new_dir_name, *(spdir->dir_names + spdir->length - 1));
    spdir->length--; // move up 1 level to cwd
    //find the parent pos and check whether have duplicated name 
    uint32_t parent_pos = find_DE_pos(spdir);
    int duplicated_name = is_duplicated_dir(parent_pos, new_dir_name);
    if(parent_pos != UINT_MAX){
        if(duplicated_name){
            printf("mkdir: %s: File exists\n", new_dir_name);
        }else{
            //initial the new direcoty entries info
            fs_de *de = (directory->d_dir_ents + free_dir_ent);
            de->de_dotdot_inode = parent_pos;
            fs_inode *inode = (directory->d_inodes + free_dir_ent);
            inode->fs_entry_type = DT_DIR;
            strcpy(de->de_name, new_dir_name);
            write_directory(directory);
            updateModTime(parent_pos);
            updateModTime(free_dir_ent);
            de = NULL;
            inode = NULL;
            success_status = 0;
        }
    }else{
        printf("mkdir: %s: cannot make new directory\n", new_dir_name);
    }
    free(abslpath);
    free_split_dir(spdir);
    free_directory(directory);
    free(new_dir_name);
    abslpath = NULL;
    new_dir_name = NULL;
    directory = NULL;
    spdir = NULL;
    return success_status;
}

int fs_rmdir(const char *pathname){
    int success_rmdir = 0;
    char *abslpath = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    char *filename = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    //construct the absolute path of the give dir name
    strcpy(filename, pathname);
    // reload directory for LBA
    abslpath = get_absolute_path(filename);
    fs_directory* directory = malloc(MINBLOCKSIZE);
    LBAread(directory, 1, fs_DIR.LBA_root_directory);
    reload_directory(directory);
    //check whether the give name is the Dir type
    int is_dir = is_Dir(abslpath);
    splitDIR *spdir = split_dir(abslpath);
    if(is_dir == 1){
        // find the directory entry position and check wheher it is empty
        uint32_t de_pos = find_DE_pos(spdir);
        fs_de *de = (directory->d_dir_ents + de_pos);
        uint32_t inode_pos = de->de_inode;
        int is_empty_dir = 1;
        for(int i = 0; i < directory->d_num_DEs; ++i){
            if((directory->d_dir_ents + i)->de_dotdot_inode == inode_pos){
                is_empty_dir = 0;
                break;
            }

        }
        if(is_empty_dir){
            // de-connected it to the parent node (which is free from directory)
            uint32_t parent_pos = de->de_dotdot_inode;
            de->de_dotdot_inode = UINT_MAX;
            success_rmdir = 1;
            //write back changed directory info to LBA
            write_directory(directory);
            updateModTime(parent_pos);
        }else{
            printf("rmdir: %s: is not a empty directory\n", de->de_name);
        }
        de = NULL;      
    }else{
        printf("rmdir: %s: No such file or directory\n", *(spdir->dir_names + spdir->length - 1));
    }
    free(abslpath);
    free_split_dir(spdir);
    free_directory(directory);
    free(filename);
    filename = NULL;
    abslpath = NULL;
    directory = NULL;
    spdir = NULL;
   return success_rmdir;
}

fdDir * fs_opendir(const char *name){
    splitDIR *spdir = split_dir(name);
    fdDir *dirp = malloc(sizeof(fdDir));
    dirp->cur_pos = 0;
    uint32_t de_pos = find_DE_pos(spdir);
    if(de_pos == UINT_MAX){
        //If not found 
        char *filename = *(spdir->dir_names + (spdir->length - 1));
        printf("no such file or direcotry: %s\n", filename);
        free_split_dir(spdir);
        free(dirp);
        spdir = NULL;
        filename = NULL;
        return NULL;
    }
    dirp->de_pos = de_pos;
    fs_directory* directory = malloc(MINBLOCKSIZE);
    LBAread(directory, 1, fs_DIR.LBA_root_directory);
    reload_directory(directory);
    uint32_t parent_inode = dirp->de_pos;
    uint32_t count_children = 0;
    uint32_t current_parent_inode;
    uint32_t parent_dotdot_inode = (directory->d_dir_ents + parent_inode)->de_dotdot_inode;
    //Located the DEs
    for(int i = 1; i < directory->d_num_DEs; ++i){
        current_parent_inode = (directory->d_dir_ents + i)->de_dotdot_inode;
        if(current_parent_inode == parent_inode){
            count_children++;
        }
    }
    dirp->num_children = count_children + 2;
    dirp->children = malloc(sizeof(struct fs_diriteminfo*) * (count_children + 2));
    (*(dirp->children + 0)) = malloc(sizeof(struct fs_diriteminfo)); 
    (*(dirp->children + 0))-> file_type = (directory->d_inodes + parent_inode)->fs_entry_type;
    strcpy(((*(dirp->children + 0))-> d_name), ".");
    //initialize ".."
     (*(dirp->children + 1)) = malloc(sizeof(struct fs_diriteminfo)); 
    (*(dirp->children + 1))-> file_type = (directory->d_inodes + parent_dotdot_inode)->fs_entry_type;
    strcpy(((*(dirp->children + 1))-> d_name), "..");
    int pos = 2;
    //Initial childrens iteminfo
    for(int i = 1; i < directory->d_num_DEs; ++i){
        fs_de *current_dir_ent = (directory->d_dir_ents + i);
        fs_inode *current_inode = (directory->d_inodes + i);
        current_parent_inode = current_dir_ent->de_dotdot_inode;
        if(current_parent_inode == parent_inode){
            (*(dirp->children + pos)) = malloc(sizeof(struct fs_diriteminfo)); 
            (*(dirp->children + pos))-> file_type = current_inode->fs_entry_type;
            strcpy((*(dirp->children + pos))-> d_name, current_dir_ent->de_name);
            pos++;
        }
        current_dir_ent = NULL;
        if(pos == (dirp->num_children)){
            break;
        }
    }
   free_directory(directory);
   directory = NULL;
   free_split_dir(spdir);
   spdir = NULL;
   return dirp;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp){
    struct fs_diriteminfo *di = NULL;
    // Iterated the the children one by one, return NULL if finish
    if((dirp->cur_pos) < (dirp->num_children)){
        di = (*(dirp->children + dirp->cur_pos));
        (dirp->cur_pos)++;
    }
    return di;
}

int fs_closedir(fdDir *dirp){
    for(int i = 0; i < dirp->num_children; ++i){
        free(*(dirp->children + i));
        (*(dirp->children + i));
    }
    free(dirp->children);
    free(dirp);
    dirp->children = NULL;
    dirp = NULL;
    return 0;
}

char * fs_getcwd(char *buf, size_t size){
   char *cwd = malloc(sizeof(char)*(size + 1));
   strcpy(cwd, fs_DIR.cwd);
   if(buf != NULL){
    strcpy(buf, fs_DIR.cwd);
   }
   return cwd;
}

int fs_setcwd(char *buf){
    int is_success = 1;
    char *abslpath = NULL;
    abslpath = get_absolute_path(buf);
    if((abslpath != NULL) && is_Dir(abslpath)){
        is_success = 0;
        strcpy(fs_DIR.cwd, abslpath);
    }
    free(abslpath);
    abslpath = NULL;
    return is_success;
    
}

int fs_isFile(char * path){
    char *fullpath = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    strcpy(fullpath, path);
    fullpath = get_absolute_path(fullpath);
    int is_file = is_File(fullpath);
    free(fullpath);
    fullpath = NULL;
    return is_file;
 }

int fs_isDir(char * path){
    char *fullpath = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    strcpy(fullpath, path);
    fullpath = get_absolute_path(fullpath);
    int is_dir = is_Dir(fullpath);
    free(fullpath);
    fullpath = NULL;
    return is_dir;
}		//return 1 if directory, 0 otherwise


int fs_delete(char* filename){
    int success_delete = 1;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    char *abslpath = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    strcpy(abslpath, filename);
    abslpath = get_absolute_path(abslpath);
    int is_file = is_File(abslpath);
    splitDIR *spdir = split_dir(abslpath);
    if(is_file == 1){
        //If it is file, release the space and
        //detached it from the system.
        uint32_t de_pos = find_DE_pos(spdir);
        fs_de *de = (directory->d_dir_ents + de_pos);
        uint32_t inode_pos = de->de_inode;
        uint32_t parent_pos = de->de_dotdot_inode;
        de->de_dotdot_inode = UINT_MAX;
        fs_inode *inode = (directory->d_inodes + inode_pos);
        freeSomeBits(inode->fs_address, inode->fs_blocks);
        inode->fs_size = sizeof(fs_de);
        inode->fs_blksize = 0;
        inode->fs_blocks = 0;
        inode->fs_createtime = time(NULL);
        inode->fs_modtime = time(NULL);
        inode->fs_accesstime = time(NULL);
        inode->fs_entry_type = DT_DIR;
        write_directory(directory);
        updateModTime(parent_pos);
        success_delete = 0;
    }
    free_directory(directory);
    free(abslpath);
    free_split_dir(spdir);
    return success_delete;
}	//removes a file

int fs_stat(const char *path, struct fs_stat *buf){
    fs_directory* directory = malloc(MINBLOCKSIZE);
    LBAread(directory, 1, fs_DIR.LBA_root_directory);
    reload_directory(directory);
    char *argv_buf = malloc(sizeof(char)*(DIR_MAXLENGTH + 1));
    strcpy(argv_buf, path);
    char *abslpath = get_absolute_path(argv_buf);
    splitDIR *spdir = split_dir(abslpath);
    uint32_t de_pos = find_DE_pos(spdir);
    uint32_t inode_pos =(directory->d_dir_ents + de_pos)->de_inode;
    fs_inode *inode = (directory->d_inodes + inode_pos);
    buf->st_size = inode->fs_size;
    buf->st_blksize = inode->fs_blocks;
    buf->st_blocks = inode->fs_blocks;
    buf->st_accesstime = inode->fs_accesstime;
    buf->st_modtime = inode->fs_modtime;
    buf->st_createtime = inode->fs_createtime;
    buf->st_accessmode = inode->fs_accessmode;
    free_directory(directory);
    free(argv_buf);
    free(abslpath);
    free_split_dir(spdir);
    argv_buf = NULL;
    abslpath = NULL;
    spdir = NULL;
    directory = NULL;
    return 0;
}


int fs_move(const char *src, const char* dest){
    int result = 0;
    char* src_buf = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    char* dest_buf = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    strcpy(src_buf, src);
    strcpy(dest_buf, dest);
    char* src_abslpath = get_absolute_path(src_buf);
    char* dest_abslpath = get_absolute_path(dest_buf);
    splitDIR *src_spdir = split_dir(src_abslpath);
    splitDIR *dest_spdir = split_dir(dest_abslpath);
    fs_directory* directory = malloc(MINBLOCKSIZE);

	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    if(!is_Dir(dest_abslpath)){
        printf("mv: %s is not a available directory location\n", dest);
        result = -2;  
    }else if(!is_File(src_abslpath)){
        printf("mv: %s is not exist a file\n", src);
        result = -1;
    }else{
        uint32_t src_de_pos = find_DE_pos(src_spdir);
        uint32_t dest_de_pos = find_DE_pos(dest_spdir);
        char *src_filename = *(src_spdir->dir_names + src_spdir->length - 1);
        int is_duplicated = is_duplicated_dir(dest_de_pos, src_filename);
        if(is_duplicated){
            printf("mv: %s (same name file) is exist in the destination directory\n", src_filename);
            result = -3;
        }else{
            //de_pos is eqaul to inode_pos (value)
            updateModTime(src_de_pos);
            updateModTime(dest_de_pos);
            fs_de *src_de = directory->d_dir_ents + src_de_pos;
            src_de->de_dotdot_inode = dest_de_pos;
            write_directory(directory);
        }
    } 
    free(src_buf);
    free(dest_buf);
    free(src_abslpath);
    free(dest_abslpath);
    free_split_dir(src_spdir);
    free_split_dir(dest_spdir);
    free_directory(directory);
    src_buf = NULL;
    dest_buf = NULL;
    src_abslpath = NULL;
    dest_abslpath = NULL;
    src_spdir = NULL;
    dest_spdir = NULL;
    directory = NULL;
    return result;
}//Test for git
