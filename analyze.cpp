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
static constexpr double   CLK_GHZ            = 10;  // 10 GHz
static constexpr double   mV_MAX_TX          = 200; // 200 mV max for Tx source

// derived constants
static constexpr double   CLK_PERIOD_PS      = 1000.0 / CLK_GHZ;
static constexpr double   mV_MAX_RX          = mV_MAX_TX/2; 

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
    // Now read in all the values of iq and iq_rx.
    //------------------------------------------------------------------
    struct Entry
    {
        int64_t index;
        double  time;
        double  iq;
        double  iq_rx;
    };
    std::vector<Entry> entries;

    while( std::getline( fraw, s ) )
    {
        size_t e = entries.size();
        entries.resize( e+1 );
        Entry& entry = entries[e];

        size_t pos = 0;
        entry.index = parse_int( s, pos );
        entry.time  = parse_flt( s, pos );
        for( uint32_t i = 0; i < 4; i++ )
        {
            if ( !std::getline( fraw, s ) ) die( "truncated entry at end of file" );
            pos = 0;
            if ( i == 2 ) entry.iq    = parse_flt( s, pos );
            if ( i == 3 ) entry.iq_rx = parse_flt( s, pos );
        }
    }
    fraw.close();
    return 0;
}
