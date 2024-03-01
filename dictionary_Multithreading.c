#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<pthread.h>
#include<ctype.h>
int cnt=0, missed=0;
pthread_rwlock_t countlock;
char **wordarr;
// constants
int pages=26, pagelen=200000, datasetlength=50000;

// structure for the page
typedef struct node{
    void** words;
    int* flag;
    int wordcount;
}page;

page* dict;

// lock for a particular page
pthread_rwlock_t *pagelock;
// lock for the entire datastructute


// Creates a random string of size 6
char *generateword(){
    char *string = "abcdefghijklmnopqrstuvwxyz";
    int stringLen = strlen(string);        
    char *randomString = malloc(sizeof(char) * (7));
    if (randomString) {
        short key = 0;
        for (int n = 0;n < 6;n++) {            
            key = rand() % stringLen;          
            randomString[n] = string[key];
        }
        randomString[6] = '\0';
        return randomString;        
    }
}

// generates a 2d array of with number of pages
void* initDictionary(){
    pagelock=(pthread_rwlock_t*)malloc(sizeof(pthread_rwlock_t)*pages);
    dict=(page*)malloc(sizeof(page)*pages);
    for(int i=0;i<pages;i++){
        dict[i].words=(void**)malloc(sizeof(void*)*pagelen);
        dict[i].flag=(int*)calloc(pagelen, sizeof(int));
        dict[i].wordcount=0;
        pthread_rwlock_init(&pagelock[i], NULL);
    }
    return NULL;
}

// adds the word-menaing pair to the dictionary
int add(void* word, int pageNumber){
    // int pageNumber=tolower(((char*)word)[0])-'a';
    pthread_rwlock_wrlock(&pagelock[pageNumber]);
    // sleep(1);
    if(dict[pageNumber].wordcount==pagelen){
        pthread_rwlock_unlock(&pagelock[pageNumber]);
        return 0;
    }
    pthread_rwlock_wrlock(&countlock);
    cnt++;
    pthread_rwlock_unlock(&countlock);
    int i;
    dict[pageNumber].wordcount++;
    for(i=0;i<pagelen;i++){
        if((dict[pageNumber]).flag[i]==0)
            break;
    }
    dict[pageNumber].words[i]=word;
    dict[pageNumber].flag[i]=1;
    pthread_rwlock_unlock(&pagelock[pageNumber]);
    return 1;
}

// returns the meaning of a particular word

// gets the page number and bucket of a particular word in the table
int find(void* word){
    int pageNumber=tolower(((char*)word)[0])-'a';

    pthread_rwlock_wrlock(&pagelock[pageNumber]);
    for(int i=0;i<pagelen;i++){
        if(dict[pageNumber].flag[i]==1){
            if(strcmp((char*)word, dict[pageNumber].words[i])==0){
                return i;
            }
        }
    }
    pthread_rwlock_unlock(&pagelock[pageNumber]);
    return -1;
}

// // deletes and frees up the particular value from the table
int delete(void *word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    int index=find(word);
    if(index==-1)
        return 0;
    pthread_rwlock_wrlock(&pagelock[pageNumber]);
    dict[pageNumber].flag[index]=0;
    dict[pageNumber].words[index]=NULL;
    dict[pageNumber].wordcount--;
    pthread_rwlock_unlock(&pagelock[pageNumber]);
    return 1;
}

int deletepage(int pagenumber){
    pthread_rwlock_wrlock(&pagelock[pagenumber]);
    for(int i=0;i<pagelen;i++){
        if(dict[pagenumber].flag[i]==1){
            dict[pagenumber].flag[i]=0;
            dict[pagenumber].words[i]=NULL;
        }
    }
    dict[pagenumber].wordcount=0;
    pthread_rwlock_unlock(&pagelock[pagenumber]);
}

// prints the entire dictionary
void *print(){
    int count=0;
    for(int i=0;i<pages;i++){
        pthread_rwlock_wrlock(&pagelock[i]);
        for(int j=0;j<pagelen;j++){
            if(dict[i].flag[j]==1){
                printf("%s\n", (char*)dict[i].words[j]);
                count++;
            }
        }
        pthread_rwlock_unlock(&pagelock[i]);
        printf("\n");
    }
    printf("Totals prints : %d", count);
    fflush(stdout);
    return NULL;
}

void* addfunc(int index){
    for(int i=0;i<(datasetlength/4) && (i+index)<datasetlength;i++){
    // int pageNumber=tolower(((char*)word)[0])-'a';
    int pagenumber=tolower(((char*)wordarr[i+index])[0])-'a';
        add(wordarr[i+index], pagenumber);
    }
    return NULL;
}

int main(){
    struct timeval start, end;
    pthread_rwlock_init(&countlock, NULL);
    initDictionary();
    wordarr=(char**)malloc(sizeof(char*)*datasetlength);
    for(int i=0;i<datasetlength;i++){
        wordarr[i]=generateword();
    } 
    pthread_t *addingthreads=(pthread_t*)malloc(sizeof(pthread_t)*4);

    gettimeofday(&start, NULL);

    for(int j=0;j<4;j++)
        pthread_create(&addingthreads[j], NULL, addfunc, (datasetlength/4)*j);
    for(int j=0;j<4;j++)
        pthread_join(addingthreads[j], NULL);
    
    gettimeofday(&end, NULL);
    printf("Elements added : %d\n",cnt);
    printf("Time taken to add %d elements is : %.ld microseconds\n", datasetlength, (end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec));
    fflush(stdout);
    // for(int i=0;i<200;i++){
    //     int index=rand()%datasetlength;
    //     delete(wordarr[index]);
    // }
    // deletepage(25);
    // print();
}

// 16689
// 16350
// 16162
// 68917
