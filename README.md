# no3inline

Place `2N` points on an `N x N` grid so that no three points lie on the same
straight line.

Build with a C compiler:

```sh
cc -std=c11 -O2 -Wall -Wextra -Wpedantic main.c -o no3inline
```

Run randomized greedy search (also the default):

```sh
./no3inline 50
# or explicitly:
./no3inline --random 50
# choose the number of randomized attempts:
./no3inline --random --attempts 1000 50
```

The randomized mode shuffles the grid and greedily adds points, retrying up to
200 times by default. Use `--attempts COUNT` to set a different positive
attempt limit. It prints a grid using `X` for a point and `.` for an empty
cell. Because this is a heuristic, it may report failure even if a placement
exists.

For exhaustive search, use:

```sh
./no3inline --exact 5
```

Exact mode checks candidate placements systematically and finds a solution if
one exists, but its runtime can grow very quickly with `N`. `N` must be at
least 2; `N = 1` cannot hold `2N` distinct points.
