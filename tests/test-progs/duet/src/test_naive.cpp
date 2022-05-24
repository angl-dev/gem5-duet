#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <thread>
#include <memory>

void task (
        volatile uint64_t * ptr_softreg
        , volatile uint64_t * ptr_arg
        )
{
    *ptr_softreg = reinterpret_cast<uint64_t> (ptr_arg);
    while ( 0 == *ptr_softreg );
}

int main(int argc, char *argv[]) {

    int fd = open ("/dev/duet", O_RDWR);

    if (fd < 0) {
        fprintf ( stderr, "Failed to open /dev/duet. ERRNO = %d\n", errno );
        return -1;
    }

    // unsigned num_threads = std::thread::hardware_concurrency ();
    unsigned num_threads = 4;
    printf ( "HW concurrency = %u\n", std::thread::hardware_concurrency () );

    volatile uint64_t * vaddr = static_cast<uint64_t *> (
            mmap(NULL, num_threads * 8, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0) );

    if ( NULL == vaddr ) {
        fprintf ( stderr, "Mmap failed\n" );
        return -1;
    }

    auto data = std::make_unique<volatile uint64_t[]>(num_threads);
    printf ( "&data = 0x%p\n", data.get() );
    for ( size_t i = 0; i < num_threads; ++i ) {
        data[i] = (i + 1) * (i + 1);
    }

    auto threads = std::make_unique<std::thread[]>(num_threads);
    for ( size_t i = 0; i < num_threads; ++i ) {
        std::thread tmp ( task, vaddr + i, data.get() + i );
        std::swap ( threads[i], tmp );
    }

    for ( size_t i = 0; i < num_threads; ++i ) {
        threads[i].join();
    }
    
    bool mismatch = false;
    for ( size_t i = 0; i < num_threads; ++i ) {
        uint64_t ref = (i + 1) * (i + 1) + 1;
        if ( data[i] != ref ) {
            mismatch = true;
            fprintf ( stderr, "Mismatch: data[%d] = 0x%016llx != ref[%d] = 0x%016llx\n",
                    i, data[i], i, ref );
        }
    }

    if ( !mismatch ) {
        printf ( "Pass!\n" );
    }

    return 0;
}
