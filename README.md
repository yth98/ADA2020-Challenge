# ADA Final Challenge 2020 - Team31 #

## Usage
* `./scheduler 01.in 01.out`

## Build and Run-time Dependency
* GCC 7.5+
* Google [OR-Tools](https://github.com/google/or-tools "Google OR-Tools - Google Optimization Tools")
* `/usr/include/ortools`, `/usr/lib/libortools.so`
* `make libs-or/lib/libortools.so` would download and build OR-Tools automatically
* `make scheduler` if OR-Tools is installed

## TODO
* [x] Generate valid outputs
* [x] Translate testcases to ~~MIP~~ CP problem instances
    * [x] Solvers feasible with APIs
        * [OR-Tools](https://developers.google.com/optimization "Google OR-Tools") - CP-SAT
        * ~~[SCIP](https://www.scipopt.org/ "Solving Constraint Integer Programs")~~
        * ~~[GLPK](https://www.gnu.org/software/glpk/ "GNU Linear Programming Kit")~~
        * ~~[lp_solve](http://lpsolve.sourceforge.net/5.5/ "MILP solver")~~
        * [Comparison of Open-Source Linear Programming Solvers](https://prod-ng.sandia.gov/techlib-noauth/access-control.cgi/2013/138847.pdf)
        * ~~[Mixed Integer Programming Models for Job Shop Scheduling: A Computational Analysis](https://tidel.mie.utoronto.ca/pubs/JSP_CandOR_2016.pdf)~~
    * [x] Illustrate the objective function
        * The metric to be minimized
    * [x] Illustrate the constraints: Disjunctive Model
        * Dependencies of operations
        * Capacity of each slice
* ~~Find suitable FPTAS~~
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
        * SatParameters::set_num_search_workers
        * ~~SCIPsolveConcurrent (may be slower on Workstations)~~
    * Divide & Conquer
        * Split jobs to groups
    * ~~Branch pruning~~
    * Set the time limit explicitly
        * SatParameters::set_max_time_in_seconds(...)
        * ~~SCIPsetRealParam(..., "limits/time", ...)~~
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
  5 |      4 |  34 |    4624 |  12667.7143  |  12667.7143  |    Opt
  6 |      4 |  33 |    4356 |  19501.14172 |  19501.36364 |    Opt
  7 |      4 |  28 |    3136 |  17533.5815  |  17695.212   |    Opt
  8 |      8 |  40 |   12800 |  43642.98051 |  45288.91921 | Strong
  9 |      8 |  87 |   60552 |  64573.2938  |  68919.2094  | Strong
 10 |     36 | 241 | 2090916 | 389586.49585 | 415366.00282 | Strong

## Private
  \# | l |  n | Ops |          Metric |       Remark
---: | - | -: | --: | --------------: | -----------:
c031 | 1 |  1 |   1 |        1        |      Optimal
3d2a | 1 | 17 | 100 |   677320.389817 |      Optimal
5626 | 2 |  1 |   1 |      388.47968  |   0s Optimal
c8c5 | 2 | 30 | 100 |  2094950.036765 |  120s CP-SAT
4ee8 | 2 | 23 | 100 |   683412.200993 |   60s CP-SAT
2e58 | 3 |  3 |   6 |      214        |   0s Optimal
b06e | 3 |  5 |  11 |      101.77677  |   0s Optimal
1f21 | 3 |  7 |  26 |    61206.753544 |   60s CP-SAT
187b | 3 |  6 |  26 |     5553        |  50s Optimal
240e | 3 |  6 |  26 |    23769        | 120s Optimal
1955 | 3 | 20 |  60 |     8076.669    |  360s CP-SAT
c53c | 3 | 17 |  60 |    16099        |  360s CP-SAT
4b3b | 3 | 20 |  65 |    13167.5427   |  360s CP-SAT
21ca | 3 | 30 | 100 |  1452587.795588 |  360s CP-SAT
6e33 | 3 | 10 | 100 |     7662        |  360s CP-SAT
01ef | 4 |  5 |  18 |      564.8458   | *Our 180s CP
d311 | 4 |  6 |  25 |      845.1861   | *Our 180s CP
657e | 4 | 15 |  34 |    14960.48323  |  480s CP-SAT
281e | 4 |  3 |  77 |   158807.638315 |  480s CP-SAT
fc0f | 4 |  4 | 100 |   386012.897789 |  480s CP-SAT
a72a | 5 |  2 |   7 |     6708        |   0s Optimal
f29f | 5 | 11 |  17 |    31237.063741 |  600s CP-SAT
368c | 5 |  4 |  21 |     4638        | 100s Opt-MIP
e238 | 5 | 10 |  38 |   153009.650022 |  600s CP-SAT
af14 | 5 | 25 |  99 |    43638.2      |  720s CP-SAT
1444 | 5 | 25 | 100 |   507194.93133  |  720s CP-SAT
3e1e | 5 |  5 | 100 |   149457.279682 |  720s CP-SAT
bc5f | 6 |  6 |  37 |    86108.7      |  180s CP-SAT
1516 | 6 | 10 |  40 |     7610.389    |  Traditional
64d1 | 6 |  2 |  47 |   127042.557474 |  900s CP-SAT
ddf1 | 6 | 15 |  78 |   238856.693142 | 1200s CP-SAT
080c | 6 | 11 |  83 |   105800.321041 | 1200s CP-SAT
1ff9 | 6 | 16 | 100 |  *876060.34     | *Traditional
7118 | 6 | 10 | 100 |  2083949.872824 | 1200s CP-SAT
e9eb | 6 | 10 | 100 |  2363918.966663 |  900s CP-SAT
5247 | 7 |  6 |  42 |    48515.092067 | 1080s CP-SAT
778c | 7 | 10 |  50 |   149754.138889 | 1080s CP-SAT
4467 | 7 | 19 |  67 |    33029.89308  |  Traditional
4551 | 7 | 27 |  76 |    36845.17234  |  Traditional
210c | 7 | 12 |  83 |   269026.028198 | 1080s CP-SAT
7d9b | 7 | 22 |  85 |   725300.748775 |  Traditional
621f | 8 |  4 |  33 |      570        | 1200s CP-SAT
ec0d | 8 | 15 |  40 |    61303.06067  | 1200s CP-SAT
95ca | 8 | 30 |  56 |   787465.26548  | 1200s CP-SAT
1b3d | 8 | 29 |  73 |  1242802.600223 |  Traditional
7427 | 8 | 10 |  73 |      758.102972 | 1200s CP-SAT
60a9 | 8 | 19 |  75 |      576.092008 | 1200s CP-SAT
94a0 | 8 | 10 |  84 |   388724.507176 | 1200s CP-SAT
20e3 | 8 | 25 |  85 |   880111.653036 | 1200s CP-SAT
d6af | 8 |  8 |  87 |   308061.409469 | 1200s CP-SAT
abaf | 8 | 10 |  88 |   291132.838896 | 1200s CP-SAT
3419 | 8 | 30 |  89 |   943287.678979 | 1200s CP-SAT
5204 | 8 | 30 |  89 |   346093.05308  |  Traditional
8a44 | 8 | 30 |  90 |    45648.958668 |  Traditional
fd5b | 8 | 30 |  90 |   386142.42016  |  Traditional
cc41 | 8 | 29 |  91 |   312216.35186  |  Traditional
18e7 | 8 | 17 |  93 |     2132        |  Traditional
2542 | 8 | 30 |  94 |   779116.665037 |  Traditional
10e2 | 8 | 26 |  96 |  1147528.548496 |  Traditional
59bf | 8 | 20 |  96 |     2441        |  Traditional
6407 | 8 | 30 |  97 |   456539.268123 |  Traditional
b41d | 8 | 30 |  97 |   112697        |  Traditional
d652 | 8 | 30 |  97 |   302854        |  Traditional
2c5f | 8 | 30 | 100 |   834279.616973 |  Traditional
46ad | 8 | 30 | 100 |  1477416.14074  |  Traditional
5905 | 8 | 30 | 100 |  1208250.244    |       CP-SAT
59a5 | 8 | 30 | 100 |      116.534    |  Traditional
9052 | 8 | 30 | 100 |  1192972.674    |       CP-SAT
fb0f | 8 | 30 | 100 |  1513040.322707 |       CP-SAT
3672 | 8 | 25 | 100 |  1335276.067131 |  Traditional
b779 | 8 | 22 | 100 |  1059680.409904 |  Traditional
cda6 | 8 | 19 | 100 |     1858        |  Traditional
96fa | 8 | 18 | 100 |   663633        |  Traditional
4b93 | 8 | 12 | 100 |     9276.196777 |  Traditional
d829 | 8 |  7 | 100 |   155595.28125  |  Traditional
0942 | 8 |  5 | 100 |   495290.452062 |  Traditional
7725 | 8 |  3 | 100 |   151608        |  Traditional
77a2 | 8 |  1 | 100 |    20485.359342 |  Traditional
