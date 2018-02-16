Efficiently find the longest Eulerian path in an undirected weighted graph.
This is a path from a node i to node j, that uses each *edge* at most once.


Example
-------

Usage example:

    time ./longest-path fast < input
    43 nodes
    longest path length: 1511

    real    0m0.079s
    user    0m0.015s
    sys     0m0.015s

    time ./longest-path brute-force < input
    43 nodes
    longest path length: 1511

    real    0m1.398s
    user    0m0.015s
    sys     0m0.000s

The brute force solution will quickly get slower for larger problems. Although compiler optimizations can get it pretty competetive for the example problem.

Algorithm
-------

The algorithm is based on perfect matchings. Roughly: we remove edges so that all odd-degree nodes get even degree, then the remaining edges form an Eulerian path.

If the the resulting graph is still connected this is correct. However, when removing edges makes the graph disconnected, this algorithm does not give the correct result.

As a simple problematic example, consider the graph in the `bad-input` file. When finding the longest path between `0` and `1`, The two 1 cost edges are removed, making the graph disconnected, and missing the longest path of length 12.

The current implementation restricts the answer to the connected component containing the source and target nodes, potentially finding a shorter path.

