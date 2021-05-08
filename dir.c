/***************************************************************************************
* Class: CSC-415-02 Spring 2021
* Names: Zhuozhuo Liu, Jinjian Tan, Yunhao Wang (Mike on iLearn), Chu Cheng Situ
* Student ID: 921410045 (Zhuozhuo), 921383408 (Jinjian), 921458509 (Yunhao), 921409278 (Chu Cheng)
* GitHub Names: liuzz10 (Zhuozhuo), KenOasis (Jinjian), mikeyhwang (Yunhao), chuchengsitu (Chu Cheng)
* Group Name: return 0
* Project: Group Project – File System
*
* File: dir.c
*
* Description:
*   This is the implementation of the dir.h for the internal helper function and some interface for 
*   writing and reading by b_io.c
*
*****************************************************************************************/

#include <string.h>
#include "dir.h"
#include "b_io.h"

uint32_t find_free_dir_ent(fs_directory* directory){
    uint32_t free_dir_ent = 0;
    for(int i = 0; i < directory->d_num_DEs; ++i){
        if((directory->d_dir_ents + i)->de_dotdot_inode == UINT_MAX){
            free_dir_ent = i;
            break;
        }
    }
    if(free_dir_ent == 0){
        fs_directory* directory = malloc(MINBLOCKSIZE);
	    LBAread(directory, 1, fs_DIR.LBA_root_directory);
	    reload_directory(directory);
        blkcnt_t old_de_blocks = directory->d_de_blocks;
        blkcnt_t old_inode_blocks = directory->d_inode_blocks;
        uint32_t old_num_inodes = directory->d_num_inodes;
        uint32_t old_num_DEs = directory->d_num_DEs;

        uint64_t new_de_start_location = expandFreeSection(directory->d_de_start_location, directory->d_de_blocks, old_de_blocks * 2);
        uint64_t new_inode_start_location =  expandFreeSection(directory->d_inode_start_location, directory->d_inode_blocks, old_inode_blocks * 2);

        directory->d_de_start_location = new_de_start_location;
        directory->d_inode_start_location = new_inode_start_location;
        directory->d_de_blocks = (old_de_blocks) * 2;
        directory->d_inode_blocks = (old_inode_blocks) * 2;
        directory->d_num_inodes = (old_num_inodes) * 2;
        directory->d_num_DEs = (old_num_DEs) * 2;
        free(directory->d_inodes);
        free(directory->d_dir_ents);
        // reload inodes and dir_ents from new location
        reload_directory(directory);

        //init new allocated inodes and dir_ents
        fs_de *dir_ents = directory->d_dir_ents;
        for(int i = old_num_DEs; i < directory->d_num_DEs; ++i){
            strcpy((dir_ents + i)->de_name, "uninitialized");
            (dir_ents + i)->de_dotdot_inode = UINT_MAX;
            (dir_ents + i)->de_inode = i;
        }
        fs_inode *inodes = directory->d_inodes;
        for(int i = old_num_inodes; i < directory->d_num_inodes; ++i){
            (inodes + i)->fs_entry_type = DT_REG;
            (inodes + i)->fs_size = sizeof(fs_de);
            (inodes + i)->fs_blksize = 0;
            (inodes + i)->fs_blocks = 0;
            (inodes + i)->fs_accesstime = time(NULL);
            (inodes + i)->fs_modtime = time(NULL);
            (inodes + i)->fs_createtime = time(NULL);
            (inodes + i)->fs_address = ULLONG_MAX;
            (inodes + i)->fs_accessmode = 0777;
        }
        // the 1st new allocated dir_ents/inodes is the new one;
        write_directory(directory);
        free_directory(directory);
        directory = NULL;
        free_dir_ent = old_num_DEs; 
    }
    return free_dir_ent;
}

int reload_directory(fs_directory * directory){
    directory->d_inodes = malloc(MINBLOCKSIZE * (directory->d_inode_blocks));
    directory->d_dir_ents = malloc(MINBLOCKSIZE * (directory->d_de_blocks));
    LBAread(directory->d_inodes, directory->d_inode_blocks, directory->d_inode_start_location);
    LBAread(directory->d_dir_ents, directory->d_de_blocks, directory->d_de_start_location);
    return 0;
}

void free_directory(fs_directory *directory){
    free(directory->d_dir_ents);
    free(directory->d_inodes);
    free(directory);
}

int write_directory(fs_directory *directory){
    fs_de * dir_ents = directory->d_dir_ents;
    fs_inode *inodes = directory->d_inodes;
    LBAwrite(dir_ents, directory->d_de_blocks, directory->d_de_start_location);
    LBAwrite(inodes, directory->d_inode_blocks, directory->d_inode_start_location);
    LBAwrite(directory, 1, fs_DIR.LBA_root_directory);
    dir_ents = NULL;
    inodes = NULL;
    return 0;
}

