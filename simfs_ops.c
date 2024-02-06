/* This file contains functions that are not part of the visible interface.
 * So they are essentially helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "simfs.h"

/* Internal helper functions first.
 */

struct fArrays {
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
};

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if((fp = fopen(filename, mode)) == NULL) {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void
closefs(FILE *fp)
{
    if(fclose(fp) != 0) {
        perror("closefs");
        exit(1);
    }
}

struct fArrays readData(char *filename){
    FILE *fp;
    struct fArrays data;

    fp = openfs(filename, "r");
    if ((fread(data.files, sizeof(fentry), MAXFILES, fp)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    if ((fread(data.fnodes, sizeof(fnode), MAXBLOCKS, fp)) == 0) {
        fprintf(stderr, "Error: could not read fnodes\n");
        closefs(fp);
        exit(1);
    }
    closefs(fp);

    return data;
}

void writeData(char *filename, struct fArrays data){
    FILE *fp;
    fp = openfs(filename, "r+");
    if(fwrite(data.files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: write failed on init\n");
        closefs(fp);
        exit(1);
    }

    if(fwrite(data.fnodes, sizeof(fnode), MAXBLOCKS, fp) < MAXBLOCKS) {
        fprintf(stderr, "Error: write failed on init\n");
        closefs(fp);
        exit(1);
    }
    closefs(fp);
}

/* File system operations: creating, deleting, reading from, and writing to files.
 */

void createfile(char *filename, char *fname){
    if(strlen(fname) >= 12){
        fprintf(stderr, "invalid name\n");
        exit(1);
    }

    int i;
    struct fArrays data;
    data = readData(filename);

    int file_found = 0;
    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            file_found = 1;
        }
    }
    if(file_found == 1){
        fprintf(stderr, "file already exists\n");
        exit(1);
    }

    for(i = 0; i < MAXFILES; i++) {
        if(data.files[i].name[0] == '\0'){
            strncpy(data.files[i].name, fname, 11);
            data.files[i].name[strlen(data.files[i].name)] = '\0';
            break;
        }
    }
    if(i == MAXFILES){
        fprintf(stderr, "Error: not enough resources\n");
        exit(1);
    }

    writeData(filename, data);    
}

int findEmptyBlock(struct fArrays data){
    int nextBlock = -1;
    int foundBlock = 0;
    for(int i = 0; i < MAXBLOCKS; i++){
        if(data.fnodes[i].blockindex < 0){
            nextBlock = data.fnodes[i].blockindex;
            foundBlock = 1;
            break;
        }
    }
    if(foundBlock == 0){
        fprintf(stderr, "Error: no available blocks\n");
        exit(1);
    }
    return nextBlock;
}

void writefile(char *filename, char *fname, int start, int length){
    struct fArrays data;
    data = readData(filename);
    char zerobuf[BLOCKSIZE] = {0};
    int newBlock;
    int blocksReq = 0;
    int blockindex[MAXBLOCKS];
    int j = 0;
    int lengthcpy = length - start;
    int size;
    int at;
    int file_found = 0;
    int empty_file = 0;

    char buffer[length + 1];
    buffer[0] = '\0';
    if(fgets(buffer, length + 1, stdin) == NULL){
        fprintf(stderr, "Error: input failed\n");
        exit(1);
    }

    if(buffer[strlen(buffer) - 1] == 10){
        buffer[strlen(buffer) - 1] = '\0';
    }

    if(strlen(buffer) < length){
        fprintf(stderr, "Error: please input length characters\n");
        exit(1);
    }

    
    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            file_found = 1;
        }
    }
    if(file_found == 0){
        fprintf(stderr, "Error: file does not exist\n");
        exit(1);
    }

    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            size = data.files[i].size;
            if(start > size){
                fprintf(stderr, "Error: incorrect start and length combination\n");
                exit(1);
            }
            if(!(start < data.files[i].size && length <= (data.files[i].size - start))){
                data.files[i].size = start + length;
                size = data.files[i].size;
            }
            if(data.files[i].firstblock == -1){
                empty_file = 1;
                newBlock = findEmptyBlock(data);
                data.files[i].firstblock = abs(newBlock);
                blockindex[j] = data.files[i].firstblock;
                j++;
            }else{
                at = data.files[i].firstblock;
                lengthcpy = size;
                lengthcpy -= BLOCKSIZE;
                blockindex[j] = data.fnodes[at].blockindex;
                j++;
                newBlock = data.fnodes[at].blockindex * -1;

                while(data.fnodes[at].nextblock != -1){
                    at = data.fnodes[at].nextblock;
                    lengthcpy -= BLOCKSIZE;
                    blockindex[j] = data.fnodes[at].blockindex;
                    j++;
                }
            }
            break;
        }
    }

    while(lengthcpy > 0){
        blocksReq++;
        lengthcpy -= BLOCKSIZE;
    }
    
    if(blocksReq >= 1 && empty_file == 0){
        newBlock = findEmptyBlock(data);
        data.fnodes[at].nextblock = abs(newBlock);
        blockindex[j] = abs(newBlock);
        j++;
    }

    for(int i = 0; i < MAXBLOCKS; i++){
        if(data.fnodes[i].blockindex <= newBlock){
            if(abs(data.fnodes[i].blockindex) == (blocksReq + abs(newBlock) - 1)){
                data.fnodes[i].blockindex = abs(data.fnodes[i].blockindex);
                data.fnodes[i].nextblock = -1;
                blockindex[j] = data.fnodes[i].nextblock;
                break;
            }else if(data.fnodes[i].blockindex > (newBlock - blocksReq)){
                data.fnodes[i].blockindex = abs(data.fnodes[i].blockindex);
                data.fnodes[i].nextblock = abs(findEmptyBlock(data));
                blockindex[j] = abs(data.fnodes[i].nextblock);
                j++;
            }
        }
    }
    
    FILE *fp;
    fp = openfs(filename, "r+");
    writeData(filename, data);
    int sizes[MAXBLOCKS];
    sizes[0] = 0;
    int numbersizes = 0;
    int startcpy = start;

    while(startcpy > 0){
        if(startcpy > BLOCKSIZE){
            sizes[numbersizes] = BLOCKSIZE;
        }else{
            sizes[numbersizes] = startcpy;
        }
        startcpy -= BLOCKSIZE;
        numbersizes++;
    }
    sizes[numbersizes] = 0;
    numbersizes++;
    sizes[numbersizes] = -1;

    int lengths[MAXBLOCKS];
    lengths[0] = 0;
    int numberlengths = 0;
    lengthcpy = length;
    int a = 0;
    int skip = 0;

    while(lengthcpy > 0){
        if(sizes[a] == -1){
            skip = 0;
        }else{
            skip = sizes[a];
            a++;
        }
        if(lengthcpy > (BLOCKSIZE - skip)){
            lengths[numberlengths] = (BLOCKSIZE - skip);
            lengthcpy -= (BLOCKSIZE - skip);
        }else if(lengthcpy > BLOCKSIZE){
            lengths[numberlengths] = BLOCKSIZE;
            lengthcpy -= BLOCKSIZE;
        }else{
            lengths[numberlengths] = lengthcpy;
            lengthcpy -= BLOCKSIZE;
        }
        numberlengths++;
    }
    lengths[numberlengths] = -1;

    int i = 0;
    a = 0;
    skip = 0;
    char *buf_ptr = buffer;
    while(lengths[i] != -1){
        if(sizes[a] == -1){
            skip = 0;
        }else{
            skip = sizes[a];
            a++;
        }
        if(fseek(fp, (blockindex[i] * BLOCKSIZE) + skip, SEEK_SET) != 0){
            fprintf(stderr, "Error: cant move file pointer1\n");
            closefs(fp);
            exit(1);
        }
        char string_to_write[BLOCKSIZE];
        memset(string_to_write, 0, BLOCKSIZE);
        if(i == 0){
            strncpy(string_to_write, buf_ptr, lengths[i]);
        }else{
            strncpy(string_to_write, &buf_ptr[lengths[i - 1]], lengths[i]);
        }
        if(fwrite(string_to_write, 1, lengths[i], fp) < lengths[i]){
            fprintf(stderr, "Error: cant write data\n");
            closefs(fp);
            exit(1);
        };
        i++;
    }

    if(fseek(fp, 0, SEEK_END) != 0){
        fprintf(stderr, "Error: cant move file pointer2\n");
        closefs(fp);
        exit(1);
    }
    int bytes_to_write = (BLOCKSIZE - ((size % BLOCKSIZE) % BLOCKSIZE)) % BLOCKSIZE;
    if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, fp) < 1) {
        fprintf(stderr, "Error: write failed on init\n");
        closefs(fp);
        exit(1);
    }

    closefs(fp);
}

