#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>

std::mutex                  m;
std::condition_variable     cv;
constexpr const unsigned    num_threads = 3;
constexpr const unsigned    total_work = 64 * num_threads;
unsigned                    cnt = 0;

void task (
        unsigned tid
        , volatile uint64_t * ptr_softreg
        , volatile uint64_t * pdata
        )
{
    {
        std::unique_lock lock ( m );

        if ( ++cnt == num_threads ) {
            lock.unlock ();
            cv.notify_all ();
        } else {
            cv.wait ( lock, []{ return cnt == num_threads; });
        }
    }

    for ( unsigned i = tid; i < total_work; i += num_threads )
        *ptr_softreg = reinterpret_cast<uint64_t> ( pdata + i );

    {
        std::unique_lock lock ( m );

        if ( --cnt == 0 ) {
            lock.unlock ();
            cv.notify_all ();
        } else {
            cv.wait ( lock, []{ return cnt == 0; });
        }
    }

    for ( unsigned i = tid; i < total_work; i += num_threads )
        while ( 0 == *ptr_softreg );
}

int main(int argc, char *argv[]) {

    int fd = open ("/dev/duet", O_RDWR);

    if (fd < 0) {
        fprintf ( stderr, "Failed to open /dev/duet. ERRNO = %d\n", errno );
        return -1;
    }

    // unsigned num_threads = std::thread::hardware_concurrency ();
    printf ( "HW concurrency = %u\n", std::thread::hardware_concurrency () );

    volatile uint64_t * vaddr = static_cast<uint64_t *> (
            mmap(NULL, num_threads * 8, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0) );

    if ( NULL == vaddr ) {
        fprintf ( stderr, "Mmap failed\n" );
        return -1;
    }

    auto data = std::make_unique<volatile uint64_t[]>(total_work);
    for ( unsigned i = 0; i < total_work; ++i ) {
        data[i] = (i + 1) * (i + 1);
    }

    auto threads = std::make_unique<std::thread[]>(num_threads);
    for ( unsigned i = 0; i < num_threads; ++i ) {
        std::thread tmp ( task, i, vaddr + i, data.get() );
        std::swap ( threads[i], tmp );
    }

    for ( unsigned i = 0; i < num_threads; ++i ) {
        threads[i].join();
    }
    
    bool mismatch = false;
    for ( unsigned i = 0; i < total_work; ++i ) {
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
