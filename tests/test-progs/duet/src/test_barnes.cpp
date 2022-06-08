#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <thread>
#include <memory>
#include <random>
#include <math.h>

struct node_t {
    unsigned char   _[8];
    double          mass;
    double          pos[3];
} __attribute__ (( aligned (16) ));

void ref (
        const double        pos0[3]
        , const double      epssq
        , const node_t    * nodes
        , const int         n
        , double &          phi
        , double            acc[3]
        )
{
    phi = acc[0] = acc[1] = acc[2] = 0.f;

    for ( int i = 0; i < n; ++i ) {
        const node_t & node = nodes[i];

        double dr[3] = {};
        double drsq = epssq;
        for ( int d = 0; d < 3; ++d ) {
            dr[d] = node.pos[d] - pos0[d];
            drsq += dr[d] * dr[d];
        }

        double drabs = sqrt ( drsq );
        double phii = node.mass / drabs;
        double mor3 = phii / drsq;

        for ( int d = 0; d < 3; ++d ) {
            acc[d] += dr[d] * mor3;
        }

        phi += phii;
    }
}

int main ( int argc, char * argv[] ) {
    int fd = open ("/dev/duet", O_RDWR);

    if (fd < 0) {
        fprintf ( stderr, "Failed to open /dev/duet. ERRNO = %d\n", errno );
        return -1;
    }

    auto const num_threads = std::thread::hardware_concurrency ();
    size_t const num_nodes = 128;

    volatile uint64_t * vaddr = static_cast<uint64_t *> (
            mmap(NULL, 128 * (num_threads + 1), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0) );

    if ( NULL == vaddr ) {
        fprintf ( stderr, "Mmap failed\n" );
        return -1;
    }

    node_t nodes [num_nodes];
    std::default_random_engine re(0xdeadbeef);
    std::uniform_real_distribution <double> dist ( 1.0, 2.0 );

    const double epssq = 1e-8;

    for ( int _i = 0; _i < 2; ++_i ) {
        printf ( "\nref %d\n", _i );
        for ( size_t i = 0; i < num_nodes; ++i ) {
            nodes[i].mass = dist (re);
            nodes[i].pos[0] = dist (re);
            nodes[i].pos[1] = dist (re);
            nodes[i].pos[2] = dist (re);

            printf ( "node %u: mass (%f), pos (%f, %f, %f)\n",
                    i, nodes[i].mass, nodes[i].pos[0], nodes[i].pos[1], nodes[i].pos[2] );
        }

        double pos0[3] = { dist(re), dist(re), dist(re) };
        printf ( "pos0 (%f, %f, %f)\n", pos0[0], pos0[1], pos0[2] );

        double phi_ref, acc_ref[3];
        // uint64_t start, end;
        // asm volatile (
        //         "rdcycle  %0"
        //         : "=r"(start)
        //     );
        ref ( pos0, epssq, nodes, num_nodes, phi_ref, acc_ref );
        // asm volatile (
        //         "rdcycle  %0"
        //         : "=r"(end)
        //     );
        // printf ( "ref: %llu cycles\n", end - start );

        printf ( "ref: phi (%f), acc (%f, %f, %f)\n", phi_ref, acc_ref[0], acc_ref[1], acc_ref[2] );

        vaddr[0]  /* epssq */ = *(reinterpret_cast <const uint64_t *> (&epssq) );
        vaddr[17] /* pos0x */ = *(reinterpret_cast <const uint64_t *> (&pos0[0]) );
        vaddr[18] /* pos0y */ = *(reinterpret_cast <const uint64_t *> (&pos0[1]) );
        vaddr[19] /* pos0z */ = *(reinterpret_cast <const uint64_t *> (&pos0[2]) );

        double phi_duet, acc_duet[3];
        // uint64_t start, end;
        // asm volatile (
        //         "rdcycle  %0"
        //         : "=r"(start)
        //         );
        for ( size_t i = 0; i < num_nodes; ++i ) {
            vaddr[16] = reinterpret_cast <uint64_t> (&nodes[i]);
        }
        while ( vaddr[20] < num_nodes );
        phi_duet =    *(reinterpret_cast <const volatile double *> (&vaddr[21]));
        acc_duet[0] = *(reinterpret_cast <const volatile double *> (&vaddr[22]));
        acc_duet[1] = *(reinterpret_cast <const volatile double *> (&vaddr[23]));
        acc_duet[2] = *(reinterpret_cast <const volatile double *> (&vaddr[24]));
        // asm volatile (
        //         "rdcycle  %0"
        //         : "=r"(end)
        //         );
        // printf ( "call 0: %llu cycles\n", end - start );

        if ( abs (phi_duet - phi_ref) > epssq )
            printf ( "[Error %d] phi: ref = %e != duet[0] = %e\n",
                    _i, phi_ref, phi_duet );
        if ( abs (acc_duet[0] - acc_ref[0]) > epssq )
            printf ( "[Error %d] acc[0]: ref = %e != duet[0] = %e\n",
                    _i, acc_ref[0], acc_duet[0] );
        if ( abs (acc_duet[1] - acc_ref[1]) > epssq )
            printf ( "[Error %d] acc[1]: ref = %e != duet[0] = %e\n",
                    _i, acc_ref[1], acc_duet[1] );
        if ( abs (acc_duet[2] - acc_ref[2]) > epssq )
            printf ( "[Error %d] acc[2]: ref = %e != duet[0] = %e\n",
                    _i, acc_ref[2], acc_duet[2] );

    }

    printf ( "done\n" );

    return 0;
}
