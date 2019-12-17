#include "const.h"
#include "transplant.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int comparestr(char *a, char *b);
int strlength(char *str);
int checkmagic();
long hextodec(int len);
int add_name(int len);
void clear_name();
/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/*
 * A function that returns printable names for the record types, for use in
 * generating debugging printout.
 */
static char *record_type_name(int i) {
    switch(i) {
    case START_OF_TRANSMISSION:
	return "START_OF_TRANSMISSION";
    case END_OF_TRANSMISSION:
	return "END_OF_TRANSMISSION";
    case START_OF_DIRECTORY:
	return "START_OF_DIRECTORY";
    case END_OF_DIRECTORY:
	return "END_OF_DIRECTORY";
    case DIRECTORY_ENTRY:
	return "DIRECTORY_ENTRY";
    case FILE_DATA:
	return "FILE_DATA";
    default:
	return "UNKNOWN";
    }
}

/*
 * @brief  Initialize path_buf to a specified base path.
 * @details  This function copies its null-terminated argument string into
 * path_buf, including its terminating null byte.
 * The function fails if the argument string, including the terminating
 * null byte, is longer than the size of path_buf.  The path_length variable
 * is set to the length of the string in path_buf, not including the terminating
 * null byte.
 *
 * @param  Pathname to be copied into path_buf.
 * @return 0 on success, -1 in case of error
 */
int path_init(char *name) {
    if(strlength(name)+1>PATH_MAX){
        return -1;
    }
    char *clear = path_buf;
    for(int i = 0;i<PATH_MAX;i++){
        clear = '\0';
        clear++;
    }

    char *point = path_buf;
    while(*name!='\0'){
        *point = *name;
        name++;
        point++;
    }
    *point = '\0';
    path_length = strlength(path_buf);
    return 0;
}

/*
 * @brief  Append an additional component to the end of the pathname in path_buf.
 * @details  This function assumes that path_buf has been initialized to a valid
 * string.  It appends to the existing string the path separator character '/',
 * followed by the string given as argument, including its terminating null byte.
 * The length of the new string, including the terminating null byte, must be
 * no more than the size of path_buf.  The variable path_length is updated to
 * remain consistent with the length of the string in path_buf.
 *
 * @param  The string to be appended to the path in path_buf.  The string must
 * not contain any occurrences of the path separator character '/'.
 * @return 0 in case of success, -1 otherwise.
 */
int path_push(char *name) {
    int newfile = strlength(name);
    if(strlength(path_buf)+newfile+1>PATH_MAX){
        return -1;
    }
    char *point = path_buf+path_length;
    *point = '/';
    point++;
    while(*name!='\0'){
        *point = *name;
        name++;
        point++;
    }
    *point = '\0';
    path_length = strlength(path_buf);
    return 0;
}

/*
 * @brief  Remove the last component from the end of the pathname.
 * @details  This function assumes that path_buf contains a non-empty string.
 * It removes the suffix of this string that starts at the last occurrence
 * of the path separator character '/'.  If there is no such occurrence,
 * then the entire string is removed, leaving an empty string in path_buf.
 * The variable path_length is updated to remain consistent with the length
 * of the string in path_buf.  The function fails if path_buf is originally
 * empty, so that there is no path component to be removed.
 *
 * @return 0 in case of success, -1 otherwise.
 */
int path_pop() {
    if(path_length==0){
        return -1;
    }
    int relative = 0;
    char *point = path_buf+path_length+1;
    int curlen = path_length+2;
    if(*path_buf=='/') relative = 1;
    while(*point!='/'&&curlen>0){
        *point = '\0';
        point--;
        curlen--;
    }
    if(relative==1||curlen>0){
        *point = '\0';
        point--;
    }
    path_length = strlength(path_buf);
    return 0;
}

