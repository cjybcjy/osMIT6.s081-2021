#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[])
{   int pid;
    int pipes1[2],pipes2[2];   
    char buf[] = {'a'};// "a" just a character signal for one byte
    pipe(pipes1);
    pipe(pipes2);
    signed ret1;
    //child send in pipes2[1],parent receives in pipes1[1];
    //parent send in pipes1[1], child receives in pipes2[0];
    int ret = fork();
    if (ret == 0) { // I am a child
        pid = getpid();
        close(pipes1[1]); //Turning off the write side triggers the EOF on the read side
        close(pipes2[0]);//child process turn off unneeded sides
        ret1 = read(pipes1[0], buf, 1);
        if (ret1 == -1) {
            fprintf(2, "%d:read error\n", pid);
            exit(1);
        } else {
            printf("%d:received ping\n", pid);
            write(pipes2[1], buf, 1);//write the byte to parent
            exit(0);
        }
    } else { // I am the parent
        pid = getpid();
        close(pipes1[0]);
        close(pipes2[1]);
        write(pipes1[1], buf, 1);
        ret1 = read(pipes2[0], buf, 1);
         if (ret1 == -1) {
            fprintf(2, "read error\n", pid);
            exit(1);
        } else {
            printf("%d:received pong\n", pid);
            exit(0);
        }
    }
}
