#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

size_t virtual_to_physical(uint64_t pid, size_t addr)
{
    char path[1024];
    snprintf(path, sizeof (path), "/proc/%d/pagemap", pid);
    int fd = open(path, O_RDONLY);
    if(fd < 0)
    {   
        printf("open '/proc/self/pagemap' failed!\n");
        return 0;
    }   
    size_t pagesize = getpagesize();
    size_t offset = (addr / pagesize) * sizeof(uint64_t);
    if(lseek(fd, offset, SEEK_SET) < 0)
    {   
        printf("lseek() failed!\n");
        close(fd);
        return 0;
    }   
    uint64_t info;
    if(read(fd, &info, sizeof(uint64_t)) != sizeof(uint64_t))
    {   
        printf("read() failed!\n");
        close(fd);
        return 0;
    }   
    if((info & (((uint64_t)1) << 63)) == 0)
    {   
        printf("page is not present!\n");
        close(fd);
        return 0;
    }   
    size_t frame = info & ((((uint64_t)1) << 55) - 1); 
    size_t phy = frame * pagesize + addr % pagesize;
    close(fd);
    return phy;
}

void main(int argc, char *argv[]) {
    uint64_t vaddr, pid;
    if (argc != 3) {
       printf("usage: <pid> <vaddr>\n");
       return;
    }
    pid = strtoul(argv[1], NULL, 0);
    vaddr = strtoul(argv[2], NULL, 16);
    printf("pid %d vaddr %p to pyh %p\n", pid, vaddr, virtual_to_physical(pid, vaddr));
    return;
}