/*
 * @brief  Serialize the contents of a directory as a sequence of records written
 * to the standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory to be serialized.  It serializes the contents of that directory as a
 * sequence of records that begins with a START_OF_DIRECTORY record, ends with an
 * END_OF_DIRECTORY record, and with the intervening records all of type DIRECTORY_ENTRY.
 *
 * @param depth  The value of the depth field that is expected to occur in the
 * START_OF_DIRECTORY, DIRECTORY_ENTRY, and END_OF_DIRECTORY records processed.
 * Note that this depth pertains only to the "top-level" records in the sequence:
 * DIRECTORY_ENTRY records may be recursively followed by similar sequence of
 * records describing sub-directories at a greater depth.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open files, failure to traverse directories, and I/O errors
 * that occur while reading file content and writing to standard output.
 */
int serialize_directory(int depth) {
    putchar(0x0c);
    putchar(0x0d);
    putchar(0xed);
    putchar(0x02);
    //put depth
    putchar((depth&0xFF000000)>>24);
    putchar((depth&0xFF0000)>>16);
    putchar((depth&0xFF00)>>8);
    putchar((depth&0xFF));
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x10);
    DIR *dir = opendir(path_buf);
    if(!dir) return -1;
    struct dirent *de = readdir(dir);
    while(de != NULL){
        char *name = de->d_name;
        int namelen = strlength(name);
        if(comparestr(name, ".")==1||comparestr(name, "..")==1) {
            de = readdir(dir);
            continue;
        }
        if(path_push(name)!=-1){
            struct stat stat_buf;
            stat(path_buf, &stat_buf);
            //put info for directory entry
            putchar(0x0c);
            putchar(0x0d);
            putchar(0xed);
            putchar(0x04);
            //put depth
            putchar((depth&0xFF000000)>>24);
            putchar((depth&0xFF0000)>>16);
            putchar((depth&0xFF00)>>8);
            putchar((depth&0xFF));
            //put total bytes
            putchar(((namelen+28)&0xFF00000000000000)>>56);
            putchar(((namelen+28)&0xFF000000000000)>>48);
            putchar(((namelen+28)&0xFF0000000000)>>40);
            putchar(((namelen+28)&0xFF00000000)>>32);
            putchar(((namelen+28)&0xFF000000)>>24);
            putchar(((namelen+28)&0xFF0000)>>16);
            putchar(((namelen+28)&0xFF00)>>8);
            putchar(((namelen+28)&0xFF));
            //put st_mode
            putchar((stat_buf.st_mode&0xFF000000)>>24);
            putchar((stat_buf.st_mode&0xFF0000)>>16);
            putchar((stat_buf.st_mode&0xFF00)>>8);
            putchar((stat_buf.st_mode&0xFF));
            //put st_size
            putchar((stat_buf.st_size&0xFF00000000000000)>>56);
            putchar((stat_buf.st_size&0xFF000000000000)>>48);
            putchar((stat_buf.st_size&0xFF0000000000)>>40);
            putchar((stat_buf.st_size&0xFF00000000)>>32);
            putchar((stat_buf.st_size&0xFF000000)>>24);
            putchar((stat_buf.st_size&0xFF0000)>>16);
            putchar((stat_buf.st_size&0xFF00)>>8);
            putchar((stat_buf.st_size&0xFF));
            //dectohex(stat_buf.st_size, 8);
            while(*name!='\0'){
                if(putchar(*name)==EOF) return -1;
                name++;
            }
            if(S_ISREG(stat_buf.st_mode)){
                if(serialize_file(depth, stat_buf.st_size)==-1) return -1;
                if(path_pop()==-1) return -1;
            }else if(S_ISDIR(stat_buf.st_mode)){
                if(serialize_directory(depth+1)==-1) return -1;
            }
            de = readdir(dir);
        }else{
            return -1;
        }
    }
    if(path_pop()==-1) return -1;
    //Put end if directory bytes
    putchar(0x0c);
    putchar(0x0d);
    putchar(0xed);
    putchar(0x03);
    //put depth
    putchar((depth&0xFF000000)>>24);
    putchar((depth&0xFF0000)>>16);
    putchar((depth&0xFF00)>>8);
    putchar((depth&0xFF));
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x10);
    closedir(dir);
    return 0;
}