void readfile(char *filename, char *fname, int start, int length){
    struct fArrays data;
    data = readData(filename);
    int blockindex[MAXBLOCKS];
    int j = 0;
    int at;
    int lengthcpy = length - start;
    // int size;
    int file_found = 0;

    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            file_found = 1;
        }
    }
    if(file_found == 0){
        fprintf(stderr, "Error: file does not exist\n");
        exit(1);
    }

    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            if(data.files[i].firstblock == -1){
                fprintf(stderr, "file is empty\n");
                exit(1);
            }else{
                // size = data.files[i].size;
                at = data.files[i].firstblock;
                blockindex[j] = data.fnodes[at].blockindex;
                j++;
                while(data.fnodes[at].nextblock != -1){
                    at = data.fnodes[at].nextblock;
                    blockindex[j] = data.fnodes[at].blockindex;
                    j++;
                    
                }
                blockindex[j] = -1;
                j++;
            }
        }
    }

    int sizes[MAXBLOCKS];
    sizes[0] = 0;
    int numbersizes = 0;
    int startcpy = start;

    while(startcpy > 0){
        if(startcpy > BLOCKSIZE){
            sizes[numbersizes] = BLOCKSIZE;
        }else{
            sizes[numbersizes] = startcpy;
        }
        startcpy -= BLOCKSIZE;
        numbersizes++;
    }
    sizes[numbersizes] = 0;
    numbersizes++;
    sizes[numbersizes] = -1;

    int lengths[MAXBLOCKS];
    lengths[0] = 0;
    int numberlengths = 0;
    lengthcpy = length;
    int a = 0;
    int skip = 0;

    while(lengthcpy > 0){
        if(sizes[a] == -1){
            skip = 0;
        }else{
            skip = sizes[a];
            a++;
        }
        if(lengthcpy > (BLOCKSIZE - skip)){
            lengths[numberlengths] = (BLOCKSIZE - skip);
            lengthcpy -= (BLOCKSIZE - skip);
        }else if(lengthcpy > BLOCKSIZE){
            lengths[numberlengths] = BLOCKSIZE;
            lengthcpy -= BLOCKSIZE;
        }else{
            lengths[numberlengths] = lengthcpy;
            lengthcpy -= BLOCKSIZE;
        }
        numberlengths++;
    }
    lengths[numberlengths] = -1;

    FILE *fp = openfs(filename, "r");
    a = 0;
    skip = 0;
    int i = 0;
    while(lengths[i] != -1){
        if(sizes[a] == -1){
            skip = 0;
        }else{
            skip = sizes[a];
            a++;
        }
        if(fseek(fp, (blockindex[i] * BLOCKSIZE) + skip, SEEK_SET) != 0){
            fprintf(stderr, "Error: cant move file pointer\n");
            closefs(fp);
            exit(1);
        }
        char string_read[BLOCKSIZE + 1];
        memset(string_read, 0, BLOCKSIZE);
        fread(string_read, 1, lengths[i], fp);
        if(fwrite(string_read, 1, lengths[i], stdout) < lengths[i]){
            fprintf(stderr, "Error: cant write data\n");
            closefs(fp);
            exit(1);
        }
        i++;
    }
    printf("\n");
    closefs(fp);
}

