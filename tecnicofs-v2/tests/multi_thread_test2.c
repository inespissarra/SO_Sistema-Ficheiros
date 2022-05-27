#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 5

#define THREADS 20

void write_ll(char *path){

    char *input = "AAA! AAA! AAA! ";

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_write(fd, input, strlen(input)) == strlen(input));

    assert(tfs_close(fd) != -1);

}

void trunc_ll(char *path){
    int fd = tfs_open(path, TFS_O_TRUNC);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

void append_ll(char *path){
    int fd = tfs_open(path, TFS_O_APPEND);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}

void create_ll(char *path){
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}


int main() {

    char *path = "/f1";
    char path2[4];

    pthread_t tid[THREADS];

    assert(tfs_init() != -1);

    write_ll(path);

    for(int i = 0; i<THREADS/3; i++){
        sprintf(path2,"/f%d", i+1);
        if(pthread_create(&tid[i], NULL, (void*)create_ll, path2) != 0)
            exit(EXIT_FAILURE);
    }
    for(int i = THREADS/3; i<(THREADS*2)/3; i++){
        if(pthread_create(&tid[i], NULL, (void*)append_ll, path) != 0)
            exit(EXIT_FAILURE);
    }
    for(int i = (THREADS*2)/3; i<THREADS; i++){
        if(pthread_create(&tid[i], NULL, (void*)trunc_ll, path) != 0)
            exit(EXIT_FAILURE);
    }

    for(int i = 0; i<THREADS; i++){
        pthread_join(tid[i], NULL);
    }

    printf("Sucessful test\n");

    return 0;
}