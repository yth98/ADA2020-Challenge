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
        * [Mixed Integer Programming Models for Job Shop Scheduling: A Computational Analysis](https://tidel.mie.utoronto.ca/pubs/JSP_CandOR_2016.pdf)
    * Illustrate the objective function
        * The metric to be minimized
    * Illustrate the constraints: Disjunctive Model
        * Dependencies of operations
        * Capacity of each slice
* Reduce the execution time
    * Multithread / ~~Parallel~~ programming
        * SCIPsolveConcurrent (may be slower on Workstations)
    * ~~Divide & Conquer~~
    * ~~Branch pruning~~
    * Set the time limit explicitly
        * SCIPsetRealParam(..., "limits/time", ...)
        * The sub-optimal solution may be worse than traditional one
        * Put the most effort in Testcase 10

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
