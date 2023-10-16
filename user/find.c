/* thinking 
1. How do you know if a path is a file or a directory?
2. How to recursively scan a directory?
3. We need to build the full path ourselves along the way.
  We need to save a buffer and add '/[subdirectory]' where appropriate to enter the recursion
4. Remember to close file descriptors when you are no longer using them*/


#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"


//retrieve the filename from whole path
char *basename(char *pathname)
{   char *prev = 0;
    char *cur = strchr(pathname, '/'); //find next '/' --'/' : This is a character constant. It represents a single character
    while (cur != 0) {                                // "/" : This is a string constant that represents a string containing the character/and a closing character \0.
        prev = cur;
        cur = strchr(cur+1, '/'); // ensure we search character after '/'
    }
    return prev; // last "/"
}

void find(char *curr_path, char *target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(curr_path, O_RDONLY)) < 0) {
        fprintf(2, "find: cannot open %s\n", curr_path);
        return;
    }

    if (fstat(fd, &st) < 0) { //put the information of path in struct st 
        fprintf(2, "find: cannot stat %s\n", curr_path);
        close(fd);
        return;
    }

    switch (st.type) {

        case T_FILE:
             ; //When you follow a declaration directly after a tag, the compiler gives you an error.
            char *f_name = basename(curr_path);
            int match = 1;
            if (f_name == 0 || strcmp(f_name+1, target) != 0) {// find last "/" && compare target with letter after "/" 
                    match = 0;
            }
            if (match) {
                printf("%s\n", curr_path);
            }
            close(fd);
            break;

        case T_DIR: //make next level pathname
        memset(buf, 0, sizeof(buf));//initialize the newly allocated memory block
        unsigned int curr_path_len = strlen(curr_path);
        memcpy(buf, curr_path, curr_path_len);
        buf[curr_path_len] = '/';//prepare to append the next level directory/filename
        p = buf + curr_path_len + 1;// pointer to the next character to be written in buf
        //Directories are implemented as a special type of file.
        // These directory files contain a set of "directory entries," each of which points to a file or subdirectory in the filesystem.
        while (read(fd, &de, sizeof(de)) == sizeof(de)) { //loop continues until the read function is unable to read a complete entry
                if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    continue;
                }   
                memcpy(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, target);
        } 
        close(fd);
        break;
    }    
}

//argc: This represents the total number of command-line arguments 
//argv: is an array of strings containing all the command-line arguments. 
//When you run a program from the command line, argv[0] is usually the name of the program
//So if you run: find directory target, then: argv[0] 会是 "find",argv[1] 会是 "directory",argv[2] 会是 "target"

int main(int argc, char *argv[]) 
{
    if (argc != 3) {
        fprintf(2, "usage: find [directory] [target filename]\n");
        exit(1);
    } 
    find(argv[1], argv[2]);
    exit(0);
}