uint32_t find_DE_pos(splitDIR *spdir){
    uint32_t de_pos = UINT_MAX;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    int found_dir_count = 0;
    int parent_pos = 0;
    int current_parent_pos = 0;
    uint32_t pos = 0;
    //Tracking from de of the root to the end to validate the path and found the de.
    for(int i = 0; i < (spdir->length); ++i){
        for(int j = 0; j < (directory->d_num_DEs); ++j){
            int not_equal_names = strcmp(*(spdir->dir_names + i),(directory->d_dir_ents + j)->de_name);
            current_parent_pos = (directory->d_dir_ents + j)->de_dotdot_inode;
            int is_child = (current_parent_pos == parent_pos);
            if((!not_equal_names) && is_child){
                found_dir_count++;
                pos = (directory->d_dir_ents + j) ->de_inode;
                parent_pos = pos;
                break;
            }
        }
        if(found_dir_count < i){
                break;
        }
    }
    if(found_dir_count == spdir->length){
        de_pos = pos;
    }
    free(directory);
    directory = NULL;
    return de_pos;
}

int is_duplicated_dir(uint32_t parent_de_pos, const char* name){// if return value = 0, means not duplicated
    int is_duplicated = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    for(int i = 1; i < directory->d_num_DEs; ++i){
        uint32_t current_parent = (directory->d_dir_ents + i)->de_dotdot_inode;
       
        int not_same_name = strcmp(name, (directory->d_dir_ents + i)->de_name);
        if((current_parent == parent_de_pos) && (!not_same_name)){
            is_duplicated = 1;
            break;
        }
    }
    free_directory(directory);
    directory = NULL;
    return is_duplicated;
}

splitDIR* split_dir(const char *name){
    // restart here! add . and .. into childrens
    const char delimiter[2] = "/";
    char *pathname = malloc(sizeof(char)*(DIR_MAXLENGTH + 1));
    char *token;
    int length = 0;
    strcpy(pathname, name);
    token = strtok(pathname, delimiter);
    while(token != NULL){
        length++;
        token = strtok(NULL, delimiter);
    }
    splitDIR *sDir = malloc(sizeof(splitDIR));
    sDir->dir_names = (char**)malloc(sizeof(char *) * length);
    sDir->length = length;
    for(int i = 0; i < length; ++i){
        *(sDir->dir_names + i) = malloc(sizeof(char) * 256);
    }
    // remember to free outside the function
    strcpy(pathname, name);
    token = strtok(pathname, delimiter);
    int i = 0;
    while ((token != NULL) && (strcmp(token, "") != 0)) 
    {
        strcpy(*(sDir->dir_names + i), token);
        i++;
        token = strtok(NULL, delimiter);
    }
    free(pathname);
    pathname = NULL;
    return sDir;    
}

void free_split_dir(splitDIR *spdir){
    for(int i = 0; i < spdir->length; ++i){
        free(*(spdir->dir_names + i));
    }
    free(spdir);
}

char *get_absolute_path(char* argv){
    char* absolute_path = NULL;
    char *cwd_buf = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    strcpy(cwd_buf, fs_DIR.cwd);
    splitDIR *cwd_spdir = NULL;
    splitDIR *argv_spdir = split_dir(argv);
    if(argv[0] == '/'){
        // if the argv start with "/" then the cwd part
        // change to the root directory
        cwd_spdir = split_dir("root/");
    }else{
        cwd_spdir= split_dir(cwd_buf);
    }
    
    int stack_capacity = cwd_spdir->length + argv_spdir->length + 1;
    stringStack *stack = initStack(stack_capacity);
    char *temp = NULL;
    for(int i = 0; i < cwd_spdir->length; ++i){
        temp = (*(cwd_spdir->dir_names + i));
        pushIntoStack(stack, temp);
    }
    
    for(int i = 0; i < argv_spdir->length; ++i){
        temp = (*(argv_spdir->dir_names + i));
        if(strcmp(temp, ".") == 0){
            continue;
        }else if(strcmp(temp, "..") == 0){
            temp = popFromStack(stack);
            if(strcmp(temp, "root") == 0){
                pushIntoStack(stack, temp);
            }
        }else{
            pushIntoStack(stack, temp);
        }
    }
    if((stack->top != 0)){
        absolute_path = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
        for(int i = 0; i < stack->top; ++i){
            temp = (*(stack->strings + i));
            if(i == 0){
                strcpy(absolute_path, temp);
            }else{
                absolute_path = strcat(absolute_path, temp);
            }
            absolute_path = strcat(absolute_path, "/");
        }
        if(strcmp(absolute_path, "root/") != 0){
            //remove the "/" if not the root dir
            absolute_path[strlen(absolute_path) - 1] = 0;
        }
    }
    free(cwd_buf);
    free_split_dir(cwd_spdir);
    free_split_dir(argv_spdir);
    free_stack(stack);
    cwd_buf = NULL;
    cwd_spdir = NULL;
    argv_spdir = NULL;
    stack = NULL;
    return absolute_path;
}

