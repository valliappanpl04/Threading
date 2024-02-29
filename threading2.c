
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/time.h>
#define size 1000
int arr[size*2];
pthread_rwlock_t lock[size];
pthread_rwlock_t toplock, countlock;
int top=0, count=0;
void* add(void* args){
    pthread_rwlock_wrlock(&countlock);
    int cnt=count;
    count+=1;
    // printf("%d \n",count);
    // sleep(1);
    pthread_rwlock_unlock(&countlock);

    pthread_rwlock_wrlock(&toplock);
        int index=top;
        top++;
        printf("threadid : %d,  Added, top = %d\n",cnt, top);
    pthread_rwlock_unlock(&toplock);
    
    pthread_rwlock_wrlock(&lock[index]);
        int element=*(int*)args;
        arr[index]=element;
        fflush(stdout);
    pthread_rwlock_unlock(&lock[index]);
    printf("threadid : %d, Add successful for %d\n", cnt,element);
    return NULL;
}
void* print(void* args){
    pthread_rwlock_wrlock(&toplock);
        int index=top;
    pthread_rwlock_unlock(&toplock);

    pthread_rwlock_rdlock(&lock[index]);
    for(int i=0;i<index;i++){
        printf("%d ", arr[i]);
    }
    printf("\n");
    pthread_rwlock_unlock(&lock[index]);

    printf("Print successful\n");
    fflush(stdout);
    return NULL;
}
void* pop(void* args){
    pthread_rwlock_wrlock(&toplock);
        int index=top;
        if(index==0){
            pthread_rwlock_unlock(&toplock);
            return NULL;
        }
        top--;
    pthread_rwlock_unlock(&toplock);

    pthread_rwlock_wrlock(&lock[index-1]);
        arr[index-1]=-1;
    pthread_rwlock_unlock(&lock[index-1]);

    printf("Pop successful\n");
    return NULL;
}
int main(){
    struct timeval start, end;
    gettimeofday(&start, NULL);
    pthread_rwlock_init(&countlock,NULL);
    pthread_t threads[size/2];
    for(int i=0;i<size/2;i++){
        // pthread_t thread;
        int* a=(int*)malloc(sizeof(int));
        *a=i+100;
        pthread_create(&threads[i], NULL, &add, a);
    }
    for(int i=0;i<size/2;i++){
        pthread_join(threads[i], NULL);
    }
    sleep(4);
    // print(NULL);
    // fflush(stdout);
    pthread_t additionalthreads[size];
    pthread_t popthread[size/2], printthread[size/2];
    for(int i=0;i<size/2;i++){
        // pthread_t t1, t2, t3, t4;
        int* a=(int*)malloc(sizeof(int));
        *a=i;
        pthread_create(&additionalthreads[(i*2)], NULL, &add, a);
        pthread_create(&popthread[i], NULL, &pop, NULL);
        pthread_create(&printthread[i], NULL, &print, NULL);
        pthread_create(&additionalthreads[(i*2)+1], NULL, &add, a);
        // print(NULL);
    }
    for(int i=0;i<size;i++){
        pthread_join(additionalthreads[i], NULL);
    }
    for(int i=0;i<size/2;i++){
        pthread_join(printthread[i], NULL);
        pthread_join(popthread[i], NULL);
    }
    gettimeofday(&end, NULL);
    sleep(4);
    printf("No of threads : %d\n", count);
    printf("Executiong time is : %ld microseconds\n", (end.tv_sec-start.tv_sec)*1000000L+(end.tv_usec-start.tv_usec));
    pthread_exit(NULL);
}