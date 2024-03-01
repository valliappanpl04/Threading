#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<pthread.h>
#include<ctype.h>
int cnt1=0, cnt2=0, missed=0;
pthread_rwlock_t countlock;
char **wordarr, **addwordarr;
// constants
int pages=26, pagelen=200000, datasetlength=500000, threads=8;

// structure for the page
typedef struct node{
    void** words;
    int* flag;
    int wordcount;
}page;

page *dict1,*dict2;

// lock for a particular page
pthread_rwlock_t *pagelock;
pthread_rwlock_t **indexlock;
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
    indexlock=(pthread_rwlock_t**)malloc(sizeof(pthread_rwlock_t)*pages);
    dict1=(page*)malloc(sizeof(page)*pages);
    dict2=(page*)malloc(sizeof(page)*pages);
    for(int i=0;i<pages;i++){
        dict1[i].words=(void**)malloc(sizeof(void*)*pagelen);
        dict1[i].flag=(int*)calloc(pagelen, sizeof(int));
        dict1[i].wordcount=0;
        dict2[i].words=(void**)malloc(sizeof(void*)*pagelen);
        dict2[i].flag=(int*)calloc(pagelen, sizeof(int));
        dict2[i].wordcount=0;
        indexlock[i]=(pthread_rwlock_t*)malloc(sizeof(pthread_rwlock_t)*pagelen);
        pthread_rwlock_init(&pagelock[i], NULL);
    }
    return NULL;
}

// adds the word to the dictionary
int add(void* word, int pageNumber){
    // int pageNumber=tolower(((char*)word)[0])-'a';
    pthread_rwlock_wrlock(&pagelock[pageNumber]);
    // sleep(1);
    if(dict1[pageNumber].wordcount==pagelen){
        pthread_rwlock_unlock(&pagelock[pageNumber]);
        return 0;
    }
    pthread_rwlock_wrlock(&countlock);
    cnt1++;
    pthread_rwlock_unlock(&countlock);
    int i;
    dict1[pageNumber].wordcount++;
    for(i=0;i<pagelen;i++){
        if((dict1[pageNumber]).flag[i]==0)
            break;
    }
    
    dict1[pageNumber].words[i]=word;
    dict1[pageNumber].flag[i]=1;
    pthread_rwlock_unlock(&pagelock[pageNumber]);
    return 1;
}

// returns the meaning of a particular word

// gets the page number and bucket of a particular word in the table
int find(void* word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    pthread_rwlock_wrlock(&pagelock[pageNumber]);
    for(int i=0;i<pagelen;i++){
        if(dict1[pageNumber].flag[i]==1){
            if(strcmp((char*)word, dict1[pageNumber].words[i])==0){
                return i;
            }
        }
    }
    pthread_rwlock_unlock(&pagelock[pageNumber]);
    return -1;
}

// // deletes up the particular value from the table
int delete(void *word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    int index=find(word);
    if(index==-1)
        return 0;
    pthread_rwlock_wrlock(&pagelock[pageNumber]);
    dict1[pageNumber].flag[index]=0;
    dict1[pageNumber].words[index]=NULL;
    dict1[pageNumber].wordcount--;
    pthread_rwlock_unlock(&pagelock[pageNumber]);
    return 1;
}


// deletes one complete page in the dictionary
int deletepage(int pagenumber){
    pthread_rwlock_wrlock(&pagelock[pagenumber]);
    for(int i=0;i<pagelen;i++){
        if(dict1[pagenumber].flag[i]==1){
            dict1[pagenumber].flag[i]=0;
            dict1[pagenumber].words[i]=NULL;
        }
    }
    dict1[pagenumber].wordcount=0;
    pthread_rwlock_unlock(&pagelock[pagenumber]);
}