/*
 * @brief  Serialize the contents of a file as a single record written to the
 * standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * file to be serialized.  It serializes the contents of that file as a single
 * FILE_DATA record emitted to the standard output.
 *
 * @param depth  The value to be used in the depth field of the FILE_DATA record.
 * @param size  The number of bytes of data in the file to be serialized.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open the file, too many or not enough data bytes read
 * from the file, and I/O errors reading the file data or writing to standard output.
 */
int serialize_file(int depth, off_t size) {
    FILE *f = fopen(path_buf, "r");
    if(!f) return -1;
    putchar(0x0c);
    putchar(0x0d);
    putchar(0xed);
    putchar(0x05);

    //put depth
    putchar((depth&0xFF000000)>>24);
    putchar((depth&0xFF0000)>>16);
    putchar((depth&0xFF00)>>8);
    putchar((depth&0xFF));

    //put total bytes
    putchar(((16+size)&0xFF00000000000000)>>56);
    putchar(((16+size)&0xFF000000000000)>>48);
    putchar(((16+size)&0xFF0000000000)>>40);
    putchar(((16+size)&0xFF00000000)>>32);
    putchar(((16+size)&0xFF000000)>>24);
    putchar(((16+size)&0xFF0000)>>16);
    putchar(((16+size)&0xFF00)>>8);
    putchar(((16+size)&0xFF));

    // int realSize = 0;
    // int c;
    // while((c = fgetc(f))!=EOF){
    //     unsigned char char2 = c;
    //     putchar(char2);
    //     realSize++;
    // }
    // fclose(f);
    // if(realSize!=size) return -1;
    for(int i = 0;i<size;i++){
        int c = fgetc(f);
        if(c==EOF) return -1;
        unsigned char test = c;
        if(putchar(test)==EOF) return -1;
    }
    fclose(f);
    return 0;
}

/**
 * @brief Serializes a tree of files and directories, writes
 * serialized data to standard output.
 * @details This function assumes path_buf has been initialized with the pathname
 * of a directory whose contents are to be serialized.  It traverses the tree of
 * files and directories contained in this directory (not including the directory
 * itself) and it emits on the standard output a sequence of bytes from which the
 * tree can be reconstructed.  Options that modify the behavior are obtained from
 * the global_options variable.
 *
 * @return 0 if serialization completes without error, -1 if an error occurs.
 */
int serialize() {
    putchar(0x0c);
    putchar(0x0d);
    putchar(0xed);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x10);
    if(serialize_directory(1) ==-1) return -1;
    putchar(0x0c);
    putchar(0x0d);
    putchar(0xed);
    putchar(0x01);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x00);
    putchar(0x10);
    return 0;
}

/*
 * @brief Deserialize directory contents into an existing directory.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory.  It reads (from the standard input) a sequence of DIRECTORY_ENTRY
 * records bracketed by a START_OF_DIRECTORY and END_OF_DIRECTORY record at the
 * same depth and it recreates the entries, leaving the deserialized files and
 * directories within the directory named by path_buf.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * each of the records processed.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including depth fields in the records read that do not match the
 * expected value, the records to be processed to not being with START_OF_DIRECTORY
 * or end with END_OF_DIRECTORY, or an I/O error occurs either while reading
 * the records from the standard input or in creating deserialized files and
 * directories.
 */
