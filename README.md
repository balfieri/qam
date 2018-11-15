<p>
N-QAM is a two-dimensional multi-level signaling technique that uses two clocks separated by a quarter cycle, i.e., an in-phase clock and 
an quadrature clock.  N-QAM is used extensively in RF applications, but can also be used in purely electrical transmissions.
</p>

<p>
N-QAM is more robust than one-dimensional multi-level signaling techniques such as PAM-N.
</p>

<p>
This repository contains a C++ program that finds the optimal N-QAM points on the 2D constellation diagram.  This means
finding a set of N points that maximizes the minimum distance between any two points.  It uses brute force to do this.
The example defaults to 16-QAM, but you can change N_SQRT at the top of the program.  It assumes that there are sqrt(N) 
levels along each of the two dimensions.
</p>

<p>
The next step is to show a simple C++ simulation of transmitting and receiving N-QAM.
</p>

<p>
This is all open-source.  Refer to the LICENSE.md for licensing details.
</p>

<p>
To build and run the test on Linux, Cygwin, or macOS:
</p>
<pre>
doit.qam
</pre>

<p>
Bob Alfieri<br>
Chapel Hill, NC
</p>
