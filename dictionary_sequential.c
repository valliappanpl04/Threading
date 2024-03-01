#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<pthread.h>
#include<ctype.h>

// constants
int pages=26, pagelen=200000, datasetlength=50000;
int cnt=0;
// structure for the page
typedef struct node{
    void** words;
    int* flag;
    int wordcount;
}page;

// lock for a particular page
pthread_rwlock_t *pagelock;
// lock for the entire datastructute
pthread_rwlock_t **lock;

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
page* initDictionary(){
    pagelock=(pthread_rwlock_t*)malloc(sizeof(pthread_rwlock_t)*pages);

    page* dict=(page*)malloc(sizeof(page)*pages);
    for(int i=0;i<pages;i++){
        dict[i].words=(void**)malloc(sizeof(void*)*pagelen);
        dict[i].flag=(int*)calloc(pagelen, sizeof(int));
        dict[i].wordcount=0;
    }
    return dict;
}

// adds the word-menaing pair to the dictionary
int add(page** dict, void* word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    // pthread_rwlock_wrlock(&pagelock[pageNumber]);
    if((*dict)[pageNumber].wordcount==pagelen)
        return -1;
    int i;
    (*dict)[pageNumber].wordcount++;
    for(i=0;i<pagelen;i++){
        if(((*dict)[pageNumber]).flag[(i)%pagelen]==0)
            break;
    }
    cnt++;
    (*dict)[pageNumber].words[i]=word;
    (*dict)[pageNumber].flag[i]=1;
    // pthread_rwlock_unlock(&pagelock[pageNumber]);
    return 1;
}

// returns the meaning of a particular word

// gets the page number and bucket of a particular word in the table
int find(page* dict, void* word){
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
int delete(page** dict, void *word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    int index=find(*dict, word);
    if(index==-1)
        return 0;
    pthread_rwlock_wrlock(&pagelock[pageNumber]);
    (*dict)[pageNumber].flag[index]=0;
    (*dict)[pageNumber].words[index]=NULL;
    (*dict)[pageNumber].wordcount--;
    pthread_rwlock_unlock(&pagelock[pageNumber]);
    return 1;
}

int deletepage(page** dict, int pagenumber){
    for(int i=0;i<pagelen;i++){
        if((*dict)[pagenumber].flag[i]==1){
            (*dict)[pagenumber].flag[i]=0;
            (*dict)[pagenumber].words[i]=NULL;
        }
    } 
    (*dict)[pagenumber].wordcount=0;
}

// prints the entire dictionary
void print(page* dict){
    for(int i=0;i<pages;i++){
        pthread_rwlock_wrlock(&pagelock[i]);
        for(int j=0;j<pagelen;j++){
            if(dict[i].flag[j]==1){
                printf("%s\n", (char*)dict[i].words[j]);
            }
        }
        pthread_rwlock_unlock(&pagelock[i]);
        printf("\n");
    }
    fflush(stdout);
}

int main(){
    struct timeval start, end;
    page* dict=initDictionary();
    char **wordarr=(char**)malloc(sizeof(char*)*datasetlength);
    for(int i=0;i<datasetlength;i++){
        wordarr[i]=generateword();
    } 
// 2527449
    gettimeofday(&start, NULL);
    for(int i=0;i<datasetlength;i++){
        add(&dict, wordarr[i]);
    }
    gettimeofday(&end, NULL);
    printf("Number of words added : %d\n", cnt);
    printf("Time taken to add %d elements is : %.ld microseconds\n", datasetlength, (end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec));
    fflush(stdout);
    // for(int i=0;i<200;i++){
    //     int index=rand()%datasetlength;
    //     delete(&dict, wordarr[index]);
    // }
    // deletepage(&dict, 25);
    // print(dict);
    // for(int i=0;i<100;i++){
    //     int index=rand()%500;
    //     delete(&dict, wordarr[index]);
    // }
    // delete(&dict, wordarr[0]);
    // sleep(5);
    // for(int i=0;i<=current_page;i++){
    //     int t=pagelen[i];
    //     for(int j=0;j<pagelen[i];j++){
    //         if(dict[i][j].flag==1)
    //             printf("%s-%s\n", (char*)dict[i][j].word, (char*)dict[i][j].meaning);
    //         else
    //             printf("-:-\n");
    //         fflush(stdout);
    //     }
    //     printf("\n");
    // }
    
}