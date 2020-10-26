/*
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your group information here.
 *
 * Group No.: 10 (Join a project group in Canvas)
 * First member's full name: Tom Chan
 * First member's email address: cctom2-c@my.cityu.edu.hk
 * Second member's full name: (leave blank if none)
 * Second member's email address: (leave blank if none)
 * Third member's full name: (leave blank if none)
 * Third member's email address: (leave blank if none)
 */

// add/remove header files as you need
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;//it is useless now until we decide what is out critical section



void *mythread (void *arg){
    


}


int main(int argc, char** argv)
{   
    if(argv<=1){
        char buffer[] = "pzip: file1 [file2 ...]";
        fwrite(buffer, 24 , 1,stdout); 
        return 1;
    }
    
    //pthread_t p1 ;
    //Not decide the number of thread

    //Pthread_create(&p1, NULL, mythread, "A");
    //Create thread

    //Pthread_join(p1 , null);
    //Join to wait for the threads to finish

    return 0;
}

