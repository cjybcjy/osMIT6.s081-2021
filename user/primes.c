
// The "Sieve of Eratosthenes" is an algorithm for finding all prime numbers within a certain range.
/*The basic idea of this algorithm is to assume that all the numbers are prime, 
and then start with the smallest prime number, 2, and remove all its multiples; */

/* thinkings: 
goal: run as a prime-number processor
1. how a process filter the multiple of prime number----the first number come from left is prime
2. how to read from left neighbor ----Write a recursive function with arguments passed by pipe read side.
3. How to wait for all child processes to end to ensure the correctness of the process lifecycle chain?
-----Each process forks one child process, and wait(int* pid). 

NOTE: Close all unwanted file descriptors in time. 
*/

#include "kernel/types.h"
#include "user/user.h"

void prime_pipe(int num) //Realization filter method
{   int my_num = 0;
    int p[2];
    int passed_num = 0;
    int forked = 0;
    
    while (1) {
        int read_bytes = read(num, &passed_num, 4);// put byte in &n
        
        if (read_bytes == 0) {
            close(num);
            if (forked) {  // tell my child I have no more number to offer
                close(p[1]);
                wait(0);
            }
            exit(0);
        }

        if (my_num == 0) { // if initial read
            my_num = passed_num;
            printf("prime %d\n", my_num);
        }
        
        if (passed_num % my_num != 0) {
            if (!forked) {
                pipe(p);
                forked = 1;
                if (fork()) {// I am the child
                    close(p[1]);
                    close(num);
                    prime_pipe(p[0]);
                } else { // I am the parent
                    close(p[0]);
                }
            }
            write(p[1], &passed_num, 4);// pass the number to right 
        }   
    }
}



int main(int argc, char *argv[])
{
    int p[2];
    if (pipe(p) < 0) {
        fprintf(2, "pipe error\n");
        exit(1);
    }

    for (int i =2; i <= 35; i++) {
        write(p[1], &i, 4);
    }
    close(p[1]);
    prime_pipe(p[0]);
    exit(0);
}