int is_File(char *fullpath){
    int is_file = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    splitDIR *spdir = split_dir(fullpath);
    uint32_t de_pos = find_DE_pos(spdir);
    // If the path is valid(exist)
    if(de_pos != UINT_MAX){
        uint32_t inode_num = (directory->d_dir_ents + de_pos)->de_inode;
        unsigned char file_type = (directory->d_inodes + inode_num)->fs_entry_type;
        if(file_type == DT_REG){
            is_file = 1;
        }
    }
    free_directory(directory);
    free_split_dir(spdir);
    directory = NULL;
    spdir = NULL;
    return is_file;
}

int is_Dir(char *fullpath){
    int is_dir = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    splitDIR *spdir = split_dir(fullpath);
    uint32_t de_pos = find_DE_pos(spdir);
    // if the path is valid(exist), check file info
    if(de_pos != UINT_MAX){
        uint32_t inode_num = (directory->d_dir_ents + de_pos)->de_inode;
        unsigned char file_type = (directory->d_inodes + inode_num)->fs_entry_type;
        if(file_type == DT_DIR){
            is_dir = 1;
        }
    }
    return is_dir;
}

char *get_parent_path(char* absolute_path){
    char *parent_path = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    if(strcmp(absolute_path, "root/") == 0){
        strcpy(parent_path, absolute_path);
    }else{
        char *last_delimeter = strrchr(absolute_path, '/');
        parent_path = strncpy(parent_path,absolute_path, last_delimeter - absolute_path);
        parent_path[last_delimeter - absolute_path] = 0;
    }
    if(strcmp(parent_path, "root") == 0){
        strcpy(parent_path, "root/");
    }
    return parent_path;
}

uint64_t getFileLBA(const char *filepath, int flags){
    //*check whether the dir is exist in the cwd
    uint64_t result = UINT_MAX;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    char *buf = malloc(sizeof(char) *(DIR_MAXLENGTH + 1));
    strcpy(buf, filepath);
    char* abslpath = get_absolute_path(buf);
    char* parent_path = get_parent_path(abslpath);
    char* last_delimeter = strrchr(abslpath, '/');
    int last_delimeter_pos = last_delimeter - abslpath;
    char* filename = malloc(sizeof(char) * (DIR_MAXLENGTH + 1));
    filename = strncpy(filename, (abslpath + last_delimeter_pos + 1), strlen(abslpath) - last_delimeter_pos);
    splitDIR *spdir = split_dir(parent_path);
    uint32_t parent_pos = find_DE_pos(spdir);
    int is_duplicated = is_duplicated_dir(parent_pos, filename);
    int is_file = 0;
    if(is_duplicated){
        is_file = is_File(abslpath);
    }
    if(is_duplicated){
        if(!is_file){
            printf("open: %s exist in system as directory\n", filename);
        }else{
            splitDIR *file_spdir = split_dir(abslpath);
            uint32_t file_de_pos = find_DE_pos(file_spdir);
            fs_de *file_de = (directory->d_dir_ents + file_de_pos);
            fs_inode *file_inode = (directory->d_inodes + file_de_pos);
            strcpy(file_de->de_name, filename);
            if((flags & O_TRUNC) == O_TRUNC){
                file_inode->fs_address = expandFreeSection(file_inode->fs_address, file_inode->fs_blocks, 10);
                file_inode->fs_blocks = 0;
                file_inode->fs_size = 0;
                file_inode->fs_blocks = 10;
                result = file_inode->fs_address;
            }else{
                result = file_inode->fs_address;
            }
            write_directory(directory);
            updateModTime(file_de->de_dotdot_inode);
            updateModTime(file_de->de_inode);
            file_de = NULL;
            file_inode = NULL;
        }
    }else if((flags & O_CREAT) == O_CREAT){
        // No same name dir or file exist
        int is_parent_exists = is_Dir(parent_path);
        if(is_parent_exists){
            uint32_t new_de_pos = find_free_dir_ent(directory);
            fs_de *new_de = (directory->d_dir_ents + new_de_pos);
            new_de -> de_dotdot_inode = parent_pos;
            fs_inode *new_inode = (directory->d_inodes + new_de_pos);
            new_inode->fs_entry_type = DT_REG;
            strcpy(new_de->de_name, filename);
            //Allocated space (LBA) for file 

            new_inode->fs_address = findMultipleBlocks(10);
            new_inode->fs_size = 0;
            new_inode->fs_blocks = 10;
            new_inode->fs_blksize = MINBLOCKSIZE;
            result = new_inode->fs_address;
            write_directory(directory);
            updateModTime(parent_pos);
            updateModTime(new_de_pos);
            new_de = NULL;
            new_inode = NULL;
        }else{
            printf("Can not create new file %s\n", filepath);
        }
    }else{
        printf("File %s not exist\n",filepath);
        result = UINT_MAX;
    }
    return result;
}

