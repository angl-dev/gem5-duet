#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

int main(int argc, char *argv[]) {

    uint32_t din = 1, dout = 1;

    int fd = open ("/dev/duet", O_RDWR);
    if (fd < 0) {
        fprintf ( stderr, "Failed to open /dev/duet. ERRNO = %d\n", errno );
        return -1;
    }

    // try read out the initial value
    if (ioctl(fd, 0, &dout)) {
        fprintf ( stderr, "First IOCTL read failed. ERRNO = %d\n", errno );
        return -1;
    } else if (dout != 0) {
        fprintf ( stderr, "First read returned 0x%08x\n", dout );
        return -1;
    } else {
        printf ( "First read success\n" );
    }

    // try set value
    if (ioctl(fd, 1, &din)) {
        fprintf ( stderr, "IOCTL write failed. ERRNO = %d\n", errno );
        return -1;
    } else {
        printf ( "Write success\n" );
    }

    // try read back
    if (ioctl(fd, 0, &dout)) {
        fprintf ( stderr, "Second IOCTL read failed. ERRNO = %d\n", errno );
        return -1;
    } else if (dout != din) {
        fprintf ( stderr, "Second read returned 0x%08x (0x%08x expected)\n", dout, din );
        return -1;
    } else {
        printf ( "Second read success\n" );
    }

    // try to be naughty
    if (!ioctl(fd, 2, NULL)) {
        fprintf ( stderr, "Naughty access is tolerated\n" );
        return -1;
    } else if ( errno != EINVAL ) {
        fprintf ( stderr, "Naughty access set ERRNO = %d\n", errno );
        return -1;
    } else {
        printf ( "Naughty access successfully rejected\n" );
    }

    // try mmap
    volatile uint8_t * vaddr = static_cast<uint8_t *> (
            mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0) );

    if ( NULL == vaddr ) {
        fprintf ( stderr, "Mmap failed\n" );
        return -1;
    }

    const uint8_t tv[2] = { 0x37, 0xac };
    for (int i = 0; i < 2; i++) {
        vaddr[128] = tv[i];

        // readback
        uint8_t tmp = vaddr[128];
        if ( tv[i] != tmp ) {
            fprintf ( stderr, "Read #%d.0 returned 0x%02x (0x%02x expected)\n",
                    i, tmp, tv[i] );
            return -1;
        }

        // persistency?
        tmp = vaddr[128];
        if ( tv[i] != tmp ) {
            fprintf ( stderr, "Read #%d.1 returned 0x%02x (0x%02x expected)\n",
                    i, tmp, tv[i] );
            return -1;
        }
    }

    return 0;
}
