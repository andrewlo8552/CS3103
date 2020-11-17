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
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

clock_t start;

int numOfInputs;
int numOfFiles = 0;
int totalPages = 0;
int pageCapa = 10485760; //Capacity of each page: 10MB
int* numOfPagesPerFile; //Each file have how many pages
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; //To prevent deadlock
pthread_cond_t wakeUp = PTHREAD_COND_INITIALIZER, wait = PTHREAD_COND_INITIALIZER; //For consumer to wake and sleep
int producerFin = 0;
#define queueCapa 10 //Capacity of queue
int queueSize = 0; //Current size of queue
int queueHead = 0, queueTail = 0; //Pointer of head and tail of queue 
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
}*output;

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

//To check do inputs contain directory and return all file names under sub directory
char** checkDir(char** inputs){
  DIR * dir;
  //Dynamic array
  char** filenames = calloc(100,sizeof(char*));
  char* fn,fdir[50];
  struct dirent * ptr;
  //printf("numOfInputs: %d",numOfInputs);
  for(int i=0;i<numOfInputs;i++){
    //if inputs[i] is dir, put all filenames in dir to filenames
    dir = opendir(inputs[i]);
    if(dir!=NULL){
      while((ptr=readdir(dir))!=NULL){
        fn = ptr->d_name;
        if(!(fn[0]=='.')){
          strcpy(fdir,inputs[i]);
          strcat(fdir,"/");
          strcat(fdir,fn);
          filenames[numOfFiles] = malloc(strlen(fdir));
          memcpy(filenames[numOfFiles],fdir,strlen(fdir)+1);
          //printf("File dir%d: %s\n",numOfFiles,filenames[numOfFiles]);
          numOfFiles++;
        }        
      }
      closedir(dir);
    }
    else{
      *(filenames+numOfFiles) = inputs[i];
      //printf("File dir%d: %s\n",numOfFiles,filenames[numOfFiles]);
      numOfFiles++;
    }
  }
  return filenames;
}

//To put pages to queue for consumer
void* producer(void *arg){
  
  printf("Producer started\n");
  char** filenames = (char**)arg;
  int file;
  struct stat fileInfo;
  char* mapAddress;
  
  for(int i=0;i<numOfFiles;i++){
    printf("fn: %s\n",filenames[i]);
    file = open(filenames[i], O_RDONLY);
    printf("File opened\n");
    //numOfPages: number of pages in file
    //lastPageSize: size of last page
    int numOfPages = 0, pageSize = 0;
    
    //Stop if file not found
    if(file == -1){
      printf("Error: File didn't open\n");
      exit(1);
    }
    
    //Get the information of file
    printf("Before get file info.\n");
    fstat(file,&fileInfo);
    printf("After get file info.\n");
    //Divide file
    numOfPages = (fileInfo.st_size/pageCapa);
    
    printf("Before calculate the size of last page\n");
    //Calculate the size of last page
    if(((double)fileInfo.st_size/pageCapa)>numOfPages){ 
			numOfPages+=1;
			pageSize = fileInfo.st_size%pageCapa;
		}
		else{
			pageSize=pageCapa;
		}
    printf("After calculate the size of last page\n");
    
    printf("Before Update totalPages & numOfPagesPerFile\n");
    //Update totalPages & numOfPagesPerFile
    totalPages += numOfPages;
    printf("nop: %p\n",numOfPagesPerFile);
    numOfPagesPerFile[i] = numOfPages;
    printf("After Update totalPages & numOfPagesPerFile\n");
    
    //Map the file
    printf("Before file map\n");
    mapAddress = mmap(NULL, fileInfo.st_size, PROT_READ, MAP_SHARED, file, 0);
    printf("File mapped\n");
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
      struct buffer temp;
      //Check is last page
			if(x == numOfPages - 1){
				temp.pageSize = pageSize;
			}
			else{
				temp.pageSize = pageCapa;
			}
			temp.address = mapAddress;
			temp.pageNum = x;
			temp.fileNum = i;
			mapAddress += pageSize;
		  printf("Ready to put\n");
      //Put temp to queue
      pthread_mutex_lock(&lock);
			put(temp);
			pthread_mutex_unlock(&lock);
			printf("Put completed\n");
      //Wake up consumer to start
      pthread_cond_signal(&wakeUp);
    }
    close(file);
  }
  //Wake up consumer when producer finished
  //printf("producer finished\n");
  producerFin = 1;
  pthread_cond_signal(&wakeUp);
  printf("out producer\n");
  return 0;
}

