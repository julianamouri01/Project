#include <stdio.h>
#include "simfstypes.h"

void printfs(char *);
void initfs(char *);

FILE *openfs(char *filename, char *mode);
void closefs(FILE *fp);

void read_fentries(FILE *fp, fentry *files);
void write_fentries(FILE *fp, fentry *files);
void read_fnodes(FILE *fp, fnode *fnodes);
void write_fnodes(FILE *fp, fnode *fnodes);

long data_start_offset(void);
long block_offset(int block_num);
void read_block(FILE *fp, int block_num, char *buf);
void write_block(FILE *fp, int block_num, const char *buf);

void createfile(char *fsname, char *filename);
void deletefile(char *fsname, char *filename);
void readfile(char *fsname, char *filename, int start, int length);
void writefile(char *fsname, char *filename, int start, int length);
