# ADA Final Challenge 2020 - Team31 #

## TODO
* Generate valid outputs
* Translate testcases to MIP problem instances
    * Solvers feasible with APIs
        * [GLPK](https://www.gnu.org/software/glpk/ "GNU Linear Programming Kit")
        * [lp_solve](http://lpsolve.sourceforge.net/5.5/ "MILP solver")
        * [Comparison of Open-Source Linear Programming Solvers](https://prod-ng.sandia.gov/techlib-noauth/access-control.cgi/2013/138847.pdf)
    * Illustrate the objective function
        * The metric to be minimized
    * Illustrate the constraints
        * Dependencies of operations
        * Capacity of each slice
* Reduce the execution time
    * Multithread / Parallel programming
    * Divide & Conquer
    * Branch pruning