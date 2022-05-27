#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 5

#define THREADS 7

void write_ll(char *path){

    char *str = "AAA! AAA! AAA! ";

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_write(fd, str, strlen(str)) == strlen(str));

    assert(tfs_close(fd) != -1);

}

void write_ll2(char *path){

    char input[SIZE];
    memset(input, 'A', SIZE);

    int fd = tfs_open(path, TFS_O_APPEND);
    assert(fd != -1);

    assert(tfs_write(fd, input, SIZE) == SIZE);

    assert(tfs_close(fd) != -1);

}
void copy_to_external(char *path){

    char *path2 = "external_file.txt";

    assert(tfs_copy_to_external_fs(path, path2) != -1);

    unlink(path2);

}

void trunc_ll(char *path){
    int fd = tfs_open(path, TFS_O_TRUNC);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
}


int main() {

    char *path = "/f1";

    pthread_t tid[THREADS];

    assert(tfs_init() != -1);

    write_ll(path);

    for(int i = 0; i<THREADS-2; i++){
        if(pthread_create(&tid[i], NULL, (void*)write_ll2, path) != 0)
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&tid[THREADS-2], NULL, (void*)copy_to_external, path) != 0)
        exit(EXIT_FAILURE);
    if(pthread_create(&tid[THREADS-1], NULL, (void*)trunc_ll, path) != 0)
        exit(EXIT_FAILURE);

    for(int i = 0; i<THREADS; i++){
        pthread_join(tid[i], NULL);
    }

    printf("Sucessful test\n");

    return 0;
}