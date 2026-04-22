#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if ((fp = fopen(filename, mode)) == NULL) {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void
closefs(FILE *fp)
{
    if (fclose(fp) != 0) {
        perror("closefs");
        exit(1);
    }
}

void
read_fentries(FILE *fp, fentry *files)
{
    rewind(fp);
    if (fread(files, sizeof(fentry), MAXFILES, fp) < (size_t)MAXFILES) {
        fprintf(stderr, "Error: could not read fentries\n");
        exit(1);
    }
}

void
write_fentries(FILE *fp, fentry *files)
{
    rewind(fp);
    if (fwrite(files, sizeof(fentry), MAXFILES, fp) < (size_t)MAXFILES) {
        fprintf(stderr, "Error: could not write fentries\n");
        exit(1);
    }
}

void
read_fnodes(FILE *fp, fnode *fnodes)
{
    fseek(fp, sizeof(fentry) * MAXFILES, SEEK_SET);
    if (fread(fnodes, sizeof(fnode), MAXBLOCKS, fp) < (size_t)MAXBLOCKS) {
        fprintf(stderr, "Error: could not read fnodes\n");
        exit(1);
    }
}

void
write_fnodes(FILE *fp, fnode *fnodes)
{
    fseek(fp, sizeof(fentry) * MAXFILES, SEEK_SET);
    if (fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) < (size_t)MAXBLOCKS) {
        fprintf(stderr, "Error: could not write fnodes\n");
        exit(1);
    }
}

long
data_start_offset(void)
{
    int bytes_used = sizeof(fentry) * MAXFILES + sizeof(fnode) * MAXBLOCKS;
    int rem = bytes_used % BLOCKSIZE;
    if (rem == 0)
        return (long)bytes_used;
    return (long)(bytes_used + (BLOCKSIZE - rem));
}

long
block_offset(int block_num)
{
    return data_start_offset() + (long)block_num * BLOCKSIZE;
}

void
read_block(FILE *fp, int block_num, char *buf)
{
    fseek(fp, block_offset(block_num), SEEK_SET);
    if (fread(buf, 1, BLOCKSIZE, fp) < (size_t)BLOCKSIZE) {
        fprintf(stderr, "Error: could not read block %d\n", block_num);
        exit(1);
    }
}

void
write_block(FILE *fp, int block_num, const char *buf)
{
    fseek(fp, block_offset(block_num), SEEK_SET);
    if (fwrite(buf, 1, BLOCKSIZE, fp) < (size_t)BLOCKSIZE) {
        fprintf(stderr, "Error: could not write block %d\n", block_num);
        exit(1);
    }
}

void
createfile(char *fsname, char *filename)
{
    if (strlen(filename) > 11) {
        fprintf(stderr, "Error: createfile: name too long\n");
        exit(1);
    }

    FILE *fp = openfs(fsname, "r+b");
    fentry files[MAXFILES];
    read_fentries(fp, files);

    int slot = -1;
    for (int i = 0; i < MAXFILES; i++) {
        if (files[i].name[0] != '\0') {
            if (strncmp(files[i].name, filename, 12) == 0) {
                fprintf(stderr, "Error: createfile: %s already exists\n", filename);
                closefs(fp);
                exit(1);
            }
        } else if (slot == -1) {
            slot = i;
        }
    }

    if (slot == -1) {
        fprintf(stderr, "Error: createfile: no space left in directory\n");
        closefs(fp);
        exit(1);
    }

    strncpy(files[slot].name, filename, 12);
    files[slot].size = 0;
    files[slot].firstblock = -1;

    write_fentries(fp, files);
    closefs(fp);
}

