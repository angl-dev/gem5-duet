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
            mmap(NULL, num_threads * 64 + 8, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0) );

    if ( NULL == vaddr ) {
        fprintf ( stderr, "Mmap failed\n" );
        return -1;
    }

    node_t nodes [num_nodes];
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_real_distribution <double> dist ( 1.0, 2.0 );
    for ( size_t i = 0; i < num_nodes; ++i ) {
        nodes[i].mass = dist (re);
        nodes[i].pos[0] = dist (re);
        nodes[i].pos[1] = dist (re);
        nodes[i].pos[2] = dist (re);
    }

    const double pos0[3] = { dist(re), dist(re), dist(re) };
    const double epssq = 1e-8;

    double phi_ref, acc_ref[3];
    ref ( pos0, epssq, nodes, num_nodes, phi_ref, acc_ref );

    vaddr[0]  /* epssq */ = *(reinterpret_cast <const uint64_t *> (&epssq) );
    vaddr[9]  /* pos0x */ = *(reinterpret_cast <const uint64_t *> (&pos0[0]) );
    vaddr[10] /* pos0y */ = *(reinterpret_cast <const uint64_t *> (&pos0[1]) );
    vaddr[11] /* pos0z */ = *(reinterpret_cast <const uint64_t *> (&pos0[2]) );

    // first call
    double phi_duet0, acc_duet0[3];
    for ( size_t i = 0; i < num_nodes; ++i ) {
        vaddr[8] = reinterpret_cast <uint64_t> (&nodes[i]);
    }
    phi_duet0 = vaddr[12];
    acc_duet0[0] = vaddr[13];
    acc_duet0[1] = vaddr[14];
    acc_duet0[2] = vaddr[15];

    // second call
    double phi_duet1, acc_duet1[3];
    for ( size_t i = 0; i < num_nodes; ++i ) {
        vaddr[8] = reinterpret_cast <uint64_t> (&nodes[i]);
    }
    phi_duet1 = vaddr[12];
    acc_duet1[0] = vaddr[13];
    acc_duet1[1] = vaddr[14];
    acc_duet1[2] = vaddr[15];

    if ( abs (phi_duet0 - phi_ref) < epssq )
        fprintf ( stderr, "phi: ref = %e != duet[0] = %e\n",
                phi_ref, phi_duet0 );
    if ( abs (acc_duet0[0] - acc_ref[0]) < epssq )
        fprintf ( stderr, "acc[0]: ref = %e != duet[0] = %e\n",
                acc_duet0[0], acc_ref[0] );
    if ( abs (acc_duet0[1] - acc_ref[1]) < epssq )
        fprintf ( stderr, "acc[1]: ref = %e != duet[0] = %e\n",
                acc_duet0[1], acc_ref[1] );
    if ( abs (acc_duet0[2] - acc_ref[2]) < epssq )
        fprintf ( stderr, "acc[2]: ref = %e != duet[0] = %e\n",
                acc_duet0[2], acc_ref[2] );

    if ( abs (phi_duet1 - phi_ref) < epssq )
        fprintf ( stderr, "phi: ref = %e != duet[0] = %e\n",
                phi_ref, phi_duet1 );
    if ( abs (acc_duet1[0] - acc_ref[0]) < epssq )
        fprintf ( stderr, "acc[0]: ref = %e != duet[0] = %e\n",
                acc_duet1[0], acc_ref[0] );
    if ( abs (acc_duet1[1] - acc_ref[1]) < epssq )
        fprintf ( stderr, "acc[1]: ref = %e != duet[0] = %e\n",
                acc_duet1[1], acc_ref[1] );
    if ( abs (acc_duet1[2] - acc_ref[2]) < epssq )
        fprintf ( stderr, "acc[2]: ref = %e != duet[0] = %e\n",
                acc_duet1[2], acc_ref[2] );

    printf ( "done\n" );

    return 0;
}
