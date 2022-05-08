#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

int main(int argc, char *argv[]) {

    uint32_t din = 1, dout = 1;

    int fd = open ("/dev/duet", O_RDWR | O_DIRECT);
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

    return 0;
}
