#include <linux/ioctl.h>

typedef unsigned long long u64;

struct cma_alloc
{
        u64 virt_start_addr;
        u64 phys_start_addr;
        u64 buffer_size;
};

#define IOCTL_CMA_ALLOC _IOWR('A', 1, struct cma_alloc)
#define IOCTL_CMA_RELEASE _IO('A', 2)

