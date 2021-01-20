# ADA Final Challenge 2020 - Team31 #

## TODO
* Generate valid outputs
* Translate testcases to MIP problem instances
    * Solvers feasible with APIs
        * [OR-Tools](https://developers.google.com/optimization "Google OR-Tools") - CP-Sat
        * [SCIP](https://www.scipopt.org/ "Solving Constraint Integer Programs")
        * ~~[GLPK](https://www.gnu.org/software/glpk/ "GNU Linear Programming Kit")~~
        * ~~[lp_solve](http://lpsolve.sourceforge.net/5.5/ "MILP solver")~~
        * [Comparison of Open-Source Linear Programming Solvers](https://prod-ng.sandia.gov/techlib-noauth/access-control.cgi/2013/138847.pdf)
        * ~~[Mixed Integer Programming Models for Job Shop Scheduling: A Computational Analysis](https://tidel.mie.utoronto.ca/pubs/JSP_CandOR_2016.pdf)~~
    * Illustrate the objective function
        * The metric to be minimized
    * Illustrate the constraints: Disjunctive Model
        * Dependencies of operations
        * Capacity of each slice
* Find suitable FPTAS
* Related researches
    * [Parallel task scheduling problem - Wikipedia](https://en.wikipedia.org/wiki/Parallel_task_scheduling_problem)
    * [Scheduling for Parallel Processing](https://link.springer.com/book/10.1007%2F978-1-84882-310-5)
        * parallel identical processors
        * the number of processors is a variable
        * nonpreemptive
        * task graph is arbitrary
        * processing times are upper- and lower-bounded by 1 and 96
        * all tasks are ready at time 0
        * no task has any deadline
        * optimality criterion:
            * schedule length (C_max)
            * weighted sum of completion times (\sum {w_j c_j})
        * rigid, noncontiguous
    * [Scheduling multiprocessor tasks — An overview](https://www.sciencedirect.com/science/article/pii/0377221796001233)
    * [Scheduling Parallel Tasks: Approximation Algorithms](https://hal.inria.fr/hal-00003126/)
    * [Bounds on Multiprocessing Timing Anomalies](https://epubs.siam.org/doi/abs/10.1137/0117039)
    * [Approximation Algorithms for Scheduling Parallel Jobs](https://epubs.siam.org/doi/10.1137/080736491)
    * [Scheduling Multiprocessor Tasks to Minimize Schedule Length](https://ieeexplore.ieee.org/document/1676781) - independent, unit length
* Reduce the execution time
    * Multithread / ~~Parallel~~ programming
        * SCIPsolveConcurrent (may be slower on Workstations)
    * ~~Divide & Conquer~~
    * ~~Branch pruning~~
    * Set the time limit explicitly
        * parameters.set_max_time_in_seconds(...)
        * SCIPsetRealParam(..., "limits/time", ...)
        * The sub-optimal solution may be worse than traditional one
        * Put the most effort in Testcase 10

## Public
 \# | Slices | Ops |     jjl |          Our |       Strong | Remark
--: | -----: | --: | ------: | -----------: | -----------: | -----:
  0 |      3 |   5 |      75 |     51       |     51       |    Opt
  1 |      1 | 172 |   29584 |  67900.09074 |  67900.09074 |    Opt
  2 |      3 |  11 |     363 |    173.977   |    173.977   |    Opt
  3 |      2 |  14 |     392 | 109757.124   | 109757.124   |    Opt
  4 |      2 |  17 |     578 | 270386.62151 | 270533.85177 |    Opt
  5 |      4 |  34 |    4624 |  12894.75411 |  12667.7143  |   Weak
  6 |      4 |  33 |    4356 |  20025.32244 |  19501.36364 |   Weak
  7 |      4 |  28 |    3136 |  17654.8373  |  17695.212   | Strong
  8 |      8 |  40 |   12800 |  44729.84147 |  45288.91921 | Strong
  9 |      8 |  87 |   60552 |  93734.82354 |  68919.2094  |   Trad
 10 |     36 | 241 | 2090916 | 528026.72405 | 415366.00282 |   Trad

## Private
  \# | l |  n | Ops |          Metric |       Remark
---: | - | -: | --: | --------------: | -----------:
c031 | 1 |  1 |   1 |        1        |      Optimal
3d2a | 1 | 17 | 100 |   677320.389817 |      Optimal
5626 | 2 |  1 |   1 |      388.47968  |      Optimal
c8c5 | 2 | 30 | 100 |  3474392.07527  |   20s CP-SAT
4ee8 | 2 | 23 | 100 |   746456.396854 |   20s CP-SAT
2e58 | 3 |  3 |   6 |      214        |      Optimal
b06e | 3 |  5 |  11 |      101.77677  |      Optimal
1f21 | 3 |  7 |  26 |    61206.753544 |   60s CP-SAT
187b | 3 |  6 |  26 |     5553        |      Optimal
240e | 3 |  6 |  26 |    24219        |   60s CP-SAT
1955 | 3 | 20 |  60 |    10313.4259   |  Traditional
c53c | 3 | 17 |  60 |    19661        |   60s CP-SAT
4b3b | 3 | 20 |  65 |    18405.3323   |  Traditional
21ca | 3 | 30 | 100 |  2490123.69485  |   60s CP-SAT
6e33 | 3 | 10 | 100 |     9392        |   60s CP-SAT
01ef | 4 |  5 |  18 |      564.8458   | *Our 180s CP
d311 | 4 |  6 |  25 |      845.1861   | *Our 180s CP
657e | 4 | 15 |  34 |    17392.25525  |  180s CP-SAT
281e | 4 |  3 |  77 |   170151.483569 |  Traditional
fc0f | 4 |  4 | 100 |   440668.230417 |  Traditional
a72a | 5 |  2 |   7 |     6708        |      Optimal
f29f | 5 | 11 |  17 |    31237.063741 |  600s CP-SAT
368c | 5 |  4 |  21
e238 | 5 | 10 |  38
af14 | 5 | 25 |  99
1444 | 5 | 25 | 100
3e1e | 5 |  5 | 100
bc5f | 6 |  6 |  37 |    86108.7      |  180s CP-SAT
1516 | 6 | 10 |  40
64d1 | 6 |  2 |  47
ddf1 | 6 | 15 |  78
080c | 6 | 11 |  83
1ff9 | 6 | 16 | 100
7118 | 6 | 10 | 100
e9eb | 6 | 10 | 100
5247 | 7 |  6 |  42
778c | 7 | 10 |  50
4467 | 7 | 19 |  67
4551 | 7 | 27 |  76
210c | 7 | 12 |  83
7d9b | 7 | 22 |  85
621f | 8 |  4 |  33
ec0d | 8 | 15 |  40
95ca | 8 | 30 |  56
1b3d | 8 | 29 |  73
7427 | 8 | 10 |  73
60a9 | 8 | 19 |  75
94a0 | 8 | 10 |  84
20e3 | 8 | 25 |  85
d6af | 8 |  8 |  87
abaf | 8 | 10 |  88
3419 | 8 | 30 |  89
5204 | 8 | 30 |  89
8a44 | 8 | 30 |  90
fd5b | 8 | 30 |  90
cc41 | 8 | 29 |  91
18e7 | 8 | 17 |  93
2542 | 8 | 30 |  94
10e2 | 8 | 26 |  96
59bf | 8 | 20 |  96
6407 | 8 | 30 |  97
b41d | 8 | 30 |  97
d652 | 8 | 30 |  97
2c5f | 8 | 30 | 100
46ad | 8 | 30 | 100
5905 | 8 | 30 | 100
59a5 | 8 | 30 | 100
9052 | 8 | 30 | 100
fb0f | 8 | 30 | 100
3672 | 8 | 25 | 100
b779 | 8 | 22 | 100
cda6 | 8 | 19 | 100
96fa | 8 | 18 | 100
4b93 | 8 | 12 | 100
d829 | 8 |  7 | 100
0942 | 8 |  5 | 100
7725 | 8 |  3 | 100
77a2 | 8 |  1 | 100