int deserialize_directory(int depth) {
    int end_dir = 0;

    //Check header for start directory
    if(checkmagic()==-1) return -1;
    //Check type
    int check = getchar();
    if(check==EOF) return -1;
    unsigned char startc = check;
    if(startc!=START_OF_DIRECTORY) return -1;
    //Check depth
    if(hextodec(4)!=depth) return -1;
    long startSize = hextodec(8);
    if(startSize == -1||startSize!=16) return -1;
    //check for file_data or more directories
    while(end_dir!=1){
        if(checkmagic()==-1) return -1;
        int check = getchar();
        if(check==EOF) return -1;
        unsigned char c = check;
        if(c==END_OF_DIRECTORY){
            //Check if end of directory
            end_dir = 1;
            if(depth!=hextodec(4)) return -1;
            long size = hextodec(8);
            if(size==-1||size!=16) return -1;
        }else if(c==DIRECTORY_ENTRY){
            if(depth!=hextodec(4)) return -1;
            long size = hextodec(8);
            if(size==-1) return -1;
            //get mode_t
            mode_t type = 0x00;
            for(int i = 0;i<4;i++){
                int check = getchar();
                if(check==EOF) return -1;
                unsigned char c = check;
                type <<= 8;
                type |= c;
            }
            //get size of file/dir
            off_t datasize = 0x00;
            for(int i = 0;i<8;i++){
                int check = getchar();
                if(check==EOF) return -1;
                unsigned char c = check;
                datasize <<= 8;
                datasize |= c;
            }
            //put name of file/dir into name_buf
            clear_name();
            if(add_name(size-HEADER_SIZE-12)==-1) return -1;
            if(path_push(name_buf)==-1) return -1;
            //Check if type is file
            if(S_ISREG(type)){
                if(deserialize_file(depth)==-1)return -1;
                chmod(path_buf, type&0777); //Change st_mode of file
                if(path_pop()==-1) return -1;
            }else if(S_ISDIR(type)){
                depth++;
                DIR *dir = opendir(path_buf);
                if((global_options&0x8)!=0x8){ //clobber not set
                    if(dir){
                        return -1;
                    }
                    mkdir(path_buf, 0700);
                }else{ //clobber set
                    if(!dir) mkdir(path_buf, 0700);
                }
                if(deserialize_directory(depth)==-1) return -1;
                depth--;
                chmod(path_buf, type&0777); //Change st_mode of file
                if(path_pop()==-1) return -1;;
            }else{
                return -1;
            }
        }else{
            return -1;
        }
    }
    return 0;
}

/*
 * @brief Deserialize the contents of a single file.
 * @details  This function assumes that path_buf contains the name of a file
 * to be deserialized.  The file must not already exist, unless the ``clobber''
 * bit is set in the global_options variable.  It reads (from the standard input)
 * a single FILE_DATA record containing the file content and it recreates the file
 * from the content.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * the FILE_DATA record.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including a depth field in the FILE_DATA record that does not match
 * the expected value, the record read is not a FILE_DATA record, the file to
 * be created already exists, or an I/O error occurs either while reading
 * the FILE_DATA record from the standard input or while re-creating the
 * deserialized file.
 */
int deserialize_file(int depth){
    FILE *f;
    struct stat stat_buf;
    int exist = stat(path_buf, &stat_buf);
    if((global_options&0x8)==0x8){
        f = fopen(path_buf, "w+");
    }else{
        if(exist==0) {
            return -1;
        }
        f = fopen(path_buf, "w");
    }
    if(!f) return -1;
    //Check magic bytes
    if(checkmagic()==-1) return -1;
    int check = getchar();
    if(check==EOF) return -1;
    unsigned char c = check;
    if(c!=FILE_DATA) return -1;
    //Check depth
    if(hextodec(4)!=depth) return -1;
    long recordSize = hextodec(8);
    long size = recordSize-HEADER_SIZE;
    if(size==-1) return -1;
    for(int i = 0;i<size;i++){
        int check = getchar();
        if(check==EOF) return -1;
        else{
            unsigned char c = check;
            fputc(c, f);
        }
    }
    fclose(f);
    return 0;
}

/**
 * @brief Reads serialized data from the standard input and reconstructs from it
 * a tree of files and directories.
 * @details  This function assumes path_buf has been initialized with the pathname
 * of a directory into which a tree of files and directories is to be placed.
 * If the directory does not already exist, it is created.  The function then reads
 * from from the standard input a sequence of bytes that represent a serialized tree
 * of files and directories in the format written by serialize() and it reconstructs
 * the tree within the specified directory.  Options that modify the behavior are
 * obtained from the global_options variable.
 *
 * @return 0 if deserialization completes without error, -1 if an error occurs.
 */
