#include "operations.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            pthread_rwlock_wrlock(&inode->rwl);
            for (int i = 0; i < ceil((double)inode->i_size / BLOCK_SIZE); i++) {
                if (i < BLOCKS_PER_FILE){
                    if (data_block_free(inode->i_data_block[i]) == -1) {
                        pthread_rwlock_unlock(&inode->rwl);
                        return -1;
                    }
                } else {
                    void *block_sup = data_block_get(inode->i_sup_block);
                    if (data_block_free((*(int *)(block_sup + (unsigned long)(i-BLOCKS_PER_FILE) 
                            * sizeof(int)))) == -1) {
                        pthread_rwlock_unlock(&inode->rwl);
                        return -1;
                    }
                }
            }
            if(inode->i_size > BLOCK_SIZE * BLOCKS_PER_FILE){
                if (data_block_free(inode->i_sup_block) == -1) {
                    pthread_rwlock_unlock(&inode->rwl);
                    return -1;
                }
            }
            inode->i_size = 0;
            pthread_rwlock_unlock(&inode->rwl);
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            pthread_rwlock_rdlock(&inode->rwl);
            offset = inode->i_size;
            pthread_rwlock_unlock(&inode->rwl);
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}

int tfs_close(int fhandle) {
    return remove_from_open_file_table(fhandle);
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {

    size_t to_write2 = to_write;
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }
    pthread_rwlock_wrlock(&inode->rwl);

    if (to_write + file->of_offset >
        BLOCK_SIZE * (BLOCKS_PER_FILE + BLOCK_SIZE / sizeof(int))) {
        pthread_rwlock_unlock(&inode->rwl);
        return -1;
    }

    while ((int)to_write2 > 0) {
        if (inode->i_size % BLOCK_SIZE == 0 &&
            inode->i_size < BLOCK_SIZE * BLOCKS_PER_FILE) {
            /* If empty file, allocate new block */
            inode->i_data_block[inode->i_size / BLOCK_SIZE] = data_block_alloc();
            if (inode->i_data_block[inode->i_size / BLOCK_SIZE] == -1){
                pthread_rwlock_unlock(&inode->rwl);
                return -1;
            }
        } else if (inode->i_size % BLOCK_SIZE == 0) {
            if (inode->i_size == BLOCK_SIZE * BLOCKS_PER_FILE) {
                inode->i_sup_block = data_block_alloc();
                if (inode->i_sup_block == -1){
                    pthread_rwlock_unlock(&inode->rwl);
                    return -1;
                }
            }
            void *block_sup = data_block_get(inode->i_sup_block);
            if(block_sup == NULL){
                pthread_rwlock_unlock(&inode->rwl);
                return -1;
            }
            int block_aux = data_block_alloc();
            if(block_aux == -1){
                pthread_rwlock_unlock(&inode->rwl);
                return -1;
            }
            memcpy(block_sup + (inode->i_size / BLOCK_SIZE - BLOCKS_PER_FILE) * sizeof(int), 
                &block_aux, sizeof(int));
        }

        void *block;
        if (inode->i_size < BLOCK_SIZE * BLOCKS_PER_FILE) {
            block = data_block_get(inode->i_data_block[inode->i_size / BLOCK_SIZE]);
        } else {
            void *block_sup = data_block_get( inode->i_sup_block);
            block = data_block_get( *(int *)(block_sup + (inode->i_size / BLOCK_SIZE - BLOCKS_PER_FILE) 
                * sizeof(int)));
        }
        if (block == NULL){
            pthread_rwlock_unlock(&inode->rwl);
            return -1;
        }

        size_t to_write_aux;
        if (to_write2 + file->of_offset % BLOCK_SIZE > BLOCK_SIZE) {
            to_write_aux = BLOCK_SIZE - file->of_offset % BLOCK_SIZE;
        } else {
            to_write_aux = to_write2;
        }
        to_write2 -= to_write_aux;

        /* Perform the actual write */
        memcpy(block + file->of_offset % BLOCK_SIZE, buffer, to_write_aux);
        buffer += to_write_aux;

        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += to_write_aux;
        inode->i_size = file->of_offset;
    }
    pthread_rwlock_unlock(&inode->rwl);
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }
    pthread_rwlock_rdlock(&inode->rwl);

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;

    if (to_read > len) {
        to_read = len;
    }

    size_t to_read2 = to_read;

    while ((int)to_read2 > 0) {
        void *block;
        if (file->of_offset < BLOCK_SIZE * BLOCKS_PER_FILE) {
            block = data_block_get( inode->i_data_block[file->of_offset / BLOCK_SIZE]);
        } else {
            void *block_sup = data_block_get(inode->i_sup_block);
            if(block_sup == NULL){
                pthread_rwlock_unlock(&inode->rwl);
                return -1;
            }
            block = data_block_get(*(int *)(block_sup + (file->of_offset / BLOCK_SIZE - BLOCKS_PER_FILE) 
                * sizeof(int)));
        }
        if (block == NULL) {
            pthread_rwlock_unlock(&inode->rwl);
            return -1;
        }

        size_t to_read_aux = to_read2;
        if (BLOCK_SIZE - file->of_offset % BLOCK_SIZE < to_read_aux) {
            to_read_aux = BLOCK_SIZE - file->of_offset % BLOCK_SIZE;
        }

        /* Perform the actual read */
        memcpy(buffer, block + file->of_offset % BLOCK_SIZE, to_read_aux);
        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += to_read_aux;
        to_read2 -= to_read_aux;
    }

    pthread_rwlock_unlock(&inode->rwl);
    return (ssize_t)to_read;
}

int tfs_copy_to_external_fs(char const *source_path, char const *dest_path) {
    int inum = tfs_lookup(source_path);
    if (inum == -1) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(inum);
    if (inode == NULL) {
        return -1;
    }
    pthread_rwlock_rdlock(&inode->rwl);

    FILE *fp = fopen(dest_path, "w");
    if (fp == NULL) {
        pthread_rwlock_unlock(&inode->rwl);
        return -1;
    }

    size_t to_read = inode->i_size;
    size_t written = 0;
    while ((int)to_read > 0) {
        void *block;
        if (written < BLOCK_SIZE * BLOCKS_PER_FILE) {
            block = data_block_get(inode->i_data_block[written / BLOCK_SIZE]);
        } else {
            void *block_sup = data_block_get(inode->i_sup_block);
            block = data_block_get(*(int *)(block_sup + (written / BLOCK_SIZE - BLOCKS_PER_FILE) * sizeof(int)));
        }
        if (block == NULL) {
            pthread_rwlock_unlock(&inode->rwl);
            return -1;
        }

        size_t to_read_aux = to_read;
        if (BLOCK_SIZE - written % BLOCK_SIZE < to_read_aux) {
            to_read_aux = BLOCK_SIZE - written % BLOCK_SIZE;
        }

        /* Perform the actual read and write */
        fputs(block + written % BLOCK_SIZE, fp);
        written+= to_read_aux;

        to_read -= written;

    }
    if(fclose(fp) == -1){
        pthread_rwlock_unlock(&inode->rwl);
        return -1;
    }
    pthread_rwlock_unlock(&inode->rwl);
    return 0;
}