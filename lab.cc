#include <iostream>

#include <sys/xattr.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

const char* p = "test";

int main(){
    system("rm test");

    int fd = creat(p, S_IRUSR | S_IWUSR | S_IXUSR);
    write(fd, "hello ", 6);
    lseek(fd, 10, SEEK_SET);
    write(fd, "world\n", 6);
    close(fd);
}
