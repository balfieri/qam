// qam.c - brute-force find optimal constellation diagram with largest minimum distance between any two points
//
#include <cstdint>
#include <string>
#include <cmath>
#include <iostream>

static constexpr uint32_t N_SQRT         = 4;
static constexpr double   N_SQRT_F       = N_SQRT;
static constexpr uint32_t N              = N_SQRT * N_SQRT;
static constexpr uint32_t INIT_PHASE_CNT = 8;
static constexpr double   PI_DIV_2       = M_PI / 2.0;

int main( int argc, const char * argv[] )
{
    double init_phase[N_SQRT]; 
    init_phase[0] = 0.0;  // can always fix first level
    uint32_t init_phase_total_cnt = INIT_PHASE_CNT * (1 << (N_SQRT - 1));
    double best_min_dist = 0.0;
    double x[N];
    double y[N];
    for( uint32_t i = 0; i < init_phase_total_cnt; i++ )
    {
        for( uint32_t j = 1; j < N_SQRT; j++ )
        {
            uint32_t nom = (i / (INIT_PHASE_CNT * (1 << (j-1)))) % INIT_PHASE_CNT;
            init_phase[j] = M_PI/4.0 * double(nom)/double(INIT_PHASE_CNT);
        }

        // choose point locations
        uint32_t k = 0;
        double this_min_dist = 100000000.0;
        for( uint32_t m = 0; this_min_dist > best_min_dist && m < N_SQRT; m++ )
        {
            const double m_f = m;
            const double a = m_f/N_SQRT_F;
            double p = init_phase[i];
            for( uint32_t n = 0; this_min_dist > best_min_dist && n < N_SQRT; n++, k++, n += PI_DIV_2 )
            {
                x[k] = a * std::cos( p );
                y[k] = a * std::sin( -p );

                if ( k != 0 ) {
                    for( uint32_t kk = 0; this_min_dist > best_min_dist && kk < k; k++ )
                    {
                        double x_diff = x[k] - x[kk];
                        double y_diff = y[k] - y[kk];
                        double dist = std::sqrt( x_diff*x_diff + y_diff*y_diff );
                        if ( dist < this_min_dist ) this_min_dist = dist;
                    }
                }
            }
        }

        if ( this_min_dist > best_min_dist ) best_min_dist = this_min_dist;
    }

    std::cout << "Best minimum distance: " << best_min_dist << "\n";
    std::cout << "Points: xy=\n";
    for( uint32_t k = 0; k < N; k++ )
    {
        std::cout << "    [" << x[k] << ", " << y[k] << "]\n";
    }

    return 0;
}
