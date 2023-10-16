/*xargs: It reads data from standard input and executes another command using that data as command-line arguments.
 It is often used in conjunction with other commands, such as find, grep, and so on, to execute a series of commands.*/


 #include "kernel/param.h"
 #include "kernel/types.h"
 #include "user/user.h"
 
 #define buf_size 512

 int main(int argc, char *argv[])
 {
    char buf[buf_size + 1] = {0};// +1 ensures that there is extra space for the null terminator (\0)
    unsigned int occupy = 0;//The number of data in the current buffer
    char *xargv[MAXARG] = {0};//paramarrays
    int stdin_end = 0;//recognize stdin is finish

    for (int i = 1; i < argc; i++) {
        xargv[i - 1] = argv[i];//Copy the command line arguments to xargv
    }

    while (!(stdin_end && occupy == 0)) {// exit condition: data is managed up
        if (!stdin_end) {
            int remain_size = buf_size - occupy;
            int read_bytes = read(0, buf + occupy, remain_size);
            if (read_bytes < 0) {
                fprintf(2, "Xargs: read returns -1 error\n");
            }
            if (read_bytes == 0) {
                close(0);
                stdin_end = 1;// stdin is off
            }
            occupy += read_bytes;
        }
    

        //process lines read
        char  *line_end = strchr(buf, '\n');//find the first occurrence of a specified word in a string
        while (line_end) {// if found
            char xbuf[buf_size+1] = {0};
            memcpy(xbuf, buf, line_end-buf);//Extract a line from buf
            xargv[argc-1] = xbuf;// store the address of the line in the last position of the xargv array

            int ret = fork();
            if (ret == 0) { // I am child

                if (!stdin_end) {
                    close(0);
                }
                if (exec(argv[1],xargv) < 0) { //argv[1] is name of the command to execute
                    fprintf(2, "xargs:exec fails with -1\n");
                    exit(1);
                }

            } else {
                //trim out line already processed
                memmove(buf, line_end + 1, occupy -(line_end-buf)-1);//-1 so that the newline character itself is not included
                occupy -= line_end - buf + 1;
                memset(buf + occupy, 0, buf_size - occupy);//clean memory
                int pid;
                wait(&pid);

                line_end = strchr(buf, '\n');//new start
            }
        }
    }
    exit(0); 
 }