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
// analyze.c - analyze signal coming out of the channel; reads in .raw file
//
#include <cstdint>
#include <string>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include "stdlib.h"

static constexpr bool     debug              = false;

// config constants
static constexpr uint32_t VLEVEL_CNT         = 4;   // number of voltage levels
static constexpr double   TX_CLK_GHZ         = 20;  // TX symbol transfer rate 
static constexpr double   RX_CLK_GHZ         = 100; // RX sample rate
static constexpr double   TX_mV_MAX          = 400; // 200 mV max for Tx source
static constexpr double   NOISE_mV_MAX       = 33;  // 33 mV max noise (our margins must be above this)

// derived constants
static constexpr double   TX_CLK_PERIOD_PS   = 1000.0 / TX_CLK_GHZ;
static constexpr double   RX_CLK_PERIOD_PS   = 1000.0 / RX_CLK_GHZ;
static constexpr double   RX_mV_MAX          = TX_mV_MAX/2; 
static constexpr double   Vt_HIGH            = RX_mV_MAX * 2.0 / 3.0;
static constexpr double   Vt_MID             = 0.0;
static constexpr double   Vt_LOW             = -Vt_HIGH;

void die( std::string msg )
{
    std::cout << "ERROR: " << msg << "\n";
    exit( 1 );
}

void parse_skip_whitespace( std::string s, size_t& pos )
{
    size_t len = s.length();
    while( pos < len ) 
    {
        char c = s.at( pos );
        if ( c != ' ' && c != '\t' && c != '\n' ) break;
        pos++;
    }
}

std::string parse_non_whitespace( std::string s, size_t& pos )
{
    parse_skip_whitespace( s, pos );
    size_t len = s.length();
    size_t pos_first = pos;
    std::string nw = "";
    while( pos < len ) 
    {
        char c = s.at( pos );
        if ( c == ' ' || c == '\t' || c == '\n' ) break;
        nw += c;
        pos++;
    }
    if ( nw == "" ) {
        std::cout << "ERROR: expected non-whitespace, got nothing more on this line starting at pos=" << pos_first << ": " << s << "\n";
        exit( 1 );
    }
    return nw;
}

int64_t parse_int( std::string s, size_t& pos )
{
    std::string i_s = parse_non_whitespace( s, pos );
    return std::stoi( i_s );
}

double parse_flt( std::string s, size_t& pos )
{
    std::string f_s = parse_non_whitespace( s, pos );
    return std::stof( f_s );
}

double lerp( double f1, const double f2, const double a )
{
    return a*f1 + (1.0 - a)*f2;
}

int pam4( double mV, double& vt, double& margin, double static_hi_lo_adjust=0.0, int prev_bits=0, double dynamic_hi_lo_adjust=0.0 )
{
    double vt_high_for_above = Vt_HIGH - static_hi_lo_adjust - ((prev_bits <= 2) ? dynamic_hi_lo_adjust : 0);
    double vt_high_for_below = Vt_HIGH - static_hi_lo_adjust;
    double vt_mid_for_above  = Vt_MID                        - ((prev_bits <= 1) ? dynamic_hi_lo_adjust : 0);
    double vt_mid_for_below  = Vt_MID                        + ((prev_bits >= 2) ? dynamic_hi_lo_adjust : 0);
    double vt_low_for_above  = Vt_LOW  + static_hi_lo_adjust;
    double vt_low_for_below  = Vt_LOW  + static_hi_lo_adjust + ((prev_bits >= 1) ? dynamic_hi_lo_adjust : 0);
    if ( mV > vt_high_for_above ) {
        vt     = vt_high_for_above;
        margin = mV - vt;
        return 0b11;
    } else if ( mV < vt_high_for_below && mV > vt_mid_for_above ) {
        vt     = vt_mid_for_above;
        margin = mV - vt;
        if ( (vt_high_for_below-mV) < margin ) {
            vt     = vt_high_for_below;
            margin = vt - mV;
        }
        return 0b10;
    } else if ( mV > vt_low_for_above  && mV < vt_mid_for_below ) {
        vt     = vt_low_for_above;
        margin = mV - vt;
        if ( (vt_mid_for_below-mV) < margin ) {
            vt     = vt_mid_for_below;
            margin = vt - mV;
        }
        return 0b01;
    } else {
        vt     = vt_low_for_below;
        margin = vt - mV;
        return 0b00;
    }
}