void
deletefile(char *fsname, char *filename)
{
    FILE *fp = openfs(fsname, "r+b");
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    read_fentries(fp, files);
    read_fnodes(fp, fnodes);

    int slot = -1;
    for (int i = 0; i < MAXFILES; i++) {
        if (files[i].name[0] != '\0' && strncmp(files[i].name, filename, 12) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        fprintf(stderr, "Error: deletefile: %s not found\n", filename);
        closefs(fp);
        exit(1);
    }

    char zeros[BLOCKSIZE];
    memset(zeros, 0, BLOCKSIZE);

    short cur = files[slot].firstblock;
    while (cur != -1) {
        short next = fnodes[cur].nextblock;
        write_block(fp, cur, zeros);
        fnodes[cur].blockindex = -cur;
        fnodes[cur].nextblock = -1;
        cur = next;
    }

    memset(&files[slot], 0, sizeof(fentry));
    files[slot].firstblock = -1;

    write_fnodes(fp, fnodes);
    write_fentries(fp, files);
    closefs(fp);
}

void
readfile(char *fsname, char *filename, int start, int length)
{
    FILE *fp = openfs(fsname, "rb");
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    read_fentries(fp, files);
    read_fnodes(fp, fnodes);

    int slot = -1;
    for (int i = 0; i < MAXFILES; i++) {
        if (files[i].name[0] != '\0' && strncmp(files[i].name, filename, 12) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        fprintf(stderr, "Error: readfile: %s not found\n", filename);
        closefs(fp);
        exit(1);
    }

    if (start > files[slot].size) {
        fprintf(stderr, "Error: readfile: offset %d is past end of file\n", start);
        closefs(fp);
        exit(1);
    }

    int remaining = length;
    int file_pos = 0;
    short cur = files[slot].firstblock;
    char buf[BLOCKSIZE];

    while (remaining > 0 && cur != -1) {
        if (file_pos + BLOCKSIZE > start) {
            read_block(fp, cur, buf);
            int bstart = (start > file_pos) ? (start - file_pos) : 0;
            int avail = BLOCKSIZE - bstart;
            int n = (avail < remaining) ? avail : remaining;
            fwrite(buf + bstart, 1, n, stdout);
            remaining -= n;
        }
        file_pos += BLOCKSIZE;
        cur = fnodes[cur].nextblock;
    }

    closefs(fp);
}

void
writefile(char *fsname, char *filename, int start, int length)
{
    FILE *fp = openfs(fsname, "r+b");
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    read_fentries(fp, files);
    read_fnodes(fp, fnodes);

    int slot = -1;
    for (int i = 0; i < MAXFILES; i++) {
        if (files[i].name[0] != '\0' && strncmp(files[i].name, filename, 12) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        fprintf(stderr, "Error: writefile: %s not found\n", filename);
        closefs(fp);
        exit(1);
    }

    if (start > files[slot].size) {
        fprintf(stderr, "Error: writefile: offset %d is past end of file\n", start);
        closefs(fp);
        exit(1);
    }

    int new_size = (start + length > files[slot].size) ? start + length : files[slot].size;
    int blocks_needed = (new_size + BLOCKSIZE - 1) / BLOCKSIZE;

    int blocks_have = 0;
    short tmp = files[slot].firstblock;
    while (tmp != -1) {
        blocks_have++;
        tmp = fnodes[tmp].nextblock;
    }

    int to_alloc = blocks_needed - blocks_have;
    if (to_alloc < 0) to_alloc = 0;

    if (to_alloc > 0) {
        int free_count = 0;
        for (int i = 1; i < MAXBLOCKS; i++) {
            if (fnodes[i].blockindex < 0)
                free_count++;
        }
        if (free_count < to_alloc) {
            fprintf(stderr, "Error: writefile: not enough free blocks\n");
            closefs(fp);
            exit(1);
        }
    }

    char *inbuf = malloc(length);
    if (!inbuf) {
        fprintf(stderr, "Error: writefile: malloc failed\n");
        closefs(fp);
        exit(1);
    }
    if (fread(inbuf, 1, length, stdin) < (size_t)length) {
        fprintf(stderr, "Error: writefile: couldn't read input\n");
        free(inbuf);
        closefs(fp);
        exit(1);
    }

    char zeros[BLOCKSIZE];
    memset(zeros, 0, BLOCKSIZE);

    short tail = -1;
    short cur = files[slot].firstblock;
    while (cur != -1 && fnodes[cur].nextblock != -1)
        cur = fnodes[cur].nextblock;
    tail = cur;

    for (int b = 0; b < to_alloc; b++) {
        int fi = -1;
        for (int i = 1; i < MAXBLOCKS; i++) {
            if (fnodes[i].blockindex < 0) {
                fi = i;
                break;
            }
        }
        fnodes[fi].blockindex = fi;
        fnodes[fi].nextblock = -1;
        write_block(fp, fi, zeros);

        if (tail == -1)
            files[slot].firstblock = (short)fi;
        else
            fnodes[tail].nextblock = (short)fi;
        tail = (short)fi;
    }

    int file_pos = 0;
    cur = files[slot].firstblock;
    char blockbuf[BLOCKSIZE];

    while (cur != -1) {
        if (file_pos + BLOCKSIZE > start && file_pos < start + length) {
            read_block(fp, cur, blockbuf);
            int bstart = (start > file_pos) ? (start - file_pos) : 0;
            int src_skip = (file_pos > start) ? (file_pos - start) : 0;
            int avail_block = BLOCKSIZE - bstart;
            int avail_src = length - src_skip;
            int n = (avail_block < avail_src) ? avail_block : avail_src;
            memcpy(blockbuf + bstart, inbuf + src_skip, n);
            write_block(fp, cur, blockbuf);
        }
        file_pos += BLOCKSIZE;
        cur = fnodes[cur].nextblock;
    }

    files[slot].size = (unsigned short)new_size;
    write_fnodes(fp, fnodes);
    write_fentries(fp, files);
    free(inbuf);
    closefs(fp);
}