//Do compression
void compress(struct buffer page){
  //Calculate position of printout
  printf("In compress\n");
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
  printf("pageSize: %d\n",page.pageSize);
  for(int i=0;i<page.pageSize;i++){
		tempString[count]=page.address[i];
		compressed.count[count]=1;
		//While next character is same and not last character, add count
    while((i+1<page.pageSize)&&(page.address[i]==page.address[i+1])){
			compressed.count[count]++;
			i++;
		}
    //Next character is not same, do next character compression
		count++;
	}
	compressed.size=count;
	compressed.data=realloc(tempString,count);
  
  //Put into output	
  output[position] = compressed;
  printf("Put into output\n");
}

//To get the pages from queue and compress
void *consumer(){
  //Work until producer is finish and queue is empty
  do{
    //Lock
    printf("consumer start");
    pthread_mutex_lock(&lock);
    
    //Wait until there is somethings in queue
    while((producerFin==0)&&(queueSize==0)){
      //Wait producer put pages to queue
      pthread_cond_signal(&wait);
      //Tell producer to start work
			pthread_cond_wait(&wakeUp,&lock);
    }
    //printf("Consumer wakeup\n");
    //If producer is finished and queue is empty
    if(producerFin==1 && queueSize==0){
			pthread_mutex_unlock(&lock);
			printf("out consumer");
      return NULL;
		}
    //Get page from queue
    printf("Get queue\n");
    struct buffer page = get();
    //Wake up producer if producer not finished
    if(producerFin == 0)
      pthread_cond_signal(&wait);
    
    //Unlock
    pthread_mutex_unlock(&lock);
    //Do compress
    compress(page);
    printf("Compressed\n");
    printf("producerFin=%d, queueSize=%d\n",producerFin,queueSize);
  }while((producerFin!=1)||(queueSize!=0));
  printf("out while loop\ns");
  return NULL;
}

//Print the final output
void printOutput(){
  //For printf
  printf("In printOutput()");
  char* finalOutput = malloc(totalPages*pageCapa*(sizeof(int)+sizeof(char)));
  char* startPoint = finalOutput;
  
   for(int i=0;i<totalPages;i++){
		//Check if last char of curr output == 1st char of next output
    if(i < totalPages-1){
			if(output[i].data[output[i].size-1]==output[i+1].data[0]){
        //Add curr count to next output
        output[i+1].count[0]+=output[i].count[output[i].size-1];
				//Prevent print out last char
        output[i].size--;
			}
		}
		
		for(int x=0;x<output[i].size;x++){
			//Count and char of output
      int num = output[i].count[x];
			char character = output[i].data[x];
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

int main(int argc, char** argv)
{
  // write your code here
  
  if(argc<2){
    printf("pzip: file1 [file2 ...]\n");
    exit(1);
  }
  
  //Create threads, create consumer by the number of processors
  numOfInputs = argc - 1;
  int numOfThreads = get_nprocs();
  char** filenames = checkDir(argv+1);
  //Give location for numOfPagesPerFile
  numOfPagesPerFile = malloc(sizeof(int)*numOfFiles);
  //Give location for output
  output = malloc(sizeof(struct output)*512000*2); 
  printf("Get filenames\n");
  pthread_t p, cid[numOfThreads-1];
  //Create producer
  pthread_create(&p, NULL, producer, filenames);
  //Create consumers
  for(int i = 0; i < numOfThreads; i++){
    //printf("Create consumer%d\n",i+1);
    pthread_create(&cid[i],NULL,consumer,NULL);
  }
  
  //Join all threads
  for(int i = 0; i < numOfThreads; i++){
    pthread_join(cid[i],NULL);
    printf("Join consumer%d",i);
  }
  pthread_join(p,NULL);
  printf("\njoined all thread\n");
  printOutput();
  
  return 0;
}
