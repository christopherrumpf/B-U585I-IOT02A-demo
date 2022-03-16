/* run this on a Raspberry Pi VM */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

uint64_t testdata(uint64_t i)
{
    i |= (i + 1) << 8;
    i |= (i + 0x202) << 16;
    i |= (i + 0x4040404) << 32;
    return i;
}

int main(void)
{
    int fd;
    unsigned i;

    fd = open("/dev/mem", O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "Failed to access /dev/mem.\n");
        return 1;
    }

    volatile uint8_t *mmio = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xFEF00000ul);
    if(mmio == (uint8_t *)MAP_FAILED) {
        fprintf(stderr, "Failed to mmap /dev/mem.\n");
        return 1;
    }

    for(i=0; i<16; i++)
        mmio[i] = testdata(i);
    for(i=0; i<16; i++)
        printf("%2d: 0x%02x\n", i, mmio[i]);

    for(i=0; i<16; i++)
        *(volatile uint16_t *)&mmio[i*2] = testdata(i);
    for(i=0; i<16; i++)
        printf("%2d: 0x%04x\n", i, *(volatile uint16_t *)&mmio[i*2]);

    for(i=0; i<16; i++)
        *(volatile uint32_t *)&mmio[i*4] = testdata(i);
    for(i=0; i<16; i++)
        printf("%2d: 0x%08x\n", i, *(volatile uint32_t *)&mmio[i*4]);

    for(i=0; i<16; i++)
        *(volatile uint64_t *)&mmio[i*8] = testdata(i);
    for(i=0; i<16; i++)
        printf("%2d: 0x%016lx\n", i, *(volatile uint64_t *)&mmio[i*8]);

    return 0;
}