blkcnt_t getBlocks(const char *filepath){
    blkcnt_t result = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    char *abslpath = malloc(sizeof(char)*(DIR_MAXLENGTH + 1));
    strcpy(abslpath, filepath);
    abslpath = get_absolute_path(abslpath);
    splitDIR *spdir = split_dir(abslpath);
    uint32_t de_pos = find_DE_pos(spdir);
    int is_file = is_File(abslpath);
    if(de_pos!= UINT_MAX && is_file){
        uint32_t inode_num = (directory->d_dir_ents + de_pos)->de_inode;
        result = (directory->d_inodes + inode_num)->fs_blocks;
    }else{
        // printf("%s is not exist or not a File\n", filename);
        result = ULLONG_MAX;
    }
    free_directory(directory);
    free_split_dir(spdir);
    free(abslpath);
    directory = NULL;
    spdir = NULL;
    abslpath = NULL;
    return result;
}

off_t getFileSize(const char *filepath){
    off_t result = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    char *abslpath = malloc(sizeof(char)*(DIR_MAXLENGTH + 1));
    strcpy(abslpath, filepath);
    abslpath = get_absolute_path(abslpath);
    splitDIR *spdir = split_dir(abslpath);
    uint32_t de_pos = find_DE_pos(spdir);
    int is_file = is_File(abslpath);
    if(de_pos!= UINT_MAX && is_file){
        uint32_t inode_num = (directory->d_dir_ents + de_pos)->de_inode;
        result = (directory->d_inodes + inode_num)->fs_size;
    }else{
        // printf("%s is not exist or not a File\n", filename);
        result = ULLONG_MAX;
    }
    free_directory(directory);
    free_split_dir(spdir);
    free(abslpath);
    directory = NULL;
    spdir = NULL;
    abslpath = NULL;
    return result;
    return 0;
}

int setFileSize(const char *filepath, off_t filesize){
    int result = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    char *abslpath = malloc(sizeof(char)*(DIR_MAXLENGTH + 1));
    strcpy(abslpath, filepath);
    abslpath = get_absolute_path(abslpath);
    splitDIR *spdir = split_dir(abslpath);
    uint32_t de_pos = find_DE_pos(spdir);
    int is_file = is_File(abslpath);
    if(de_pos!= UINT_MAX && is_file){
        //Located the inode
        uint32_t inode_num = (directory->d_dir_ents + de_pos)->de_inode;
        (directory->d_inodes + inode_num)->fs_size = filesize;
        write_directory(directory);
        updateModTime(de_pos);
        result = 1;
    }else{
        // printf("%s is not exist or not a File\n", filename);
    }
    free_directory(directory);
    free_split_dir(spdir);
    free(abslpath);
    directory = NULL;
    spdir = NULL;
    abslpath = NULL;
    return result;
}

int setFileBlocks(const char *filepath, blkcnt_t count){
    int result = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    char *abslpath = malloc(sizeof(char)*(DIR_MAXLENGTH + 1));
    strcpy(abslpath, filepath);
    abslpath = get_absolute_path(abslpath);
    splitDIR *spdir = split_dir(abslpath);
    uint32_t de_pos = find_DE_pos(spdir);
    int is_file = is_File(abslpath);
    if(de_pos!= UINT_MAX && is_file){
        uint32_t inode_num = (directory->d_dir_ents + de_pos)->de_inode;
        (directory->d_inodes + inode_num)->fs_blocks = count;
        write_directory(directory);
        updateModTime(de_pos);
        result = 1;
    }else{
        // printf("%s is not exist or not a File\n", filename);
    }
    free_directory(directory);
    free_split_dir(spdir);
    free(abslpath);
    directory = NULL;
    spdir = NULL;
    abslpath = NULL;
    return result;
}

