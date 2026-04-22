#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

#define MAXOPS  7
char *ops[MAXOPS] = {"initfs", "printfs", "createfile", "readfile",
                     "writefile", "deletefile", "info"};
int find_command(char *);

int
main(int argc, char **argv)
{
    int   oc;
    char *cmd;
    char *fsname;

    char *usage_string = "Usage: simfs -f file cmd arg1 arg2 ...\n";

    if (argc < 4) {
        fputs(usage_string, stderr);
        exit(1);
    }

    while ((oc = getopt(argc, argv, "f:")) != -1) {
        switch (oc) {
        case 'f':
            fsname = optarg;
            break;
        default:
            fputs(usage_string, stderr);
            exit(1);
        }
    }

    cmd = argv[optind];
    optind++;

    switch (find_command(cmd)) {
    case 0: /* initfs */
        initfs(fsname);
        break;
    case 1: /* printfs */
        printfs(fsname);
        break;
    case 2: /* createfile */
        if (optind >= argc) {
            fprintf(stderr, "Usage: simfs -f file createfile <name>\n");
            exit(1);
        }
        createfile(fsname, argv[optind]);
        break;
    case 3: /* readfile */
        if (optind + 2 >= argc) {
            fprintf(stderr, "Usage: simfs -f file readfile <name> <start> <length>\n");
            exit(1);
        }
        readfile(fsname, argv[optind],
                 atoi(argv[optind + 1]),
                 atoi(argv[optind + 2]));
        break;
    case 4: /* writefile */
        if (optind + 2 >= argc) {
            fprintf(stderr, "Usage: simfs -f file writefile <name> <start> <length>\n");
            exit(1);
        }
        writefile(fsname, argv[optind],
                  atoi(argv[optind + 1]),
                  atoi(argv[optind + 2]));
        break;
    case 5: /* deletefile */
        if (optind >= argc) {
            fprintf(stderr, "Usage: simfs -f file deletefile <name>\n");
            exit(1);
        }
        deletefile(fsname, argv[optind]);
        break;
    default:
        fprintf(stderr, "Error: Invalid command\n");
        exit(1);
    }

    return 0;
}

int
find_command(char *cmd)
{
    int i;
    for (i = 0; i < MAXOPS; i++) {
        if ((strncmp(cmd, ops[i], strlen(ops[i]))) == 0)
            return i;
    }
    fprintf(stderr, "Error: Command %s not found\n", cmd);
    return -1;
}
