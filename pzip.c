/*
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your group information here.
 *
 * Group No.:  (Join a project group in Canvas)
 * First member's full name: Chan Pun Hei
 * First member's email address: punhchan2-c@my.cityu.edu.hk
 * Second member's full name: Andrew Low
 * Second member's email address: 
 * Third member's full name: Lam Chi Ting
 * Third member's email address: chitlam7-c@my.cityu.edu.hk
*/

//Work Progress
//Lam Ting 10/26 mmap worked
//Lam Ting 11/08 Producer get the mmap text and able to print it

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

clock_t start;

int numOfInputs;
int numOfFiles = 0;
int pageCapa = 25*1024*1024; //Capacity of each page: 25MB
int totalPages = 0;
int* numOfPagesPerFile; //Each file have how many pages
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER, compressLock = PTHREAD_MUTEX_INITIALIZER; //To prevent deadlock
pthread_cond_t wakeUp = PTHREAD_COND_INITIALIZER, wait = PTHREAD_COND_INITIALIZER; //For consumer to wake and sleep
int producerFin = 0;
#define queueCapa 10 //Capacity of queue
int queueSize = 0; //Current size of queue
int queueHead = 0, queueTail = 0; //Pointer of head and tail of queue
//int finishedConsumer = 0;

struct buffer {
  char* address; //Mapping address of the page
  int fileNum; //nth file
  int pageNum; //nth page of file
  int pageSize; //Size of this page
}buf[queueCapa];
//Compressed output
struct output{
  char* data; //Compressed character
  int* count; //Number of compress character
  int size; //Total size of output
}*out;

//Put page to queue
void put(struct buffer page){
  //Put page to queue
  buf[queueHead] = page;
  //Point to next and prevent over the capacity
  queueHead = (queueHead + 1) % queueCapa;
  queueSize++;
}

//Return buffer to consumer
struct buffer get(){
  struct buffer outPage = buf[queueTail];
  //Point to next and prevent over the capacity
  queueTail = (queueTail + 1) % queueCapa;
  queueSize--;
  return outPage;
}

//To put pages to queue for consumer to compress
void* producer(void *arg){
	char** filenames = (char**)arg;
  int file;
  struct stat fileInfo;
  char* mapAddress;
	
	//Split files to pages
  for(int i=0;i<numOfFiles;i++){
		file = open(filenames[i], O_RDONLY);
    //numOfPages: number of pages in file
    //pageSize: size of page
    int numOfPages = 0, pageSize = 0;
		
		//Stop if file not found
    if(file == -1){
      printf("Error: File didn't open\n");
      exit(1);
    }

	  //Get the information of file
    fstat(file,&fileInfo);
    //Divide file
    numOfPages = (fileInfo.st_size/pageCapa);

    //printf("Before calculate the size of last page\n");
    //Calculate the size of last page
    if(((double)fileInfo.st_size/pageCapa)>numOfPages){ 
      numOfPages+=1;
      pageSize = fileInfo.st_size%pageCapa;
    }
    else{
      pageSize=pageCapa;
    }
    //Update totalPages & numOfPagesPerFile
        totalPages += numOfPages;
        numOfPagesPerFile[i] = numOfPages;
        
        //Map the file
        mapAddress = mmap(NULL, fileInfo.st_size, PROT_READ, MAP_SHARED, file, 0);
        //Create buffer for each page and put into queue
		    for(int x=0;x<numOfPages;x++){
			    pthread_mutex_lock(&lock);
			    //Wake up comsumer if queue is full
          while(queueSize == queueCapa){
            pthread_cond_broadcast(&wakeUp);
				    pthread_cond_wait(&wait,&lock);
			    }
			    pthread_mutex_unlock(&lock);
			
            //Create buffer to put in queue
            struct buffer page;
            //Check is last page
            if(x == numOfPages - 1){
                //If last page, size is size of last page 
                page.pageSize = pageSize;
            }
            else{
                //Else is the default capacity of page
                page.pageSize = pageCapa;
            }
            page.address = mapAddress;
            page.pageNum = x;
            page.fileNum = i;
            mapAddress += pageCapa;
            //printf("Ready to put\n");
            //Put temp to queue
            pthread_mutex_lock(&lock);
            put(page);
            pthread_mutex_unlock(&lock);
            //Wake up consumer to start
            pthread_cond_signal(&wakeUp);
		}
		close(file);
	}
    //Wake up all consumer when producer finished
    //printf("producer() finished time: %f\n",((double)(clock()-start))/CLOCKS_PER_SEC);
    producerFin = 1;
    pthread_cond_broadcast(&wakeUp);
    return 0;
}