int setFileLBA(const char *filepath, uint64_t address){
    // Not check the whether the LBA is available or legal
    int result = 0;
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    char *abslpath = malloc(sizeof(char)*(DIR_MAXLENGTH + 1));
    strcpy(abslpath, filepath);
    abslpath = get_absolute_path(abslpath);
    splitDIR *spdir = split_dir(abslpath);
    uint32_t de_pos = find_DE_pos(spdir);
    int is_file = is_File(abslpath);
    if(de_pos!= UINT_MAX && is_file){
        uint32_t inode_num = (directory->d_dir_ents + de_pos)->de_inode;
        (directory->d_inodes + inode_num)->fs_address = address;
        write_directory(directory);
        updateModTime(de_pos);
        result = 1;
    }else{
        // printf("%s is not exist or not a File\n", filename);
    }
    free_directory(directory);
    free_split_dir(spdir);
    free(abslpath);
    directory = NULL;
    spdir = NULL;
    abslpath = NULL;
    return result;
}

// Not used
int updateAccessTime(uint32_t inode){
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    (directory->d_inodes + inode)->fs_accesstime = time(NULL);
    write_directory(directory);
    free_directory(directory);
    directory = NULL;
    return 1;
}

int updateModTime(uint32_t inode){
    fs_directory* directory = malloc(MINBLOCKSIZE);
	LBAread(directory, 1, fs_DIR.LBA_root_directory);
	reload_directory(directory);
    (directory->d_inodes + inode)->fs_modtime = time(NULL);
    write_directory(directory);
    free_directory(directory);
    directory = NULL;
    return 1;
}

stringStack* initStack(int capacity){
    stringStack *stack = malloc(sizeof(stringStack));
    stack->capacity = capacity;
    stack->top = 0;
    stack->strings = malloc(sizeof(char*) * capacity);
    for(int i = 0; i < capacity; ++i){
        *(stack->strings + i) = malloc(sizeof(char) * 256);
    }
    return stack;
}

int pushIntoStack(stringStack* stack, char* string){
    int result = 1;
    if((stack->top) < (stack->capacity)){
        strcpy(*(stack->strings + stack->top),  string);
        (stack->top)++;
        result = 0;
    }
    return result;
}

char* popFromStack(stringStack* stack){
    char *result = NULL;
    if(stack->top > 0){
        result = malloc(sizeof(char) * 256);
        strcpy(result, *(stack->strings + stack->top - 1));
        (stack->top)--;
    }
    return result;
}

int free_stack(stringStack *stack){
    for(int i = 0; i < stack->top; ++i){
        free(*(stack->strings + i));
    }
    free(stack->strings);
    return 0;
}

char* display_time(time_t t){
    char *time_str = malloc(sizeof(char) * 32);
    struct tm *ptm = NULL;
    char *month_str[12] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
    };
    if(t != -1){
        ptm = localtime(&t);
    }
    
    if (ptm != NULL) {
         sprintf(time_str, "%s %02d %02d:%02d", month_str[ptm->tm_mon],ptm->tm_mday, ptm->tm_hour, 
           ptm->tm_min);
    }
    ptm = NULL;
    return time_str;
}

/****************************************************
*  helper function to format accessmode output, test only
****************************************************/
void print_accessmode(int access_mode, int file_type){    char access_mode_str[11] = "----------";
    if(file_type == DT_DIR){
        access_mode_str[0] = 'd';
    }else if(file_type == DT_LNK){
        access_mode_str[0] = 'l';
    }
    int owner_access_mode = (0700 & access_mode);
    if((owner_access_mode & 0400) == 0400){
        access_mode_str[1] = 'r';
    }
    if((owner_access_mode & 0200) == 0200){
        access_mode_str[2] = 'w';
    }
    if((owner_access_mode & 0100) == 0100){
        access_mode_str[3] = 'x';
    }
    int group_access_mode = (0070 & access_mode);
    if((group_access_mode & 0040) == 0040){
        access_mode_str[4] = 'r';
    }
    if((group_access_mode & 0020) == 0020){
        access_mode_str[5] = 'w';
    }
    if((group_access_mode & 0010) == 0010){
        access_mode_str[6] = 'x';
    }
    int global_access_mode = (0007 & access_mode);
    if((global_access_mode & 0004) == 0004){
        access_mode_str[7] = 'r';
    }
    if((global_access_mode & 0002) == 0002){
        access_mode_str[8] = 'w';
    }
    if((global_access_mode & 0001) == 0001){
        access_mode_str[9] = 'x';
    }
    printf("%s", access_mode_str);
}