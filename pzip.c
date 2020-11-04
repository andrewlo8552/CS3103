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

/*
//Lam Ting 10/26 mmap worked
//One Producer produce the String from inputfile to give multiple Consumer
//if Producer get n+1 char is different with n 
//it will start give the char to another Consumer(2)
//Consumer (1) start count the result output to the stdout
//Consumer (1) will wait until next round.

// it will have multiple lock 
//One Lock will lock the given char to Consumer
//One Lock will lock the writing to stdout
//Only One Consumer take the char from Producer
//Only One Consumer output the result to stdout
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
int main(int argc, char** argv)
{   
    //pthread_mutex_t lock; it is useless now until we decide what is out critical section
    if(argc<=1){
        char buffer[] = "pzip: file1 [file2 ...]\n";
        fwrite(buffer, 24 , 1,stdout); 
        return 1;
    }
    printf("Ver.12d3233\n");//version check
    //Test opening of file printf("%d", file_open);
    int file_open = open(argv[1] , O_RDONLY);
    //file struct
    struct stat statbuf;
    //get the detail of file here : num of text || Testprintf("%ld", statbuf.st_size);
    fstat (file_open,&statbuf);
    //mmap pointer
    char *src;
    //check mmap successful
    if ((src = mmap (NULL, statbuf.st_size, PROT_READ, MAP_SHARED, file_open, 0)) == (caddr_t) -1)
    {
        printf ("mmap error for input");
        return 0;
    }
    //print mmap value
    for(int i=0;i<statbuf.st_size;i++){
        printf ("%c",src[i]); 
    }
    /*
    pthread_t p1 ;
    //Not decide the number of thread

    Pthread_create(&p1, NULL, mythread, "A");
    //Create thread

    Pthread_join(p1 , null);
    //Join to wait for the threads to finish
    */
    return 0;
}

void *consumer(void *arg){
    int
}

