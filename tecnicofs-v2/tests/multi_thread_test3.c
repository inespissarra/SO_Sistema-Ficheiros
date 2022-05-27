#include "../fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 2000

#define THREADS 20

void write_ll(char *path){

    char *input = "AAA! AAA! AAA! ";

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_write(fd, input, strlen(input)) == strlen(input));

    assert(tfs_close(fd) != -1);

}

void write_ll2(char *path){

    char *input = "ABCDEFG";

    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);

    assert(tfs_write(fd, input, strlen(input)) == strlen(input));

    assert(tfs_close(fd) != -1);

}

void read_ll(char *path){

    char *input = "AAA! AAA! AAA! ";

    char output[strlen(input)];

    int fd = tfs_open(path, 0);
    assert(fd != -1 );

    assert(tfs_read(fd, output, strlen(input)) == strlen(input));

    assert (memcmp(input, output, strlen(input)) == 0);

    assert(tfs_close(fd) != -1);
}

void read_ll2(char *path){

    char *input = "ABCDEFG";

    char output[strlen(input)];

    int fd = tfs_open(path, 0);
    assert(fd != -1 );

    assert(tfs_read(fd, output, strlen(input)) == strlen(input));

    assert (memcmp(input, output, strlen(input)) == 0);

    assert(tfs_close(fd) != -1);
}

void copy_to_external(char *path){

    char *path2 = "external_file.txt";

    char *input = "AAA! AAA! AAA! ";
    char buffer[strlen(input)];

    assert(tfs_copy_to_external_fs(path, path2) != -1);

    FILE *fp = fopen(path2, "r");

    assert(fp != NULL);

    assert(fread(buffer, sizeof(char), strlen(input), fp) == strlen(input));
    
    assert (memcmp(input, buffer, strlen(input)) == 0);

    assert(fclose(fp) != -1);

    unlink(path2);

}

void copy_to_external2(char *path){

    char *path2 = "external_file2.txt";

    char *input = "ABCDEFG";
    char buffer[strlen(input)];

    assert(tfs_copy_to_external_fs(path, path2) != -1);

    FILE *fp = fopen(path2, "r");

    assert(fp != NULL);

    assert(fread(buffer, sizeof(char), strlen(input), fp) == strlen(input));
    
    assert (memcmp(input, buffer, strlen(input)) == 0);

    assert(fclose(fp) != -1);

    unlink(path2);

}


int main() {

    char *path = "/f1";
    char *path2 = "/f2";

    pthread_t tid[THREADS];

    assert(tfs_init() != -1);

    write_ll(path);

    write_ll2(path2);

    for(int i=0; i<THREADS/2;i++){
        if(pthread_create(&tid[i], NULL, (void*)read_ll, path) != 0)
            exit(EXIT_FAILURE);
    }
    if(pthread_create(&tid[THREADS/2], NULL, (void*)copy_to_external, path) != 0)
            exit(EXIT_FAILURE);

    for(int i=THREADS/2+1; i<THREADS-1;i++){
        if(pthread_create(&tid[i], NULL, (void*)read_ll2, path2) != 0)
            exit(EXIT_FAILURE);
    }
    if(pthread_create(&tid[THREADS-1], NULL, (void*)copy_to_external2, path2) != 0)
            exit(EXIT_FAILURE);

    for(int i = 0; i<THREADS; i++){
        pthread_join(tid[i], NULL);
    }

    printf("Sucessful test\n");

    return 0;
}