void deletefile(char *filename, char *fname){
    struct fArrays data;
    data = readData(filename);
    int blockindex[MAXBLOCKS];
    int j = 0;
    int at;
    char zerobuf[BLOCKSIZE] = {0};
    // int size;
    int file_found = 0;

    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            file_found = 1;
        }
    }
    if(file_found == 0){
        fprintf(stderr, "Error: file does not exist\n");
        exit(1);
    }

    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            if(data.files[i].firstblock == -1){
                fprintf(stderr, "file is empty\n");
                exit(1);
            }else{
                at = data.files[i].firstblock;
                blockindex[j] = data.fnodes[at].blockindex;
                j++;
                while(data.fnodes[at].nextblock != -1){
                    at = data.fnodes[at].nextblock;
                    blockindex[j] = data.fnodes[at].blockindex;
                    j++;
                    
                }
                blockindex[j] = -1;
                j++;
            }
        }
    }

    for(int i = 0; i < MAXFILES; i++) {
        if(strcmp(data.files[i].name, fname) == 0){
            data.files[i].name[0] = '\0';
            data.files[i].firstblock = -1;
            data.files[i].size = 0;
        }
    }

    int i = 0;
    while(blockindex[i] != -1){
        for(int num = 0; num < MAXBLOCKS; num++) {
            if(data.fnodes[num].blockindex == blockindex[i]){
                data.fnodes[num].blockindex = data.fnodes[num].blockindex * -1;
                data.fnodes[num].nextblock = -1;
            }
        }
        i++;
    }
    writeData(filename, data);
    i = 0;
    FILE *fp = openfs(filename, "r+");
    while(blockindex[i] != -1){
        fseek(fp, blockindex[i] * BLOCKSIZE, SEEK_SET);
        if (fwrite(zerobuf, BLOCKSIZE, 1, fp) < 1) {
            fprintf(stderr, "Error: write failed on init\n");
            closefs(fp);
            exit(1);
        }
        i++;
    }
    closefs(fp);
}