// prints the entire dictionary
void *print(){
    int count=0;
    for(int i=0;i<pages;i++){
        pthread_rwlock_wrlock(&pagelock[i]);
        for(int j=0;j<pagelen;j++){
            if(dict1[i].flag[j]==1){
                printf("%s\n", (char*)dict1[i].words[j]);
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

// intermediate function for adding data
void* addfunc(int index){
    int localcount=0;
    for(int i=0;i<(datasetlength/threads) && (i+index)<datasetlength;i++){
        int pagenumber=tolower(((char*)wordarr[i+index])[0])-'a';
        add(wordarr[i+index], pagenumber);
    }
    return NULL;
}


int pos=0;
pthread_rwlock_t poslock;

// calls random function
void* generatefunc(){
    for(int i=0;i<1000;i++){
        int f=rand()%3;
        pthread_rwlock_wrlock(&poslock);
        int in=(pos++)%datasetlength;
        pthread_rwlock_unlock(&poslock);
        switch(f){
            case 1:
                int pagenumber=tolower(((char*)wordarr[in])[0])-'a';
                add(addwordarr[in], pagenumber);
                break;
            case 2:
                delete(wordarr[rand()%datasetlength]);
                break;
            case 3:
                find(wordarr[in]);
                break;
        }
    }
}


// sequential functions
// adds data onto the dictionary
int addseq(void* word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    if(dict2[pageNumber].wordcount==pagelen)
        return -1;
    int i;
    dict2[pageNumber].wordcount++;
    for(i=0;i<pagelen;i++){
        if((dict2[pageNumber]).flag[(i)%pagelen]==0)
            break;
    }
    cnt2++;
    dict2[pageNumber].words[i]=word;
    dict2[pageNumber].flag[i]=1;
    return 1;
}

// finds the index of the data in the dictionary
int findseq(void* word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    for(int i=0;i<pagelen;i++){
        if(dict2[pageNumber].flag[i]==1){
            if(strcmp((char*)word, dict2[pageNumber].words[i])==0){
                return i;
            }
        }
    }
    return -1;
}

// deletes data from the table
int deleteseq(void *word){
    int pageNumber=tolower(((char*)word)[0])-'a';
    int index=findseq(word);
    if(index==-1)
        return 0;
    dict2[pageNumber].flag[index]=0;
    dict2[pageNumber].words[index]=NULL;
    dict2[pageNumber].wordcount--;
    return 1;
}

// deletes the entire page from the dictionary
int deletepageseq(int pagenumber){
    for(int i=0;i<pagelen;i++){
        if(dict2[pagenumber].flag[i]==1){
            dict2[pagenumber].flag[i]=0;
            dict2[pagenumber].words[i]=NULL;
        }
    } 
    dict2[pagenumber].wordcount=0;
}

int main(){
    struct timeval start, end;
    pthread_rwlock_init(&countlock, NULL);
    initDictionary();
    wordarr=(char**)malloc(sizeof(char*)*datasetlength);
    addwordarr=(char**)malloc(sizeof(char*)*datasetlength);
    for(int i=0;i<datasetlength;i++){
        wordarr[i]=generateword();
    } 
    for(int i=0;i<datasetlength;i++){
        addwordarr[i]=generateword();
    } 
    pthread_t *addingthreads=(pthread_t*)malloc(sizeof(pthread_t)*threads);

    gettimeofday(&start, NULL);

    for(int j=0;j<threads;j++)
        pthread_create(&addingthreads[j], NULL, addfunc, (int)(datasetlength/threads)*j);
    for(int j=0;j<threads;j++)
        pthread_join(addingthreads[j], NULL);
    
    gettimeofday(&end, NULL);

    printf("Elements added in multithreading : %d\n",cnt1);
    printf("Time taken to add %d elements is : %.6f seconds\n", datasetlength, ((end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec))/1000000.0);
    fflush(stdout);

    pthread_t *th=(pthread_t*)malloc(sizeof(pthread_t)*threads);
    gettimeofday(&start, NULL);
    for(int i=0;i<threads;i++){
        pthread_create(&th[i], NULL, generatefunc, NULL);
    }
    for(int i=0;i<threads;i++){
        pthread_join(th[i], NULL);
    }
    gettimeofday(&end, NULL);

    printf("Time taken is : %.6f seconds\n", ((end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec))/1000000.0);
    printf("\n\n\n");
    fflush(stdout);

    // code for sequential execution

    gettimeofday(&start, NULL);
    for(int i=0;i<datasetlength;i++){
        addseq(wordarr[i]);
    }
    gettimeofday(&end, NULL);
    printf("Number of words added in sequence : %d\n", cnt2);
    printf("Time taken to add %d elements in sequence is : %.6f seconds\n", datasetlength, ((end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec))/1000000.0);
    int position=0;

    gettimeofday(&start, NULL);
    for(int i=0;i<threads*1000;i++){
        int f=rand()%3;
        position++;
        switch(f){
            case 1:
                addseq(addwordarr[position]);
                break;
            case 2:
                deleteseq(wordarr[rand()%datasetlength]);
                break;
            case 3:
                findseq(wordarr[position]);
                break;
        }
    }
    gettimeofday(&end, NULL);
    printf("Time taken is : %.6f seconds\n",((end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec))/1000000.0);
    fflush(stdout);
    printf("\n\n\n");
    fflush(stdout);
}