//Do compression
void compress(struct buffer page){
	
    //Calculate position of page
    int position = 0;
    for(int i=0;i<page.fileNum;i++){
            position += numOfPagesPerFile[i];
        }
    position += page.pageNum;

    //Do compression
    struct output compressed;
    compressed.count=malloc(page.pageSize*sizeof(int));
    char* tempString=malloc(page.pageSize);
    int count=0; 
    //printf("pageSize: %d\n",page.pageSize);
    for(int i=0;i<page.pageSize;i++){
        //Start new character and count
        tempString[count]=page.address[i];
        compressed.count[count]=1;
        //While next character is same and not last character, add count
        while((i+1<page.pageSize)&&(page.address[i]==page.address[i+1])){
            compressed.count[count]++; 
            i++;
        }
        //Next character is not same, do next character compressions
        count++;
    }
    compressed.size=count;
    compressed.data=realloc(tempString,count);
    out[position] = compressed;
    //printf("compress end time: %f\n",((double)(clock()-start))/CLOCKS_PER_SEC);
    //return compressed;
    //printf("compressed.count,data: %ls%s\n",compressed.count,compressed.data);
    //Put into output
    //printf("Before put\n");
    
    //printf("out[position].count,data: %d%s\n",out[position].size,out[position].data);
    //printf("out[position].size: %d\n", out[position].size);
    //printf("Put into output\n");
}

//To get the pages from queue and compress
void *consumer(){
	//Work until producer is finish and queue is empty
    do{
        pthread_mutex_lock(&lock);
        while((producerFin==0)&&(queueSize==0)){
            //Wait until producer put page to queue
            pthread_cond_signal(&wait);
            pthread_cond_wait(&wakeUp,&lock);
        }
        if(producerFin==1 && queueSize==0){
            //Exit when all page compression finished
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }
		    struct buffer page = get();
        if(producerFin == 0){
            //Tell producer to work when his job is not finished
            pthread_cond_signal(&wait);
        }
        
        pthread_mutex_unlock(&lock);
        //Pass page to do compress
        compress(page);
    }while((producerFin!=1)||(queueSize!=0));
    pthread_exit(NULL);
}

//Print the final output
void printOutput(){
    //Prepare location for finaloutput 
    char* finalOutput = malloc(totalPages*pageCapa*(sizeof(int)+sizeof(char)));
    //printf("finalOutput: %p\n",finalOutput);
    char* startPoint = finalOutput;
    //printf("startPoint: %p\n",startPoint);
	  for(int i=0;i<totalPages;i++){
		//Check if last char of curr output == 1st char of next output
        if(i < totalPages-1){
            if(out[i].data[out[i].size-1]==out[i+1].data[0]){
                //Add curr count to next output
                out[i+1].count[0] += out[i].count[out[i].size-1];
                //Prevent print out last char
                out[i].size--;
            }
        }
            
        for(int x=0;x<out[i].size;x++){
            //Count and char of output
            int num = out[i].count[x];
            char character = out[i].data[x];
            //Store output into finalOutput and go to next pointer
            *((int*)finalOutput)=num;
            finalOutput+=sizeof(int);
            *((char*)finalOutput)=character;      
            finalOutput+=sizeof(char);
        }
	}
	//Print the result
	fwrite(startPoint,finalOutput-startPoint,sizeof(char),stdout);
}

char** checkDir(char** inputs){
    DIR * dir;
    //Dynamic array
    char** filenames = calloc(100,sizeof(char*));
    char* fn,fdir[70];
    struct dirent * ptr;
    for(int i=0;i<numOfInputs;i++){
        //if inputs[i] is dir, put all filenames in dir to filenames
        dir = opendir(inputs[i]);
        if(dir!=NULL){
            while((ptr=readdir(dir))!=NULL){
                fn = ptr->d_name;
                if(!(fn[0]=='.')){
                    //Combine directory and filename
                    strcpy(fdir,inputs[i]);
                    strcat(fdir,"/");
                    strcat(fdir,fn);
                    //Store the combined name
                    filenames[numOfFiles] = malloc(strlen(fdir));
                    memcpy(filenames[numOfFiles],fdir,strlen(fdir)+1);
                    //printf("File dir%d: %s\n",numOfFiles,filenames[numOfFiles]);
                    numOfFiles++;
                }        
            }
            //printf("Go to close dir\n");
            closedir(dir);
        }
        else{
            //If input is file name, just put it into array
            *(filenames+numOfFiles) = inputs[i];
            //printf("File dir%d: %s\n",numOfFiles,filenames[numOfFiles]);
            numOfFiles++;
        }
    }
    //Use to check file names
    /*for(int i=0;i<numOfFiles;i++){
      printf("filename: %s\n",filenames[i]);
    }*/
    //printf("Filename() end time: %f\n",((double)(clock()-start))/CLOCKS_PER_SEC);
    return filenames;
}

int main(int argc, char* argv[]){
	// write your code here
    
    //Check if no file name there
    if(argc<2){
        printf("pzip: file1 [file2 ...]\n");
        exit(1);
    }

	  //Create threads, create consumer by the number of processors
    numOfInputs = argc - 1;
    int numOfThreads = get_nprocs();
    //Check does input contain directory
    char** filenames = checkDir(argv+1);
	  //Give location for numOfPagesPerFile and output
    numOfPagesPerFile = malloc(sizeof(int)*numOfFiles);
    out = malloc(sizeof(struct output)*512000*2); 
    //printf("Get filenames\n");
    pthread_t pid, cid[numOfThreads];
	  //Create producer
    pthread_create(&pid, NULL, producer, filenames);
    //Create consumers
    for(int i = 0; i < numOfThreads; i++){
        pthread_create(&cid[i],NULL,consumer,NULL);
    }

    //Join all threads
    for(int i = 0; i < numOfThreads; i++){
        pthread_join(cid[i],NULL);
    }
    pthread_join(pid,NULL);
    //Print the final output
    printOutput();
    
    return 0;
}