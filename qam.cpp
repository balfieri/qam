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
// qam.c - simulate N-QAM (N = 4 or 16 currently)
//
#include <cstdint>
#include <string>
#include <cmath>
#include <iostream>
#include <bitset>
#include "stdlib.h"

static constexpr bool     debug              = false;

// config constants
static constexpr uint32_t N_SQRT             = 4;   // sqrt(N)
static constexpr double   CLK_GHZ            = 10;  // 10 GHz
static constexpr uint32_t CLK_TIMESTEP_CNT   = 50;  // per clock
static constexpr uint32_t SIM_CLK_CNT        = 8;
static constexpr double   mV_MAX             = 200; // 200 mV max per clock
static constexpr bool     USE_SIN_COS        = true; 
static constexpr double   TRANS_START_FRAC   = 0.20; // fraction of half-clock where transition starts
static constexpr double   TRANS_END_FRAC     = 0.30; // fraction of half-clock where transition ends

// derived constants
static constexpr double   N_SQRT_F           = N_SQRT;
static constexpr uint32_t N                  = N_SQRT * N_SQRT;
static constexpr uint32_t INIT_PHASE_CNT_LG2 = 8;
static constexpr uint32_t INIT_PHASE_CNT     = 1 << INIT_PHASE_CNT_LG2;
static constexpr double   PI_DIV_2           = M_PI / 2.0;
static constexpr double   EPSILON            = 1e-10;
static constexpr double   CLK_PERIOD_PS      = 1000.0 / CLK_GHZ;
static constexpr double   TIMESTEP_PS        = CLK_PERIOD_PS / double(CLK_TIMESTEP_CNT);
static constexpr double   RISING_START_PS    = CLK_PERIOD_PS/2.0 * TRANS_START_FRAC;
static constexpr double   RISING_END_PS      = CLK_PERIOD_PS/2.0 * TRANS_END_FRAC;
static constexpr double   FALLING_START_PS   = CLK_PERIOD_PS/2.0 * (1.0-TRANS_END_FRAC);
static constexpr double   FALLING_END_PS     = CLK_PERIOD_PS/2.0 * (1.0-TRANS_START_FRAC);
static constexpr double   mV_MID             = mV_MAX / 3.0;
static constexpr double   mV_INC             = mV_MID;

// global variables
static double x[N];
static double y[N];

// forward decls
void choose_points( void );
void sim( void );

int main( int argc, const char * argv[] )
{
    sim();
    return 0;
}

uint32_t rand_n( uint32_t n )
{
    return rand() % n;
}

