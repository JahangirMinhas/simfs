#include <stdio.h>
#include "simfstypes.h"

/* File system operations */
void printfs(char *);
void initfs(char *);
void createfile(char *, char *);
void writefile(char *filename, char *fname, int start, int length);
void readfile(char *filename, char *fname, int start, int length);
void deletefile(char *filename, char *fname);

/* Internal functions */
FILE *openfs(char *filename, char *mode);
void closefs(FILE *fp);
