<p>
This repository contains a C++ program that simulates N-QAM where N is currently restricted to 4 or 16 (default is 16).  
</p>

<p>
N-QAM is a two-dimensional multi-level signaling technique that uses two clocks separated by a quarter cycle, i.e., an in-phase clock and 
an quadrature clock.  N-QAM is used extensively in RF applications, but can also be used in purely electrical transmissions.
</p>

<p>
N-QAM is more robust than one-dimensional multi-level signaling techniques such as PAM-N.
</p>

<p>
This is all open-source.  Refer to the LICENSE.md for licensing details.
</p>

<p>
To build and run the test on Linux, Cygwin, or macOS:
</p>
<pre>
doit.qam > qam.out
</pre>

<p>
There is also an <b>out2sp</b> script that can generate and run an ngspice or hspice simulation using the values from the qam.out output
file.  The script will likely require some fiddling for your environment.
</p>

<p>
Bob Alfieri<br>
Chapel Hill, NC
</p>
