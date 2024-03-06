#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>

#define H 32
#define noofSegments x
#define segmentSize y
#define datasize z
#define upperLimit 100000000
#define lowerLimit 1000
#define Noofthreads 10
// int x=8,y=8388608,z=50000000;
int hHsize=0, segsize=0;
// int x=4,y=16,z=12;
int x=2, y=16, z=33;
char** wordarr;
int* keyarr;
int* loadfactor=NULL;
int load=0;

pthread_rwlock_t *segmentlock;
pthread_rwlock_t loadlock;

typedef struct hashtable{
    void* key;
    void* val;
    int hop_info;
    int flag;
}table;
table** arr;

// returns a table with size mentioned in the macros
void* initializeTable(){
    arr=(table**)malloc(sizeof(table*)*noofSegments);
    segmentlock=(pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t)*noofSegments);
    for(int i=0;i<noofSegments;i++){
        arr[i]=(table*)malloc(sizeof(table)*segmentSize);
        for(int j=0;j<segmentSize;j++){
            arr[i][j].flag=0;
            arr[i][j].hop_info=0;
        }
    }
    return NULL;
}

// deletes and free up space of the table
void deleteTable(table*** arr){
    for(int i=0;i<noofSegments;i++){
        free((*arr)[i]);
    }
    free(*arr);
}

