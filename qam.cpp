// qam.c - brute-force find optimal constellation diagram with largest minimum distance between any two points
//
#include <cstdint>
#include <string>
#include <cmath>
#include <iostream>
#include "stdlib.h"

static constexpr uint32_t N_SQRT             = 4;
static constexpr double   N_SQRT_F           = N_SQRT;
static constexpr uint32_t N                  = N_SQRT * N_SQRT;
static constexpr uint32_t INIT_PHASE_CNT_LG2 = 4;
static constexpr uint32_t INIT_PHASE_CNT     = 1 << INIT_PHASE_CNT_LG2;
static constexpr double   PI_DIV_2           = M_PI / 2.0;

int main( int argc, const char * argv[] )
{
    double init_phase[N_SQRT]; 
    init_phase[0] = 0.0;  // can always fix first level
    const uint32_t init_phase_total_cnt = INIT_PHASE_CNT * (1 << N_SQRT);
    double best_min_dist = 0.0;
    double x_best[N];
    double y_best[N];
    double x[N];
    double y[N];
    std::cout << "init_phase_total_cnt=" << init_phase_total_cnt << "\n";
    for( uint32_t i = 0; i < init_phase_total_cnt; i++ )
    {
        for( uint32_t j = 1; j < N_SQRT; j++ )
        {
            uint32_t nom = (i >> ((j-1)*INIT_PHASE_CNT_LG2)) & (INIT_PHASE_CNT - 1);
            init_phase[j] = M_PI/4.0 * double(nom)/double(INIT_PHASE_CNT);
            std::cout << "i=" << i << " init_phase[" << j << "]=" << init_phase[j] << " (nom=" << nom << ")\n";
        }

        // choose point locations
        uint32_t k = 0;
        double this_min_dist = 100000000.0;
        for( uint32_t m = 0; this_min_dist > best_min_dist && m < N_SQRT; m++ )
        {
            const double m_f = m+1;
            const double a = m_f/N_SQRT_F;
            double p = init_phase[m];
            for( uint32_t n = 0; this_min_dist > best_min_dist && n < N_SQRT; n++, k++, p += PI_DIV_2 )
            {
                x[k] = a * std::cos( p );
                y[k] = a * std::sin( p );
                std::cout << "    xy[" << a << "," << p << "]=[" << x[k] << "," << y[k] << "]\n";
                if ( k != 0 ) {
                    for( uint32_t kk = 0; this_min_dist > best_min_dist && kk < k; kk++ )
                    {
                        double x_diff = x[k] - x[kk];
                        double y_diff = y[k] - y[kk];
                        double dist = std::sqrt( x_diff*x_diff + y_diff*y_diff );
                        std::cout << "        dist=" << dist << " xy_diff=[" << x_diff << "," << y_diff << "]\n";
                        if ( dist == 0 ) exit( 1 );
                        if ( dist < this_min_dist ) this_min_dist = dist;
                    }
                }
            }
        }

        if ( this_min_dist > best_min_dist ) {
            best_min_dist = this_min_dist;
            std::cout << "    new best_min_dist=" << best_min_dist << "\n";
            for( k = 0; k < N; k++ )
            {
                x_best[k] = x[k];
                y_best[k] = y[k];
            }
        }
    }

    std::cout << "Best minimum distance: " << best_min_dist << "\n";
    std::cout << "Points: xy=\n";
    for( uint32_t k = 0; k < N; k++ )
    {
        std::cout << "    [" << x_best[k] << ", " << y_best[k] << "]\n";
    }

    return 0;
}
