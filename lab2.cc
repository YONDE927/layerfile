#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <unistd.h>


#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)


static void usageError(char *progName)
{
    fprintf(stderr, "Usage: %s file\n", progName);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0)
        usageError(argv[0]);

    system("rm test");

    int fd = creat("test", 500);
    close(fd);

    int v = 550; 
    if (setxattr(argv[1], "user.x", &v, sizeof(v), 0) == -1)
        errExit("setxattr");

    char* value = "y attribute value";
    if (setxattr(argv[1], "user.y", value, strlen(value), 0) == -1)
        errExit("setxattr");

    int r;
    if (getxattr(argv[1], "user.x", &r, sizeof(r)) == -1)
        errExit("setxattr");
    printf("%d\n", r);
    exit(EXIT_SUCCESS);
}