// returns hash value 
__uint64_t hashfunction(void* key , int len){
    const __uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;
    long long int seed=123;
    __uint64_t h = seed ^ (len * m);

    const __uint64_t * data = (const __uint64_t *)key;
    const __uint64_t * end = data + (len/8);

    while(data != end)
    {
    __uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
    }

    const unsigned char * data2 = (const unsigned char*)data;
 
    switch(len & 7) {
        case 7: h ^= (__uint64_t)((__uint64_t)data2[6] << (__uint64_t)48);
        case 6: h ^= (__uint64_t)((__uint64_t)data2[5] << (__uint64_t)40);
        case 5: h ^= (__uint64_t)((__uint64_t)data2[4] << (__uint64_t)32);
        case 4: h ^= (__uint64_t)((__uint64_t)data2[3] << (__uint64_t)24);
        case 3: h ^= (__uint64_t)((__uint64_t)data2[2] << (__uint64_t)16);
        case 2: h ^= (__uint64_t)((__uint64_t)data2[1] << (__uint64_t)8 );
        case 1: h ^= (__uint64_t)((__uint64_t)data2[0]);
        h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    h*=m;
    h^=h>>r;
    return h;
}

// generates a random string
char* generateStr(){
    // static int mySeed = 25011984;
    char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
    int stringLen = strlen(string);        
    char *randomString = malloc(sizeof(char) * (11));
    // srand(time(NULL) * 10 + ++mySeed);
    // srand(time(0));
    if (randomString) {
        short key = 0;
        for (int n = 0;n < 10;n++) {            
            key = rand() % stringLen;          
            randomString[n] = string[key];
        }
        randomString[10] = '\0';
        return randomString;        
    }
}

// generates a random number within a range
int generateInt(){
    // srand(time(0));
    return (rand()%(upperLimit-lowerLimit))+lowerLimit;
}



int contains(table** arr, void* key){
    unsigned long long int hash=hashfunction(key, sizeof(int));
    int seg=hash&(noofSegments-1);
    int buck=hash&(segmentSize-1);
    pthread_rwlock_rdlock(&segmentlock[seg]);
    int hopinfo=arr[seg][buck].hop_info;
    pthread_rwlock_unlock(&segmentlock[seg]);
    int mask=1;
    for(int i=0;i<H;i++){
        if((hopinfo>>i)&1==1){
            int s=(seg+((buck+i)/segmentSize))%noofSegments,b=(buck+i)%segmentSize;
            pthread_rwlock_rdlock(&segmentlock[s]);
            if(*(int*)arr[s][b].key==*(int*)key){
                pthread_rwlock_unlock(&segmentlock[s]);
                return 1;
            }
            pthread_rwlock_unlock(&segmentlock[s]);
        }
    }
    return 0;
}
// adds element to the table
int threadid=0;
pthread_rwlock_t threadlock;

int add(table*** tab, void* key, void *val, int id){
    // printf("Thread id : %d, adding data : %d\n", id, *(int*)key);
    if(contains(*tab, key)){
        // printf("Thread id : %d, key: %d is already present\n", id,*(int*)key);
        return -2;
    }
    // printf("Thread id : %d, key: %d is not present and adding it\n", id,*(int*)key);
    int total=noofSegments*segmentSize;
    unsigned long long int hash=hashfunction(key, sizeof(int));
    int seg=hash&(noofSegments-1);
    int buck=hash&(segmentSize-1);
    // int seg=hash%noofSegments;
    // int buck=hash%segmentSize;
    int current_pos=(seg*segmentSize)+buck;
    int hop=0,i;
    for(i=0;i<total;i++){
        int index=(i+current_pos)%(total);
        int s=index/segmentSize,b=index%segmentSize;
        // printf("segment %d locked\n", s);
        pthread_rwlock_wrlock(&segmentlock[s]);
        if((*tab)[s][b].flag==0){
            pthread_rwlock_unlock(&segmentlock[s]);
            // printf("segment %d unlocked\n", s);
            break;
        }
        pthread_rwlock_unlock(&segmentlock[s]);
        // printf("segment %d unlocked\n", s);
        hop++;
    }
    if(i==total){
        printf("Segmentsize failed");
        segsize++;
        return 0;
    }
    int free_index=(current_pos+hop)%(total);
    while(1){
        if(free_index<current_pos)
            free_index+=total;
        if(current_pos+H-1>=free_index){
            int s=(free_index%total)/segmentSize,b=(free_index%total)%segmentSize;
            // printf("segment %d locked\n", s);
            pthread_rwlock_wrlock(&segmentlock[s]);
                (*tab)[s][b].flag=1;
                (*tab)[s][b].val=val;
                (*tab)[s][b].key=key;
                loadfactor[s]++;
            pthread_rwlock_unlock(&segmentlock[s]);
            // printf("segment %d unlocked\n", s);

            pthread_rwlock_wrlock(&loadlock);
                load++;
            pthread_rwlock_unlock(&loadlock);

            // printf("segment %d locked\n", seg);
            pthread_rwlock_wrlock(&segmentlock[seg]);
                (*tab)[seg][buck].hop_info|=(1<<(free_index-current_pos));
            pthread_rwlock_unlock(&segmentlock[seg]);
            // printf("segment %d unlocked\n", seg);

            return 1;
        }
        int step=1;
        while(step<H){
            int xindex=(free_index-H+step+total)%total;
            int s=xindex/segmentSize,b=xindex%segmentSize;
            pthread_rwlock_wrlock(&segmentlock[s]);
            unsigned long long int xhash=(hashfunction((*tab)[s][b].key, sizeof(int)));
            pthread_rwlock_unlock(&segmentlock[s]);
            int xseg=xhash&(noofSegments-1),xbuck=xhash&(segmentSize-1);
            // int xseg=xhash%noofSegments, xbuck=xhash%segmentSize;
            int xcur_pos=(xseg*segmentSize)+xbuck;
            if(free_index<xcur_pos){
                free_index+=total;
            }
            if(xcur_pos+H-1>=free_index){
                int s1=(free_index%total)/segmentSize,b1=(free_index%total)%segmentSize;
                if(s==s1)
                    pthread_rwlock_wrlock(&segmentlock[s]);
                else{
                    pthread_rwlock_wrlock(&segmentlock[s]);
                    pthread_rwlock_wrlock(&segmentlock[s1]);
                }
                (*tab)[s1][b1].flag=1;
                (*tab)[s1][b1].key=(*tab)[s][b].key;
                (*tab)[s1][b1].val=(*tab)[s][b].val;

                (*tab)[s][b].flag=0;
                (*tab)[s][b].val=NULL;
                (*tab)[s][b].key=NULL;
                if(s==s1)
                    pthread_rwlock_unlock(&segmentlock[s]);
                else{
                    pthread_rwlock_unlock(&segmentlock[s]);
                    pthread_rwlock_unlock(&segmentlock[s1]);
                }
                pthread_rwlock_wrlock(&segmentlock[xseg]);
                (*tab)[xseg][xbuck].hop_info&=~(1<<(xindex-xcur_pos));
                (*tab)[xseg][xbuck].hop_info|=(1<<(free_index-xcur_pos));
                pthread_rwlock_unlock(&segmentlock[xseg]);
                free_index=xindex;
                break;
            }
            step++;
        }
        if(step==H){
            pthread_rwlock_wrlock(&loadlock);
            printf("Load factor is : %f%%\n", ((float)load/(float)total)*100);
            pthread_rwlock_unlock(&loadlock);
            printf("Load factor of the segments are : \n");
            for(int i=0;i<noofSegments;i++){
                pthread_rwlock_wrlock(&segmentlock[i]);
                printf("segment number %d : %f%% \n",i,  ((float)loadfactor[i]/(float)segmentSize)*100);
                pthread_rwlock_unlock(&segmentlock[i]);
            }
            hHsize++;
            return 0;
        }
    }
}

// resizes by doubling the size of the segment
table** resize(table*** arr){
    y*=2;
    load=0;
    table** temp=(table**)malloc(sizeof(table*)*noofSegments);
    for(int i=0;i<noofSegments;i++){
        temp[i]=(table*)malloc(sizeof(table)*segmentSize);
        for(int j=0;j<segmentSize;j++){
            temp[i][j].flag=0;
            temp[i][j].hop_info=0;
        }
    }
    for(int i=0;i<noofSegments;i++){
        loadfactor[i]=0;
        // pthread_wrlock
        for(int j=0;j<segmentSize/2;j++){
            if((*arr)[i][j].flag==1)
                add(&temp, (*arr)[i][j].key, (*arr)[i][j].val, -1);
        }
    }
    y/=2;
    deleteTable(arr);
    y*=2;
    *arr=temp;
}

// to print the entire table
void print(table **arr){
    for(int i=0;i<noofSegments;i++){
        for(int j=0;j<segmentSize;j++){
            if(arr[i][j].flag==1)
                printf("%d:%s  ",*((int*)arr[i][j].key), (char*)arr[i][j].val);
            else    
                printf(" -:-  ");
        }
        printf("\n\n");
    }
    printf("\n");
}

//  deletes the entire data
void deleteall(table** arr){
    for(int i=0;i<noofSegments;i++){
        for(int j=0;j<segmentSize;j++){
            if(arr[i][j].flag==1){
                free(arr[i][j].val);
            }
        }
    }
}

char* get(table** arr, void* key){
    unsigned long long int hash=hashfunction(key, sizeof(int));
    int seg=hash&(noofSegments-1);
    int buck=hash&(segmentSize-1);
    int hopinfo=arr[seg][buck].hop_info;
    int mask=1;
    for(int i=0;i<H;i++,mask<<=1){
        if((hopinfo>>i)&1==1){
            int s=(seg+((buck+i)/segmentSize))%noofSegments,b=(buck+i)%segmentSize;
            if(*(int*)arr[s][b].key==*(int*)key)
                return (char*)arr[s][b].val;
        }
    }
    return 0;
}
int s=0,f=0,d=0;
pthread_rwlock_t addlock;
void* addfunction(){
    pthread_rwlock_wrlock(&threadlock);
    int thid=threadid++;
    pthread_rwlock_unlock(&threadlock);
    // for(int i=start;i<(start+(datasize/Noofthreads))&& i<datasize; i++){
    while(thid<datasize){
        // add(wordarr[i], keyarr[i]);
        int in=add(&arr, &keyarr[thid], wordarr[thid], thid);
        if(in==1){
            pthread_rwlock_wrlock(&addlock);
            s++;
            pthread_rwlock_unlock(&addlock);
        }
        else if(in==-2){
            pthread_rwlock_wrlock(&addlock);
            d++;
            pthread_rwlock_unlock(&addlock);
        }
        else{
            pthread_rwlock_wrlock(&addlock);
            f++;
            pthread_rwlock_unlock(&addlock);
        }
        thid+=Noofthreads;
    }
}
int main(){
    initializeTable();
    wordarr=(char**)malloc(sizeof(char*)*datasize);
    keyarr=(int*)malloc(sizeof(int)*datasize);
    loadfactor=(int*)calloc(noofSegments, sizeof(int));
    int failedCount=0,resizeCount=0,present=0;

    for(int i=0;i<datasize;i++){
        wordarr[i]=generateStr();
        keyarr[i]=generateInt();
    }
    // for(int i=0;i<datasize;i++){
    //     printf("%d:%s | ",keyarr[i], wordarr[i]);
    // }
    struct timeval start, end;
    gettimeofday(&start, NULL);
    pthread_t *threads=(pthread_t*)malloc(sizeof(pthread_t)*Noofthreads);
    for(int i=0;i<Noofthreads;i++){
        pthread_create(&threads[i], NULL, addfunction, NULL);
    }
    for(int i=0;i<Noofthreads;i++){
        pthread_join(threads[i], NULL);
    }
    printf("Success : %d, failed : %d, duplicate : %d\n", s,f,d);
    // for(int i=0;i<datasize;i++){
    //     // wordarr[i]=generateStr();
    //     // keyarr[i]=generateInt();
    //     int in=add(&keyarr[i], wordarr[i]);
    //     if(in==0){  
    //         resizeCount++;
    //         arr=resize(&arr);
    //         add(&keyarr[i],wordarr[i]);
    //     }
    //     if(in==-1)
    //         failedCount++;
    //     if(in==-2){
    //         present++;
    //     }
    // }

    gettimeofday(&end, NULL);
    printf("execution time is : %ld microseconds\n", (end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec));
   
    struct timeval st,en;
    gettimeofday(&st, NULL);
    int failedToFetch=0;
    for(int i=0;i<datasize;i++){
        if(!contains(arr, &keyarr[i]))
            failedToFetch++;
    }
    gettimeofday(&en, NULL);
    printf("execution time of contains call is : %.2f seconds\n", ((en.tv_sec-st.tv_sec)*1000000L+(en.tv_usec-st.tv_usec))/1000000.0);
    
    // print(arr);

    // struct timeval getst, getend;
    // gettimeofday(&getst, NULL);
    // int eq=0;
    // for(int i=0;i<datasize;i++){
    //     if(strcmp(get(arr, &keyarr[i]), wordarr[i])==0)
    //         eq++;
    // }
    // gettimeofday(&getend, NULL);
    // printf("No of elements with same value : %d\n",eq);
    // printf("execution time of get call is : %.2f seconds\n", ((getend.tv_sec-getst.tv_sec)*1000000L+(getend.tv_usec-getst.tv_usec))/1000000.0);
    

    // int total=noofSegments*segmentSize;
    // printf("Segment size : %d\nNo of resizes(failed) : %d\n",segmentSize,resizeCount);
    // printf("No of failed : %d\n",failedCount);
    // printf("Elements failed to fetch : %d\n",failedToFetch);
    // printf("Duplicate elements : %d\n",present);
    // printf("No of resizes due to H size overflow : %d\n",hHsize);
    // printf("No of resizes due to segment size overflow : %d\n", segsize);
    // printf("Load factor after adding all the elements is : %f%%\n",((float)load/(float)total)*100);
    // for(int i=0;i<100;i++){
    //     struct timeval start, end;
    //     gettimeofday(&start, NULL);
    //     if(contains(arr, generateInt()));
    //     gettimeofday(&end, NULL);
    //     printf("execution time is : %ld microseconds\n", (end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec));
    // }


    // int notpresent=0, check=0;
    // for(int i=0;i<100000;i++){
    //     int key=generateInt();
    //     if(!contains(arr, key)){
    //         notpresent++;
    //         int flag=0;
    //         for(int j=0;j<datasize;j++){
    //             if(keyarr[j]==key)
    //                 flag=1;
    //         }
    //         if(flag==0)
    //             check++;
    //     }
    // }
    // printf("No of elements not present in the array : %d\nNo of elements not present in the tabel : %d\n", check, notpresent);

    // free(wordarr);
    // free(keyarr);
    // deleteall(arr);
    // deleteTable(&arr);
    
}