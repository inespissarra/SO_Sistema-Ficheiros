#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 2000

#define THREADS 20

void write_ll(char *path){

    char input[SIZE];
    memset(input, 'A', SIZE);

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_write(fd, input, SIZE) == SIZE);

    assert(tfs_close(fd) != -1);

}

void read_ll(char *path){

    char output[SIZE];

    int fd = tfs_open(path, 0);
    assert(fd != -1 );

    assert(tfs_read(fd, output, SIZE) == SIZE);

    assert(tfs_close(fd) != -1);
}


int main() {

    char *path = "/f1";

    pthread_t tid[THREADS];


    assert(tfs_init() != -1);

    write_ll(path);

    for(int i = 0; i<THREADS; i++){
        if(pthread_create(&tid[i], NULL, (void*)read_ll, path) != 0)
            exit(EXIT_FAILURE);
    }

    for(int i = 0; i<THREADS; i++){
        pthread_join(tid[i], NULL);
    }

    printf("Sucessful test\n");

    return 0;
}