void sim( void )
{
    //------------------------------------------------------
    // For each clock cycle
    //------------------------------------------------------
    std::cout << "TIMESTEP_PS=" << TIMESTEP_PS << "\n\n";
    double Q_mag_prev = mV_MAX;
    double eye_width_ps_min = 1000000.0;
    double eye_width_ps_max = 0.0;
    double eye_width_ps_tot = 0.0;
    for( uint32_t i = 0; i < SIM_CLK_CNT; i++ )
    {
        //------------------------------------------------------
        // Choose random bits from 0 .. N-1.
        // Then determine peak amplitude and polarity of I and Q clocks.
        //------------------------------------------------------
        uint32_t bits = rand_n( N );
        bool     I_pos = (bits & 1) != 0;
        bool     Q_pos = (bits & 2) != 0;
        double   I_mag;
        double   Q_mag;
        if ( N == 16 && (bits & 4) == 0 ) {
            I_mag = I_pos ? mV_MID : -mV_MID;
        } else {
            I_mag = I_pos ? mV_MAX : -mV_MAX;
        }
        if ( N == 16 && (bits & 8) == 0 ) {
            Q_mag = Q_pos ? mV_MID : -mV_MID;
        } else {
            Q_mag = Q_pos ? mV_MAX : -mV_MAX;
        }
        double   I_min = (I_mag == -mV_MAX) ? -1000000.0 : (I_mag-mV_INC);
        double   I_max = (I_mag ==  mV_MAX) ?  1000000.0 : (I_mag+mV_INC);

        //------------------------------------------------------
        // Figure out I and Q voltage at each timestep.
        // Let I be a sin() wave and Q be a cos() wave.
        // We define a clock period as PI.
        // Then sum them.  
        //------------------------------------------------------
        std::cout << i << ": " << std::bitset<N_SQRT>( bits ) << " I_mag=" << I_mag << " Q_mag=" << Q_mag << ":\n";
        uint32_t I_ts_eye_cnt = 0;
        uint32_t I_ts_eye_cnt_max = 0;
        for( uint32_t ts = 1; ts <= CLK_TIMESTEP_CNT; ts++ )
        {
            double a = double( ts ) * M_PI / double(CLK_TIMESTEP_CNT);
            double I_mV, Q_mV;
            if ( USE_SIN_COS ) {
                I_mV = I_mag * sin( a );
                double Q_mgg = (ts <= CLK_TIMESTEP_CNT/2) ? Q_mag_prev : -Q_mag;
                Q_mV = Q_mgg * cos( a );
            } else {
                //------------------------------------------------------
                // Sharp Edges
                //------------------------------------------------------
                bool I_rising = ts <= CLK_TIMESTEP_CNT/2;
                double rising_to    = I_rising ? I_mag      : Q_mag;  
                double falling_from = I_rising ? Q_mag_prev : I_mag;
                double rising_mV    = 0.0;
                double falling_mV   = falling_from;
                if ( ts >= RISING_END_PS ) {
                    rising_mV = rising_to;
                } else if ( ts >= RISING_START_PS ) {
                    rising_mV = (ts-RISING_START_PS)/(RISING_END_PS-RISING_START_PS) * rising_to;
                }
                if ( ts >= FALLING_END_PS ) {
                    falling_mV = 0.0;
                } else if ( ts >= FALLING_START_PS ) {
                    falling_mV = (1.0 - (ts-FALLING_START_PS)/(FALLING_END_PS-FALLING_START_PS)) * falling_from;
                }
                I_mV = I_rising ? rising_mV  : falling_mV;
                Q_mV = I_rising ? falling_mV : rising_mV;
            }

            double IQ_mV = I_mV + Q_mV;
            bool   in_eye = IQ_mV > I_min && IQ_mV < I_max;
            if ( in_eye ) {
                I_ts_eye_cnt++;
                if ( I_ts_eye_cnt > I_ts_eye_cnt_max ) I_ts_eye_cnt_max = I_ts_eye_cnt;
            } else {
                I_ts_eye_cnt = 0;
            }
            std::string clk_str = (ts == (CLK_TIMESTEP_CNT/2)) ? "  <---- I_clk samples here" :
                                  (ts == CLK_TIMESTEP_CNT)     ? "  <---- Q_clk samples here" : "";
            std::cout << (in_eye ? "*" : " ") << "   " << I_mV << " + " << Q_mV << " = " << IQ_mV << clk_str << "\n";
        }
        double eye_width_ps = double(I_ts_eye_cnt_max) * TIMESTEP_PS;
        std::cout << "    eye_width=" << eye_width_ps << " ps\n";
        if ( eye_width_ps < eye_width_ps_min ) eye_width_ps_min = eye_width_ps;
        if ( eye_width_ps > eye_width_ps_max ) eye_width_ps_max = eye_width_ps;
        eye_width_ps_tot += eye_width_ps;
        Q_mag_prev = Q_mag;
    }
    double eye_width_ps_avg = eye_width_ps_tot / double(SIM_CLK_CNT);
    std::cout << "\neye_width min..max = " << eye_width_ps_min << " ps .. " << eye_width_ps_max << " ps\n";
    std::cout << "\neye_width avg      = " << eye_width_ps_avg << " ps\n";
}
