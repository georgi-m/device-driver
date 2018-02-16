#include <iostream>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned long long u64;

struct cma_alloc
{
        u64 virt_start_addr;
        u64 phys_start_addr;
        u64 buffer_size;
        bool log_control;
};

#define IOCTL_CMA_ALLOC _IOWR('A', 1, struct cma_alloc)
#define IOCTL_CMA_RELEASE _IO('A', 2)

int main()
{
        cma_alloc cma;
        cma.log_control = false;
        int fd = open("/dev/cma_module", O_RDWR);
        if (-1 == fd) {
                std::cout<<"Failed to open cma_module device file."<<std::endl;
        }
        std::cout << "before ioclt call: Virtual address: " <<cma.virt_start_addr<< std::endl;
        //cma.buffer_size = 131072;
        cma.buffer_size = 4096;
        ioctl(fd, IOCTL_CMA_ALLOC, &cma);
        std::cout << "after ioctl call Virtual address: " <<std::hex<<cma.virt_start_addr<< std::endl;
        std::cout << "after ioctl call Physical address: " <<std::hex<<cma.phys_start_addr<< std::endl;
        ioctl(fd, IOCTL_CMA_RELEASE);
        close(fd);
        return 0;
}
