#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<pthread.h>

int pages=25, pagesize=25, datasetlength=1000;
int current_page=0;

// lock for a particular page
pthread_rwlock_t *pagelock;
pthread_rwlock_t **lock;
// defines the structure
typedef struct defenition{
    void *word, *meaning;
    int flag;
}dictionary;

// array required to store number of elements added to a page
int *pagelen;


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

// generates randoms string of size 15
char *generatemeaning(){
    char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
    int stringLen = strlen(string);        
    char *randomString = malloc(sizeof(char) * (15));
    if (randomString) {
        short key = 0;
        for (int n = 0;n < 15;n++) {            
            key = rand() % stringLen;          
            randomString[n] = string[key];
        }
        randomString[16] = '\0';
        return randomString;        
    }
}

// generates a 2d array of with number of pages
dictionary** initDictionary(){
    pagelock=(pthread_rwlock_t *)malloc(sizeof)
    
    pagelen=(int*)malloc(pages*sizeof(int));
    dictionary** dict=(dictionary**)malloc(sizeof(dictionary*)*pages);
    for(int i=0;i<pages;i++){
        dict[i]=(dictionary*)malloc(sizeof(dictionary)*pagesize);
        for(int j=0;j<pagesize;j++){
            dict[i][j].flag=0;
        }
    }
    return dict;
}

// adds the word-menaing pair to the dictionary
int add(dictionary*** dict, void* word, void* meaning){
    if(pagelen[current_page]==pagesize)
        current_page++;
    int c=current_page, l=pagelen[current_page]++;
    (*dict)[c][l].word=word;
    (*dict)[c][l].meaning=meaning;
    (*dict)[c][l].flag=1;
}

// returns the meaning of a particular word
char* meaningof(dictionary** dict, void *word){
    for(int i=0;i<=current_page;i++){
        for(int j=0;j<pagelen[i];j++){
            if(dict[i][j].flag==1 && strcmp((char*)dict[i][j].word,(char*)word)==0){
                return (char*)dict[i][j].meaning;
            }
        }
    }
    return NULL;
}

// gets the page number and bucket of a particular word in the table
void getpage(int* p, int *b, dictionary** dict, void* word){
    for(int i=0;i<=current_page;i++){
        for(int j=0;j<pagelen[i];j++){
            if(dict[i][j].flag==1 && strcmp((char*)dict[i][j].word, (char*)word)==0){
                *p=i;*b=j;
                return;
            }
        }
    }
}


// deletes and frees up the particular value from the table
void delete(dictionary*** dict, void* word){
    int p,b;
    getpage(&p,&b,*dict, word);
    free((*dict)[p][b].word);
    free((*dict)[p][b].meaning);
    (*dict)[p][b].flag=0;
}


int main(){
    dictionary** dict=initDictionary();
    char **wordarr=(char**)malloc(sizeof(char*)*datasetlength);
    char **meaningarr=(char**)malloc(sizeof(char*)*datasetlength);
    for(int i=0;i<datasetlength;i++){
        wordarr[i]=generateword();
        meaningarr[i]=generatemeaning();
    } 
    for(int i=0;i<500;i++){
        add(&dict, wordarr[i], meaningarr[i]);
    }
    for(int i=0;i<pages;i++)
        printf("%d ", pagelen[i]);
    fflush(stdout);
    
    for(int i=0;i<100;i++){
        printf("Meaning of %s is %s\n", wordarr[i], meaningof(dict, wordarr[i]));
    }
    for(int i=0;i<100;i++){
        int index=rand()%500;
        delete(&dict, wordarr[index]);
    }
    delete(&dict, wordarr[0]);
    sleep(5);
    for(int i=0;i<=current_page;i++){
        int t=pagelen[i];
        for(int j=0;j<pagelen[i];j++){
            if(dict[i][j].flag==1)
                printf("%s-%s\n", (char*)dict[i][j].word, (char*)dict[i][j].meaning);
            else
                printf("-:-\n");
            fflush(stdout);
        }
        printf("\n");
    }
    
}