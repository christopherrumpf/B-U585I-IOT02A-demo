#include <stdio.h>
#include <string.h>

#include "coreio.h"

static uint8_t memory[256];

static int test_read(void *priv, uint64_t addr, size_t len, void *buf, unsigned flags)
{
    printf("read  %08x %d %x\n", (unsigned)addr, (unsigned)len, flags);

    addr &= 0xFFF;
    if(addr + len < sizeof(memory))
        memcpy(buf, &memory[addr], len);
    return 0;
}

static int test_write(void *priv, uint64_t addr, size_t len, void *buf, unsigned flags)
{
    printf("write %08x %d %016llx %x\n", (unsigned)addr, (unsigned)len, *(unsigned long long *)buf, flags);

    addr &= 0xFFF;
    if(addr + len < sizeof(memory))
        memcpy(&memory[addr], buf, len);
    return 0;
}

static const coreio_func_t test_func = {
    .read = test_read,
    .write = test_write,
};

int main(int argc, char *argv[])
{
    if(argc != 2) {
        fprintf(stderr, "usage: vmmio-test-outside <host:port>\n");
        return 1;
    }

    if(coreio_connect(argv[1]))
        return 1;

    coreio_register(0, &test_func, NULL);

    while(!coreio_mainloop(1000000)) {
        printf("tick\n");
        fflush(stdout);
    }

    coreio_disconnect();

    return 0;
}
