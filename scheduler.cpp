#include <algorithm>   // for max, copy, fill_n
#include <cassert>     // for assert
#include <cfenv>       // for feenableexcept
#include <cmath>       // for isfinite
#include <cstdint>     // for uint8_t, int8_t
#include <cstdlib>     // for exit
#include <cstring>     // for strcmp
#include <fstream>     // for ifstream
#include <iomanip>     // for operator<<, setprecision
#include <iostream>    // for operator<<, ifstream, basic_istream::operat...
#include <limits>      // for numeric_limits
#include <memory>      // for allocator, allocator_traits<>::value_type
#include <regex>       // for regex_match, match_results<>::_Base_type
#include <sstream>     // for stringstream
#include <string>      // for string, basic_string, operator+, char_traits
#include <tuple>       // for tuple
#include <utility>     // for pair
#include <vector>      // for vector, vector<>::reference, _Bit_reference

#include "scip/scip.h"
#include "scip/scipdefplugins.h"

namespace {

struct Operation {
    uint32_t duration{}, start_time{};
    uint16_t slices{}, ij{};
    bool done{false}, inTopo{false}, pre{false}, front{false}, back{false};
    std::vector<uint16_t> deps;
    std::basic_string<uint8_t> in_slice;
};

struct Job {
    double weight{};
    uint32_t duration{0};
    std::vector<Operation> ops;
    std::vector<uint16_t> opTopo;
};

struct Event {
	uint32_t time{};
	uint16_t job{}, op{};
	bool is_start{};
	bool operator<(const Event &rhs) const {
		return time != rhs.time ? time < rhs.time : is_start < rhs.is_start;
	}
};

void OpTopoSort(Job&, uint16_t&);

std::pair<uint16_t, std::vector<Job>> ReadJobs(const std::string &file) {
    uint16_t l{}; // Number of available slices
    uint16_t n{}; // Number of jobs
    std::ifstream testf(file);

    auto getnewline = [&]() {
        testf.get();
    };
    auto getspace = [&]() {
        testf.get();
    };

    testf.unsetf(std::ios_base::skipws);
    testf.exceptions(std::ios_base::failbit);

    testf >> l;
    getnewline();

    testf >> n;
    getnewline();

    std::vector<Job> jobs(n);
    for (auto &job : jobs) {
        size_t m{}; // Number of operations
        testf >> m;
        job.ops.resize(m);
        getnewline();

        std::string weight_str;
        testf >> weight_str;
        job.weight = stod(weight_str);
        getnewline();

        for (auto &op : job.ops) {
            testf >> op.slices;
            getspace();

            testf >> op.duration;
            job.duration += op.duration;
            getspace();

            size_t p{}; // Number of dependencies
            testf >> p;
            op.deps.resize(p);

            for (auto &dep : op.deps) {
                getspace();
                testf >> dep;
                dep -= 1;
            }
            getnewline();
        }
        // Sort the operations in (Reversed) Topological Ordering
        for (uint16_t j = 0; j < job.ops.size(); j++) OpTopoSort(job, j);
        assert(job.ops.size() == job.opTopo.size());
        job.ops[job.opTopo.front()].front = true;
        job.ops[job.opTopo.back()].back = true;
    }
    return {l, jobs};
}

long double WriteSchedule(
    const std::string &outfile, const std::vector<Job> &jobs) {
    std::ofstream outf(outfile);

    long double makespan = 0, weighted_flow;
    for (size_t j = 0; j < jobs.size(); j++) {
        long double job_end = 0;
        for (size_t k = 0; k < jobs[j].ops.size(); k++) {
            outf << jobs[j].ops[k].start_time;

            for (auto &s : jobs[j].ops[k].in_slice) {
                outf << " " << (s + 1);
            }

            uint32_t fin = jobs[j].ops[k].start_time + jobs[j].ops[k].duration;
            makespan = std::max<long double>(makespan, fin);
            job_end = std::max<long double>(job_end, fin);
            outf << "\n";
        }
        weighted_flow += static_cast<long double>(jobs[j].weight) * job_end;
    }

    weighted_flow += makespan;
    return weighted_flow;
}

long double CalculateScore(const std::string &outfile, std::vector<Job> &jobs) {
    return WriteSchedule(outfile, jobs);
}

static
SCIP_RETCODE FormulateMIP(SCIP* scip, std::vector<SCIP_VAR*> &c, std::vector<SCIP_VAR*> &x, std::vector<SCIP_VAR*> &y,
                          const std::string &lpfile, std::vector<Job> &jobs, const uint16_t &l, uint32_t &dGCD) {
    SCIP_VAR* Cmax;
    uint32_t V = 0; SCIP_Real VR;
    uint16_t j, j1, j2;
    uint8_t i, i1, i2, q, carriage{0};
    char name[SCIP_MAXSTRLEN];
    std::ofstream lpf(lpfile);
    std::vector<Operation*> Ops;
    std::vector<SCIP_VAR*> z;
    std::vector<bool> Deps;

    // Write out lpfile
    dGCD = jobs[0].ops[0].duration;
    for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++)
            dGCD = std::__gcd(dGCD, jobs[i].ops[j].duration);
    for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++)
            V += jobs[i].ops[j].duration / dGCD;
    lpf << "\\ The GCD of durations of all operations is " << dGCD << "\n";
    lpf << "\\ V = " << V << "\n";
    lpf << "MINIMIZE cmax";
    for(i = 0; i < jobs.size(); i++) lpf << " + " << jobs[i].weight << " c" << (i+1);
    lpf << "\nST\n";
    if(l >= 2) for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++)
            for(auto &d : jobs[i].ops[j].deps)
                lpf << "x"<<(i+1)<<"_"<<(j+1) << " - x"<<(i+1)<<"_"<<(d+1) << " >= " << (jobs[i].ops[d].duration/dGCD) << "\n";
    if(l == 1)
        for(i1 = 0; i1 < jobs.size(); i1++)
            for(i2 = i1+1; i2 < jobs.size(); i2++)
                lpf << "c"<<(i1+1) << " - c"<<(i2+1)
                    << " + " << V << " z"<<(i1+1)<<"_1_"<<(i2+1)<<"_1"
                    << " >= " << (jobs[i1].duration/dGCD) << "\n"
                    << "c"<<(i2+1) << " - c"<<(i1+1)
                    << " - " << V << " z"<<(i1+1)<<"_1_"<<(i2+1)<<"_1"
                    << " >= -" << (V-jobs[i2].duration/dGCD) << "\n";
    else for(q = 0; q < l; q++)
        for(i1 = 0; i1 < jobs.size(); i1++)
            for(j1 = 0; j1 < jobs[i1].ops.size(); j1++) {
                for(j2 = j1+1; j2 < jobs[i1].ops.size(); j2++) {
                    bool skip = false;
                    for(auto &d : jobs[i1].ops[j1].deps) if(d == j2) skip = true;
                    for(auto &d : jobs[i1].ops[j2].deps) if(d == j1) skip = true;
                    if(skip) continue;
                    lpf << "x"<<(i1+1)<<"_"<<(j1+1) << " - x"<<(i1+1)<<"_"<<(j2+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(q+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<<(j2+1)<<"_"<<(q+1)
                        << " + " << V << " z"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(i1+1)<<"_"<<(j2+1)
                        << " >= -" << (V*2-jobs[i1].ops[j2].duration/dGCD) << "\n"
                        << "x"<<(i1+1)<<"_"<<(j2+1) << " - x"<<(i1+1)<<"_"<<(j1+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(q+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<<(j2+1)<<"_"<<(q+1)
                        << " - " << V << " z"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(i1+1)<<"_"<<(j2+1)
                        << " >= -" << (V*3-jobs[i1].ops[j1].duration/dGCD) << "\n";
                }
                for(i2 = i1+1; i2 < jobs.size(); i2++)
                    for(j2 = 0; j2 < jobs[i2].ops.size(); j2++) {
                        if(l == 1 && !(jobs[i1].ops[j1].front && jobs[i2].ops[j2].back)
                                  && !(jobs[i2].ops[j2].front && jobs[i1].ops[j1].back)) continue;
                        lpf << "x"<<(i1+1)<<"_"<<(j1+1) << " - x"<<(i2+1)<<"_"<<(j2+1)
                            << " - " << V << " y"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(q+1)
                            << " - " << V << " y"<<(i2+1)<<"_"<<(j2+1)<<"_"<<(q+1)
                            << " + " << V << " z"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(i2+1)<<"_"<<(j2+1)
                            << " >= -" << (V*2-jobs[i2].ops[j2].duration/dGCD) << "\n"
                            << "x"<<(i2+1)<<"_"<<(j2+1) << " - x"<<(i1+1)<<"_"<<(j1+1)
                            << " - " << V << " y"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(q+1)
                            << " - " << V << " y"<<(i2+1)<<"_"<<(j2+1)<<"_"<<(q+1)
                            << " - " << V << " z"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(i2+1)<<"_"<<(j2+1)
                            << " >= -" << (V*3-jobs[i1].ops[j1].duration/dGCD) << "\n";
                    }
            }
    if(l >= 2) for(i = 0; i < jobs.size(); i++) {
        for(j = 0; j < jobs[i].ops.size(); j++) {
            for(q = 0; q < l; q++) {
                if(q) lpf << " + ";
                lpf << "y"<<(i+1)<<"_"<<(j+1)<<"_"<<(q+1);
            }
            lpf << " = " << jobs[i].ops[j].slices << "\n";
            lpf << "c"<<(i+1) << " - x"<<(i+1)<<"_"<<(j+1) << " >= " << (jobs[i].ops[j].duration/dGCD) << "\n";
        }
        lpf << "cmax - c"<<(i+1) << " >= 0\n";
    }
    if(l >= 10) for(i1 = 0; i1 < jobs.size()-1; i1++) for(i2 = i1+1; i2 < jobs.size(); i2++)
        for(j1 = 0; j1 < jobs[i1].ops.size(); j1++) for(j2 = 0; j2 < jobs[i2].ops.size(); j2++)
            lpf << "z"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(i2+1)<<"_"<<(j2+1)
                << " = " << (jobs[i1].weight * jobs[i2].ops[j2].duration >= jobs[i2].weight * jobs[i1].ops[j1].duration ? 1 : 0) << "\n";
    if(l == 1) lpf << "cmax = "<< (V) << "\n";
    lpf << "GENERAL cmax";
    for(i = 0; i < jobs.size(); i++) {
        lpf << "\nc"<<(i+1);
        if(l >= 2) for(j = 0; j < jobs[i].ops.size(); j++) {
            lpf << " x"<<(i+1)<<"_"<<(j+1);
            if(carriage++%16==15) lpf << "\n";
        }
    }
    lpf << "\nBINARY";
    for(i1 = 0; i1 < jobs.size(); i1++)
        if(l == 1) for(i2 = i1+1; i2 < jobs.size(); i2++)
            lpf << " z"<<(i1+1)<<"_1_"<<(i2+1)<<"_1";
        else for(j1 = 0; j1 < jobs[i1].ops.size(); j1++) {
            for(q = 0; q < l; q++)
                lpf << "\ny"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(q+1);
            for(j2 = j1+1; j2 < jobs[i1].ops.size(); j2++) {
                bool skip = false;
                for(auto &d : jobs[i1].ops[j1].deps) if(d == j2) skip = true;
                for(auto &d : jobs[i1].ops[j2].deps) if(d == j1) skip = true;
                if(skip) continue;
                lpf << " z"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(i1+1)<<"_"<<(j2+1);
                if(carriage++%16==15) lpf << "\n";
            }
            for(i2 = i1+1; i2 < jobs.size(); i2++) {
                for(j2 = 0; j2 < jobs[i2].ops.size(); j2++) {
                    lpf << " z"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(i2+1)<<"_"<<(j2+1);
                    if(carriage++%16==15) lpf << "\n";
                }
            }
        }
    lpf.close();

    // Setup the problem in SCIP
    VR = V;
    for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++) {
            jobs[i].ops[j].ij = Ops.size();
            Ops.push_back(&jobs[i].ops[j]);
        }
    // Variables
    c.resize(jobs.size());
    x.resize(Ops.size());
    y.resize(Ops.size()*l);
    z.resize(Ops.size()*Ops.size());
    Deps.resize(Ops.size()*Ops.size(), false);
    std::vector<SCIP_CONS*> Precedence, DisjunctiveP, DisjunctiveN, Slice, LastC, LastMax;
    SCIP_CALL( SCIPcreateProbBasic(scip, "JSP") );
    SCIP_CALL( SCIPcreateVarBasic(scip, &Cmax, "cmax", 0.0, VR, 1.0, SCIP_VARTYPE_INTEGER) );
    SCIP_CALL( SCIPaddVar(scip, Cmax) );
    if(l == 1) for(i = 0; i < jobs.size(); i++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "c%d", i+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &c[i], name, jobs[i].duration, VR, jobs[i].weight, SCIP_VARTYPE_INTEGER) );
        SCIP_CALL( SCIPaddVar(scip, c[i]) );
    }
    else for(i = 0; i < jobs.size(); i++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "c%d", i+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &c[i], name, 0.0, VR, jobs[i].weight, SCIP_VARTYPE_INTEGER) );
        SCIP_CALL( SCIPaddVar(scip, c[i]) );
    }
    if(l >= 2) for(j = 0; j < Ops.size(); j++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "x%d", j+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &x[j], name, 0.0, VR, 0.0, SCIP_VARTYPE_INTEGER) );
        SCIP_CALL( SCIPaddVar(scip, x[j]) );
    }
    if(l >= 2) for(j = 0; j < Ops.size(); j++) for(q = 0; q < l; q++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "y%d_%d", j+1, q+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &y[j*l+q], name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
        SCIP_CALL( SCIPaddVar(scip, y[j*l+q]) );
    }
    for(j1 = 0; j1 < Ops.size(); j1++) for(j2 = j1+1; j2 < Ops.size(); j2++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "z%d_%d", j1+1, j2+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &z[j1*Ops.size()+j2], name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
        SCIP_CALL( SCIPaddVar(scip, z[j1*Ops.size()+j2]) );
    }
    // Constraints
    if(l >= 2) for(i = 0; i < jobs.size(); i++) for(j = 0; j < jobs[i].ops.size(); j++) for(auto &d : jobs[i].ops[j].deps) {
        SCIP_CONS* Pre;
        jobs[i].ops[d].pre = true;
        Deps[jobs[i].ops[j].ij*Ops.size()+jobs[i].ops[d].ij] = true;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(3)%d-%d", jobs[i].ops[j].ij+1, jobs[i].ops[d].ij+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &Pre, name, 0, NULL, NULL, jobs[i].ops[d].duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, Pre, x[jobs[i].ops[j].ij], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, Pre, x[jobs[i].ops[d].ij], -1.0) );
        SCIP_CALL( SCIPaddCons(scip, Pre) );
        Precedence.push_back(Pre);
        SCIP_CALL( SCIPreleaseCons(scip, &Pre) );
    }
    if(l == 1) for(i1 = 0; i1 < jobs.size(); i1++) for(i2 = i1+1; i2 < jobs.size(); i2++) {
        SCIP_CONS *DisP, *DisN;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(4)%d-%d", i1+1, i2+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &DisP, name, 0, NULL, NULL, jobs[i1].duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, c[i1], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, c[i2], -1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, z[jobs[i1].ops[0].ij*Ops.size()+jobs[i2].ops[0].ij], VR) );
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(5)%d-%d", i1+1, i2+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &DisN, name, 0, NULL, NULL, -VR+jobs[i2].duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, c[i2], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, c[i1], -1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, z[jobs[i1].ops[0].ij*Ops.size()+jobs[i2].ops[0].ij], -VR) );
        SCIP_CALL( SCIPaddCons(scip, DisP) );
        SCIP_CALL( SCIPaddCons(scip, DisN) );
        Precedence.push_back(DisP);
        Precedence.push_back(DisN);
        SCIP_CALL( SCIPreleaseCons(scip, &DisP) );
        SCIP_CALL( SCIPreleaseCons(scip, &DisN) );
    }
    else for(j1 = 0; j1 < Ops.size(); j1++) for(j2 = j1+1; j2 < Ops.size(); j2++) for(q = 0; q < l; q++) {
        SCIP_CONS *DisP, *DisN;
        if(Deps[j1*Ops.size()+j2] || Deps[j2*Ops.size()+j1]) break;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(4)%d-%d-%d", j1+1, j2+1, q+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &DisP, name, 0, NULL, NULL, -VR*2+Ops[j2]->duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, x[j1], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, x[j2], -1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, y[j1*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, y[j2*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, z[j1*Ops.size()+j2], VR) );
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(5)%d-%d-%d", j1+1, j2+1, q+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &DisN, name, 0, NULL, NULL, -VR*3+Ops[j1]->duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, x[j2], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, x[j1], -1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, y[j1*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, y[j2*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, z[j1*Ops.size()+j2], -VR) );
        SCIP_CALL( SCIPaddCons(scip, DisP) );
        SCIP_CALL( SCIPaddCons(scip, DisN) );
        Precedence.push_back(DisP);
        Precedence.push_back(DisN);
        SCIP_CALL( SCIPreleaseCons(scip, &DisP) );
        SCIP_CALL( SCIPreleaseCons(scip, &DisN) );
    }
    if(l >= 2) for(j = 0; j < Ops.size(); j++) {
        SCIP_CONS *Slic;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(6)%d", j+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &Slic, name, 0, NULL, NULL, Ops[j]->slices, Ops[j]->slices) );
        for(q = 0; q < l; q++)
            SCIP_CALL( SCIPaddCoefLinear(scip, Slic, y[j*l+q], 1.0) );
        SCIP_CALL( SCIPaddCons(scip, Slic) );
        Slice.push_back(Slic);
        SCIP_CALL( SCIPreleaseCons(scip, &Slic) );
    }
    if(l >= 2) for(i = 0; i < jobs.size(); i++) for(j = 0; j < jobs[i].ops.size(); j++) {
        SCIP_CONS *Last;
        if(jobs[i].ops[j].pre) continue;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(7)%d", jobs[i].ops[j].ij+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &Last, name, 0, NULL, NULL, Ops[jobs[i].ops[j].ij]->duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, Last, c[i], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, Last, x[jobs[i].ops[j].ij], -1.0) );
        SCIP_CALL( SCIPaddCons(scip, Last) );
        LastC.push_back(Last);
        SCIP_CALL( SCIPreleaseCons(scip, &Last) );
    }
    for(i = 0; i < jobs.size(); i++) {
        SCIP_CONS *LastM;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(8)%d", i+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &LastM, name, 0, NULL, NULL, 0.0, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, LastM, Cmax, 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, LastM, c[i], -1.0) );
        SCIP_CALL( SCIPaddCons(scip, LastM) );
        LastMax.push_back(LastM);
        SCIP_CALL( SCIPreleaseCons(scip, &LastM) );
    }
    if(l == 1) {
        SCIP_CONS *LastH;
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &LastH, "(V)", 0, NULL, NULL, VR, VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, LastH, Cmax, 1.0) );
        SCIP_CALL( SCIPaddCons(scip, LastH) );
        SCIP_CALL( SCIPreleaseCons(scip, &LastH) );
    }
    if(l == 1) {
        std::vector<uint16_t> j_order(jobs.size());
        // Optimal Substructure
        for(i = 0; i < jobs.size(); i++) j_order[i] = i;
        std::sort(j_order.begin(), j_order.end(), [&](const uint16_t &a, const uint16_t &b) {
            return jobs[a].weight * jobs[b].duration >= jobs[b].weight * jobs[a].duration;
        });
        for(i = 0; i < jobs.size()-1; i++) {
            SCIP_CONS *Ones;
            (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(l==1)%d-%d", j_order[i]+1, j_order[i+1]+1);
            SCIP_CALL( SCIPcreateConsBasicLinear(scip, &Ones, name, 0, NULL, NULL, 0.0, SCIPinfinity(scip)) );
            SCIP_CALL( SCIPaddCoefLinear(scip, Ones, c[j_order[i+1]], 1.0) );
            SCIP_CALL( SCIPaddCoefLinear(scip, Ones, c[j_order[i]], -1.0) );
            SCIP_CALL( SCIPaddCons(scip, Ones) );
            SCIP_CALL( SCIPreleaseCons(scip, &Ones) );
        }
    }
    else if(Ops.size() >= 101) { // Heuristic for large problem
        for(i1 = 0; i1 < jobs.size()-1; i1++) for(i2 = i1+1; i2 < jobs.size(); i2++) for(auto &J1 : jobs[i1].ops) for(auto &J2 : jobs[i2].ops) {
            SCIP_CONS *Heur;
            SCIP_Real is_pre = jobs[i1].weight * J2.duration >= jobs[i2].weight * J1.duration ? 1.0 : 0.0;
            (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(Heur)%d-%d", J1.ij+1, J2.ij+1);
            SCIP_CALL( SCIPcreateConsBasicLinear(scip, &Heur, name, 0, NULL, NULL, is_pre, is_pre) );
            SCIP_CALL( SCIPaddCoefLinear(scip, Heur, z[J1.ij*Ops.size()+J2.ij], 1.0) );
            SCIP_CALL( SCIPaddCons(scip, Heur) );
            SCIP_CALL( SCIPreleaseCons(scip, &Heur) );
        }
    }
    SCIP_CALL( SCIPreleaseVar(scip, &Cmax) );
    for(j1 = 0; j1 < Ops.size(); j1++) for(j2 = j1+1; j2 < Ops.size(); j2++)
        SCIP_CALL( SCIPreleaseVar(scip, &z[j1*Ops.size()+j2]) );
    return SCIP_OKAY;
}

SCIP_RETCODE RunJSP(const std::string &outfile, std::vector<Job> &jobs, const uint16_t &l) {
    SCIP* scip;
    SCIP_SOL* Sol;
    uint32_t dGCD;
    int NSols, tLimit;
    std::vector<SCIP_VAR*> c, x, y;
    SCIP_CALL( SCIPcreate(&scip) );
    SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

    SCIP_CALL( FormulateMIP(scip, c, x, y, outfile+".lp", jobs, l, dGCD) );
    // SCIP_CALL( SCIPprintOrigProblem(scip, NULL, "cip", FALSE) );
    // std::cerr << "\nNumber of operations: " << x.size() << "\n";
    // There is a time limit of 12 hours for the public tests combined
    // The time limit for the private tests combined is 24 hours
    switch(x.size()) {
        case   1 ...  20: tLimit =   720; break; // 0 1 2 3 4
        case  21 ...  35: tLimit =  3600; break; // 7 6 5
        case  36 ... 100: tLimit =  5400; break; // 8 9
        default: tLimit = 20880; // 10
    }
    SCIP_CALL( SCIPsetRealParam(scip, "limits/time", tLimit) );
    if(x.size() <= 35) SCIP_CALL( SCIPsetIntParam(scip, "misc/usesymmetry", 0) );

    SCIPinfoMessage(scip, NULL, "\nPresolving...\n");
    SCIP_CALL( SCIPpresolve(scip) );

    SCIPinfoMessage(scip, NULL, "\nSolving...\n");
    SCIP_CALL( SCIPsolve(scip) );

    if( (NSols = SCIPgetNSols(scip)) > 0 ) {
        SCIPinfoMessage(scip, NULL, "\nSolution:\n");
        Sol = SCIPgetBestSol(scip);
        SCIP_CALL( SCIPprintSol(scip, Sol, NULL, FALSE) );
        if(l == 1) for(uint8_t i = 0; i < jobs.size(); i++) {
            uint32_t start_time = SCIPgetSolVal(scip, Sol, c[i]) * dGCD - jobs[i].duration;
            for(auto &j : jobs[i].opTopo) {
                jobs[i].ops[j].start_time = start_time;
                start_time += jobs[i].ops[j].duration;
                jobs[i].ops[j].in_slice.push_back(0);
            }
        }
        else for(auto &job : jobs) for(auto &op : job.ops) {
            op.start_time = SCIPgetSolVal(scip, Sol, x[op.ij]) * dGCD;
            // std::cerr<<SCIPvarGetName(x[op.ij])<<" "<<SCIPgetSolVal(scip, Sol, x[op.ij])<<" "<<op.start_time<<"\n";
            for(uint8_t q = 0; q < l; q++) {
                if(SCIPisFeasEQ(scip, SCIPgetSolVal(scip, Sol, y[op.ij*l+q]), 1.0)) op.in_slice.push_back(q);
                // std::cerr<<SCIPvarGetName(y[op.ij*l+q])<<" "<<SCIPgetSolVal(scip, Sol, y[op.ij*l+q])<<"\n";
            }
        }
    }

    for(auto &i : c) SCIP_CALL( SCIPreleaseVar(scip, &i) );
    if(l >= 2) for(auto &i : x) SCIP_CALL( SCIPreleaseVar(scip, &i) );
    if(l >= 2) for(auto &j : y) SCIP_CALL( SCIPreleaseVar(scip, &j) );
    SCIP_CALL( SCIPfree(&scip) );
    return NSols ? SCIP_OKAY : SCIP_ERROR;
}

void OpTopoSort(Job &job, uint16_t &o) {
    if(job.ops[o].inTopo) return;
    assert(!job.ops[o].done);
    job.ops[o].done = true;
    for(auto &d : job.ops[o].deps)
        OpTopoSort(job, d);
    job.ops[o].done = false;
    job.ops[o].inTopo = true;
    job.opTopo.push_back(o);
}

void TraditionalScheduling(std::vector<Job> &jobs, const uint16_t &l) {
    uint32_t global_start{0};
    std::vector<uint16_t> j_order(jobs.size());
    // Sort the jobs by weight in descending order
    for (uint8_t i = 0; i < jobs.size(); i++) j_order[i] = i;
    std::sort(j_order.begin(), j_order.end(), [&](const uint16_t &a, const uint16_t &b) {
        if(l == 1) return jobs[a].weight * jobs[b].duration >= jobs[b].weight * jobs[a].duration;
        return jobs[a].weight >= jobs[b].weight;
    });
    for (auto &j : j_order) {
        uint32_t j_end{global_start};
        // Schedule each operation in the job
        for (auto &o : jobs[j].opTopo) {
            uint32_t j_start{j_end};
            assert(!jobs[j].ops[o].done);
            for (auto &d : jobs[j].ops[o].deps) {
                j_start = std::max<uint32_t>(j_start, jobs[j].ops[d].start_time + jobs[j].ops[d].duration);
            }
            jobs[j].ops[o].start_time = j_end;
            // std::cerr << o << " " << jobs[j].ops[o].start_time << " " << j_start << " " << j_end << " " << jobs[j].ops[o].slices << "\n";
            for (uint8_t s = 0; s < jobs[j].ops[o].slices; s++)
                jobs[j].ops[o].in_slice.push_back(s);
            j_end = std::max<uint32_t>(j_end, jobs[j].ops[o].start_time + jobs[j].ops[o].duration);
            jobs[j].ops[o].done = true;
        }
        // std::cerr << "\n";
        global_start = j_end;
    }
}

}  // namespace

int main(int argc, char **argv) {
    assert(argc == 3);
    auto [l, jobs] = ReadJobs(argv[1]);

    if(l == 1 || RunJSP(argv[2], jobs, l) != SCIP_OKAY) TraditionalScheduling(jobs, l);
    const auto score = CalculateScore(argv[2], jobs);

    std::cout << std::fixed << std::setprecision(8) << score << '\n';
}