int deserialize() {
    //Check contents of Start of transmission header
    if(checkmagic()==-1) return -1;
    //Check type
    int check = getchar();
    if(check==EOF) return -1;
    unsigned char startc = check;
    if(startc!=START_OF_TRANSMISSION) return -1;
    //Check depth
    int depth = hextodec(4);
    if(depth == -1) return -1;
    long startSize = hextodec(8);
    if(startSize == -1 || startSize != 16) return -1;
    //Check if directory exists
    DIR *dir = opendir(path_buf);
    if(!dir) mkdir(path_buf, 0700);
    if(deserialize_directory(depth+1)==-1) return -1;
    //Check end of transmission
    if(checkmagic()==-1) return -1;
    //Check type
    int endcheck = getchar();
    if(endcheck==EOF) return -1;
    unsigned char endc = endcheck;
    if(endc!=END_OF_TRANSMISSION) return -1;
    //Check depth
    if(hextodec(4) != depth) return -1;
    long endSize = hextodec(8);
    if(endSize == -1 || endSize != 16) return -1;
    return 0;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv)
{
    int serial = 0;
    int existp = 0;
    if(argc>5||argc<2){
        return -1;
    }
    argv++;
    if(comparestr(*argv, "-h")){
        global_options |= 0x1;
        return 0;
    }if(comparestr(*argv, "-s")){
        global_options |= 0x2;
        serial = 1;
        argv++;
    }else if(comparestr(*argv, "-d")){
        global_options |= 0x4;
        argv++;
    }else{
        return -1;
    }
    while(*argv != NULL){
        if(comparestr(*argv, "-c")){
            if(serial==1){
                global_options = 0;
                return -1;
            }
            global_options |= 0x8;
            argv++;
        }else if(comparestr(*argv, "-p")){
            argv++;
            if(*argv == NULL || **argv == *"-"){
                global_options = 0;
                return -1;
            }
            if(path_init(*argv)==-1) return -1;
            existp = 1;
            argv++;
        }else{
            global_options = 0;
            return -1;
        }
    }
    if(existp == 0){
        if(path_init("./")==-1) return -1;
    }
    return 0;
}

int comparestr(char *a, char *b){
    while (*a == *b) {
        if(*a == '\0' && *b=='\0') return 1;
        else if (*a != *b) return 0;
        a++;
        b++;
    }
    return 0;
}

int strlength(char *str){
    int len = 0;
    while(*str!='\0'){
        len++;
        str++;
    }
    return len;
}

int checkmagic(){
    int fir = getchar();
    int sec = getchar();
    int thir = getchar();
    if(fir==EOF||sec==EOF||thir==EOF) return -1;
    else{
        unsigned char firc = fir;
        unsigned char secc = sec;
        unsigned char thirc = thir;
        if(firc!=MAGIC0) return -1;
        if(secc!=MAGIC1) return -1;
        if(thirc!=MAGIC2) return -1;
    }
    return 0;
}

long hextodec(int len){
    long dec = 0;
    long pow = len-1;
    long baseTwoFive = 1;
    while(pow>0){
        baseTwoFive *= 256;
        pow--;
    }
    for(int i = 0;i<len;i++){
        int check = getchar();
        if(check==EOF) return -1;
        unsigned char c = check;
        dec += c * baseTwoFive;
        baseTwoFive /= 256;
    }
    return dec;
}

int add_name(int len){
    char *point = name_buf;
    for(int i = 0;i < len;i++){
        int check = getchar();
        if(check==EOF) return -1;
        unsigned char c = check;
        *point = c;
        point++;
    }
    return 0;
}

void clear_name(){
    char *point = name_buf;
    while(*point!='\0'){
        *point = '\0';
        point++;
    }
}