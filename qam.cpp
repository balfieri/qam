// Copyright (c) 2014-2019 Robert A. Alfieri
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// qam.c - brute-force find optimal constellation diagram for N-QAM with largest minimum distance between any two points
//
#include <cstdint>
#include <string>
#include <cmath>
#include <iostream>
#include "stdlib.h"

static constexpr bool     debug              = false;
static constexpr uint32_t N_SQRT             = 4;
static constexpr double   N_SQRT_F           = N_SQRT;
static constexpr uint32_t N                  = N_SQRT * N_SQRT;
static constexpr uint32_t INIT_PHASE_CNT_LG2 = 8;
static constexpr uint32_t INIT_PHASE_CNT     = 1 << INIT_PHASE_CNT_LG2;
static constexpr double   PI_DIV_2           = M_PI / 2.0;
static constexpr double   EPSILON            = 1e-10;

int main( int argc, const char * argv[] )
{
    double init_phase[N_SQRT]; 
    init_phase[0] = 0.0;  // can always fix first level
    uint32_t init_phase_total_cnt = 1;
    for( uint32_t i = 1; i < N_SQRT; i++ )
    {
        init_phase_total_cnt *= INIT_PHASE_CNT;
    }

    double best_min_dist = 0.0;
    double x_best[N];
    double y_best[N];
    double x[N];
    double y[N];
    if ( debug ) std::cout << "init_phase_total_cnt=" << init_phase_total_cnt << "\n";
    for( uint32_t i = 0; i < init_phase_total_cnt; i++ )
    {
        for( uint32_t j = 1; j < N_SQRT; j++ )
        {
            uint32_t nom = (i >> ((j-1)*INIT_PHASE_CNT_LG2)) & (INIT_PHASE_CNT - 1);
            init_phase[j] = PI_DIV_2 * double(nom)/double(INIT_PHASE_CNT);
            if ( debug ) std::cout << "i=" << i << " init_phase[" << j << "]=" << init_phase[j] << " (nom=" << nom << ")\n";
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
                if ( x[k] >= -EPSILON && x[k] <= EPSILON ) x[k] = 0.0;
                if ( y[k] >= -EPSILON && y[k] <= EPSILON ) y[k] = 0.0;
                if ( debug ) std::cout << "    xy[" << a << "," << p << "]=[" << x[k] << "," << y[k] << "]\n";
                if ( k != 0 ) {
                    for( uint32_t kk = 0; this_min_dist > best_min_dist && kk < k; kk++ )
                    {
                        double x_diff = x[k] - x[kk];
                        double y_diff = y[k] - y[kk];
                        double dist = std::sqrt( x_diff*x_diff + y_diff*y_diff );
                        if ( debug ) std::cout << "        dist=" << dist << " xy_diff=[" << x_diff << "," << y_diff << "]\n";
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

    std::cout << "\nBest minimum distance: " << best_min_dist << "\n";
    std::cout << "Points on constellation diagram:\n";
    for( uint32_t k = 0; k < N; k++ )
    {
        std::cout << "    [" << x_best[k] << ", " << y_best[k] << "]\n";
    }

    return 0;
}