int main( int argc, const char * argv[] )
{
    if ( argc != 2 ) die( "usage: analyze <raw_file>" );
    std::string raw_file = std::string( argv[1] );

    //------------------------------------------------------------------
    // First skip through second 'Values:' header.
    //------------------------------------------------------------------
    std::ifstream fraw( raw_file );
    if ( !fraw.is_open() ) die( "could not open raw file " + raw_file );
    std::string s;
    uint32_t values_cnt = 0;
    while( values_cnt != 2 && std::getline( fraw, s ) )
    {
        if ( s.substr( 0, 7 ) == "Values:" ) {
            values_cnt++;
        }
    }

    //------------------------------------------------------------------
    // Read in all the values of iq_tx and iq_rx.
    //------------------------------------------------------------------
    struct Entry
    {
        int64_t index;
        double  time_ps;
        double  iq_tx_mv;
        double  iq_rx_mv;
    };
    std::vector<Entry> entries;

    while( std::getline( fraw, s ) )
    {
        size_t e = entries.size();
        entries.resize( e+1 );
        Entry& entry = entries[e];

        size_t pos = 0;
        entry.index   = parse_int( s, pos );
        entry.time_ps = parse_flt( s, pos ) * 1.0e12;
        for( uint32_t i = 0; i < 4; i++ )
        {
            if ( !std::getline( fraw, s ) ) die( "truncated entry at end of file" );
            pos = 0;
            if ( i == 2 ) entry.iq_tx_mv = parse_flt( s, pos ) *  500.0;   // in RX terms
            if ( i == 3 ) entry.iq_rx_mv = parse_flt( s, pos ) * 1000.0;
        }
    }
    fraw.close();

    //------------------------------------------------------------------
    // Sample iq_tx and iq_rx values at their periods.
    // Write the samples to new lists.
    //------------------------------------------------------------------
    struct Sample
    {
        double time_ps;
        double iq_mv;
        double margin;
        int    bits;
    };
    std::vector<Sample> tx_samples;
    std::vector<Sample> rx_samples;
    Entry entry_prev{ -1, -TX_CLK_PERIOD_PS, RX_mV_MAX, 0.0 };
    Entry entry;
    double iq_tx_time_ps = 0.0;
    double iq_rx_time_ps = 0.0;
    std::vector<double> iq_tx_values;
    std::vector<double> iq_rx_values;
    for( auto it = entries.begin(); it != entries.end(); it++ )
    {
        entry = *it;

        // iq_tx
        if ( entry.time_ps >= iq_tx_time_ps ) {
            double a     = (iq_tx_time_ps - entry_prev.time_ps) / (entry.time_ps - entry_prev.time_ps); 
            double iq_tx = lerp( entry_prev.iq_tx_mv, entry.iq_tx_mv, a );
            size_t vi    = iq_tx_values.size();
            iq_tx_values.resize( vi+1 );
            iq_tx_values[vi] = iq_tx;
            iq_tx_time_ps += TX_CLK_PERIOD_PS;
            double   vt;
            double   margin;
            uint32_t bits = pam4( iq_tx, vt, margin );
            bool above_noise = margin > NOISE_mV_MAX;
            if ( debug ) printf( "TX: %5d %4d %1d %5d %4d %c\n", int(entry.time_ps), int(iq_tx), bits, int(vt), int(margin), above_noise ? '+' : '-' );

            size_t si = tx_samples.size();
            tx_samples.resize( si+1 );
            Sample& sample = tx_samples[si];
            sample.time_ps = iq_tx_time_ps;
            sample.iq_mv   = iq_tx;
            sample.margin  = margin;
            sample.bits    = bits;
        }

        // iq_rx
        if ( entry.time_ps >= iq_rx_time_ps ) {
            double a     = (iq_rx_time_ps - entry_prev.time_ps) / (entry.time_ps - entry_prev.time_ps); 
            double iq_rx = lerp( entry_prev.iq_rx_mv, entry.iq_rx_mv, a );
            size_t vi    = iq_rx_values.size();
            iq_rx_values.resize( vi+1 );
            iq_rx_values[vi] = iq_rx;
            iq_rx_time_ps += RX_CLK_PERIOD_PS;
            double   vt;
            double   margin;
            uint32_t bits = pam4( iq_rx, vt, margin );
            bool above_noise = margin > NOISE_mV_MAX;
            if ( debug ) printf( "RX: %5d %4d %1d %5d %4d %c\n", int(entry.time_ps), int(iq_rx), bits, int(vt), int(margin), above_noise ? '+' : '-' );

            size_t si = rx_samples.size();
            rx_samples.resize( si+1 );
            Sample& sample = rx_samples[si];
            sample.time_ps = iq_rx_time_ps;
            sample.iq_mv   = iq_rx;
            sample.margin  = margin;
            sample.bits    = bits;
        }

        entry_prev = entry;
    }

    //------------------------------------------------------------------
    // Print all RX samples.
    //------------------------------------------------------------------
    if ( debug ) {
        for( size_t i = 0; i < rx_samples.size(); i++ )
        {
            const Sample& sample = rx_samples[i];
            bool above_noise = sample.margin > NOISE_mV_MAX;
            printf( "RX: %5d %4d %1d %4d %c\n", int(sample.time_ps), int(sample.iq_mv), sample.bits, 
                    int(sample.margin), above_noise ? '+' : '-' );
        }
        printf( "------------------------------------------------------------------------------\n" );
    }

    //------------------------------------------------------------------
    // Get answers for rx_stride=1 vs. normal.
    //------------------------------------------------------------------
    for( uint32_t stride_i = 1; stride_i < 2; stride_i++ )
    {
        const uint32_t rx_stride = (stride_i == 0) ? 1 : (RX_CLK_GHZ / TX_CLK_GHZ);

        double   best_pct = 0.0;
        double   best_static_hi_lo_adjust = 0.0;
        double   best_dynamic_hi_lo_adjust = 0.0;
        uint32_t best_rx_offset = 0;

        //------------------------------------------------------------------
        // Try various Vt_HIGH/Vt_LOW.
        // Assume that we'll never want to make Vt_HIGH higher (or Vt_LOW lower).
        //------------------------------------------------------------------
        for( double static_hi_lo_adjust = 0.0; static_hi_lo_adjust <= Vt_HIGH/4; static_hi_lo_adjust += 1.0 )
        {
            //------------------------------------------------------------------
            // Try various Vt_HIGH/Vt_LOW adjustments when coming from extreme bits values.
            //------------------------------------------------------------------
            for( double dynamic_hi_lo_adjust = 0.0; dynamic_hi_lo_adjust <= Vt_HIGH/4; dynamic_hi_lo_adjust += 1.0 )
            {
                //------------------------------------------------------------------
                // Try various RX time offsets.
                //------------------------------------------------------------------
                int  prev_chosen_bits = 1;
                for( uint32_t rx_offset = 0; rx_offset < rx_stride; rx_offset++ )
                {
                    uint32_t cnt = 0;
                    uint32_t above_noise_cnt = 0;
                    uint32_t val_cnt[16];
                    uint32_t val_above_noise_cnt[16];
                    for( uint32_t i = 0; i < 16; i++ ) 
                    {
                        val_cnt[i] = 0;
                        val_above_noise_cnt[i] = 0;
                    }
                    for( size_t i = rx_offset; i < rx_samples.size(); i += rx_stride )
                    {
                        bool ignore = i == rx_offset || i == (rx_offset+rx_stride);
                        if ( !ignore ) cnt++;            // don't count start-up
                        const Sample& sample = rx_samples[i];
                        double vt;
                        double margin;
                        int bits = pam4( sample.iq_mv, vt, margin, static_hi_lo_adjust, prev_chosen_bits, dynamic_hi_lo_adjust );
                        if ( !ignore ) val_cnt[bits]++;  // don't count start-up
                        bool above_noise = margin > NOISE_mV_MAX;
                        bool prev_above_noise = false;
                        if ( rx_stride > 1 && i != 0 ) {
                            const Sample& prev_sample = rx_samples[i-1];
                            double prev_vt;
                            double prev_margin;
                            int prev_bits = pam4( prev_sample.iq_mv, prev_vt, prev_margin, static_hi_lo_adjust, prev_chosen_bits, dynamic_hi_lo_adjust );
                            prev_above_noise = prev_bits == bits && prev_margin > NOISE_mV_MAX;
                        }
                        if ( !ignore && (above_noise || prev_above_noise) ) {
                            above_noise_cnt++;
                            val_above_noise_cnt[bits]++;
                        }
                        if ( debug ) printf( "RX: %5d %4d %1d %5d %4d %c\n", int(sample.time_ps), int(sample.iq_mv), bits, 
                                             int(vt), int(margin), ignore ? 'x' : above_noise ? '+' : prev_above_noise ? '^' : '-' );
                        prev_chosen_bits = bits;
                    }
                    double pct = double(above_noise_cnt) / double(cnt) * 100.0;
                    for( uint32_t i = 0; i < 16; i++ ) 
                    {
                        if ( val_cnt[i] > 0 ) {
                            double val_pct = double(val_above_noise_cnt[i]) / double(val_cnt[i]) * 100.0;
                            if ( debug ) printf( "    %1d: above noise: %d of %d samples (%0.2f%%)\n", i, val_above_noise_cnt[i], val_cnt[i], val_pct );
                        } 
                    }
                    if ( pct > best_pct ) {
                        printf( "NEW BEST: rx_stride=%d static_hi_lo_adjust=%0.2f dynamic_hi_lo_adjust=%0.2f rx_offset=%d above noise: %d of %d samples (%0.2f%%)\n", 
                                rx_stride, static_hi_lo_adjust, dynamic_hi_lo_adjust, rx_offset, above_noise_cnt, cnt, pct );
                        best_pct                  = pct;
                        best_static_hi_lo_adjust  = static_hi_lo_adjust;
                        best_dynamic_hi_lo_adjust = dynamic_hi_lo_adjust;
                        best_rx_offset            = rx_offset;
                    }
                }
            }
        }

        //------------------------------------------------------------------
        // Show all RX samples with chosen Vts and rx_offsets.
        //------------------------------------------------------------------
        uint32_t next_chosen = best_rx_offset;
        int  prev_chosen_bits = 0;
        int  prev_bits = 0;
        bool prev_above_noise = false;
        for( size_t i = 0; i < rx_samples.size(); i++ )
        {
            const Sample& sample = rx_samples[i];
            double vt;
            double margin;
            int bits = pam4( sample.iq_mv, vt, margin, best_static_hi_lo_adjust, prev_chosen_bits, best_dynamic_hi_lo_adjust );
            bool ignore    = i <= (best_rx_offset+rx_stride);
            bool is_chosen = i == next_chosen;
            bool above_noise = margin > NOISE_mV_MAX;
            if ( is_chosen ) next_chosen += rx_stride;
            printf( "RX: %5d %4d %1d %5d %4d %c %s\n", int(sample.time_ps), int(sample.iq_mv), bits, 
                    int(vt), int(margin), 
                    ignore ? 'x' : above_noise ? '+' : (is_chosen && prev_above_noise && prev_bits == bits) ? '^' : '-', 
                    ignore ? "(ignored)" : is_chosen ? "<===" : "" );
            if ( is_chosen ) prev_chosen_bits = bits;
            prev_bits = bits;
            prev_above_noise = above_noise;
        }

        printf( "\nrx_stride=%d static_hi_lo_adjust=%0.2f dynamic_hi_lo_adjust=%0.2f rx_offset=%d had best above-noise percentage of %0.2f%%\n", 
                rx_stride, best_static_hi_lo_adjust, best_dynamic_hi_lo_adjust, best_rx_offset, best_pct );
    }

    return 0;
}
