/*
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your group information here.
 *
 * Group No.:  (Join a project group in Canvas)
 * First member's full name: Ryan Chan
 * First member's email address: 
 * Second member's full name: Andrew Low
 * Second member's email address: 
 * Third member's full name: Lam Chi Ting
 * Third member's email address: chitlam7-c@my.cityu.edu.hk
 */
/*  Work Progress
//Lam Ting 10/26 mmap worked
//Lam Ting 11/08 Producer get the mmap text and able to print it


























*/
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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

void* Producer(void *arg){//Worked get the text 11/08
    char *str = (char*) arg;//get the data 
    fprintf( stdout, "%s\n", str);//print the data from main 
    pthread_exit(NULL);//leave thread
}

int main(int argc, char** argv)
{   
    //pthread_mutex_t lock; it is useless now until we decide what is out critical section
    if(argc<=1){
        char buffer[] = "pzip: file1 [file2 ...]\n";
        fwrite(buffer, 24 , 1,stdout); 
        return 1;
    }
    //printf("Ver.12d3233\n");//version check//Test opening of file printf("%d", file_open);
    int file_open = open(argv[1] , O_RDONLY);
    struct stat statbuf;//file struct
    fstat (file_open,&statbuf);//get the detail of file here : num of text || Testprintf("%ld", statbuf.st_size);
    char *src; //mmap pointer
    if ((src = mmap (NULL, statbuf.st_size, PROT_READ, MAP_SHARED, file_open, 0)) == (caddr_t) -1)//check mmap successful
    {
        printf ("mmap error for input");
        return 0;
    }
    //print mmap value
    /*
    for(int i=0;i<statbuf.st_size;i++){
        printf ("%c",src[i]); 
    }
    */
    //not work
    pthread_t producer;//declear Producer
    pthread_create(&producer, NULL, Producer, src);//Create thread Worked 11/08

    pthread_join(producer,NULL);//wait thread end

    return 0;
}

