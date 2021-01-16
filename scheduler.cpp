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
    bool done{false}, inTopo{false};
    std::vector<uint16_t> deps;
    std::basic_string<uint8_t> in_slice;
};

struct Job {
    double weight{};
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
SCIP_RETCODE FormulateMIP(SCIP* scip, std::vector<SCIP_VAR*> &x, std::vector<SCIP_VAR*> &y,
                          const std::string &lpfile, std::vector<Job> &jobs, const uint16_t &l, uint32_t &dGCD) {
    SCIP_VAR* Cmax;
    uint32_t V = 0; SCIP_Real VR;
    uint16_t j, j1, j2;
    uint8_t i, i1, i2, q, carriage{0};
    char name[SCIP_MAXSTRLEN];
    std::ofstream lpf(lpfile);
    std::vector<Operation*> Ops;

    // Write out lpfile
    dGCD = jobs[0].ops[0].duration;
    for(i = 0; i < jobs.size(); i++)
        for (j = 0; j < jobs[i].ops.size(); j++)
            dGCD = std::__gcd(dGCD, jobs[i].ops[j].duration);
    for(i = 0; i < jobs.size(); i++)
        for (j = 0; j < jobs[i].ops.size(); j++)
            V += jobs[i].ops[j].duration / dGCD;
    lpf << "\\ The GCD of durations of all operations is " << dGCD << "\n";
    lpf << "\\ V = " << V << "\n";
    lpf << "MINIMIZE cmax";
    for(i = 0; i < jobs.size(); i++) lpf << " + " << jobs[i].weight << " c" << (i+1);
    lpf << "\nST\n";
    for(i = 0; i < jobs.size(); i++)
        for (j = 0; j < jobs[i].ops.size(); j++)
            for(auto &d : jobs[i].ops[j].deps)
                lpf << "x"<<(i+1)<<"_"<< (j+1) << " - x"<<(i+1)<<"_"<< (d+1) << " >= " << (jobs[i].ops[d].duration/dGCD) << "\n";
    for(q = 0; q < l; q++)
        for(i1 = 0; i1 < jobs.size(); i1++)
            for (j1 = 0; j1 < jobs[i1].ops.size(); j1++) {
                for (j2 = j1+1; j2 < jobs[i1].ops.size(); j2++) {
                    lpf << "x"<<(i1+1)<<"_"<< (j1+1) << " - x"<<(i1+1)<<"_"<< (j2+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(q+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                        << " + " << V << " z"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(i1+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                        << " >= -" << (V*2-jobs[i1].ops[j2].duration/dGCD) << "\n";
                    lpf << "x"<<(i1+1)<<"_"<< (j2+1) << " - x"<<(i1+1)<<"_"<< (j1+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(q+1)
                        << " - " << V << " y"<<(i1+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                        << " - " << V << " z"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(i1+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                        << " >= -" << (V*3-jobs[i1].ops[j1].duration/dGCD) << "\n";
                }
                for(i2 = i1+1; i2 < jobs.size(); i2++)
                    for (j2 = 0; j2 < jobs[i2].ops.size(); j2++) {
                        lpf << "x"<<(i1+1)<<"_"<< (j1+1) << " - x"<<(i2+1)<<"_"<< (j2+1)
                            << " - " << V << " y"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(q+1)
                            << " - " << V << " y"<<(i2+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                            << " + " << V << " z"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(i2+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                            << " >= -" << (V*2-jobs[i2].ops[j2].duration/dGCD) << "\n";
                        lpf << "x"<<(i2+1)<<"_"<< (j2+1) << " - x"<<(i1+1)<<"_"<< (j1+1)
                            << " - " << V << " y"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(q+1)
                            << " - " << V << " y"<<(i2+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                            << " - " << V << " z"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(i2+1)<<"_"<< (j2+1)<<"_"<<(q+1)
                            << " >= -" << (V*3-jobs[i1].ops[j1].duration/dGCD) << "\n";
                    }
            }
    for(i = 0; i < jobs.size(); i++) {
        for (j = 0; j < jobs[i].ops.size(); j++) {
            for(q = 0; q < l; q++) {
                if(q) lpf << " + ";
                lpf << "y"<<(i+1)<<"_"<<(j+1)<<"_"<<(q+1);
            }
            lpf << " >= " << jobs[i].ops[j].slices << "\n";
            lpf << "c"<<(i+1) << " - x"<<(i+1)<<"_"<< (j+1) << " >= " << (jobs[i].ops[j].duration/dGCD) << "\n";
        }
        lpf << "cmax - c"<<(i+1) << " >= 0\n";
    }
    lpf << "GENERAL cmax";
    for(i = 0; i < jobs.size(); i++) {
        lpf << "\nc"<<(i+1);
        for (j = 0; j < jobs[i].ops.size(); j++) {
            lpf << " x"<<(i+1)<<"_"<<(j+1);
            if(carriage++%16==15) lpf << "\n";
        }
    }
    lpf << "\nBINARY";
    for(q = 0; q < l; q++)
        for(i1 = 0; i1 < jobs.size(); i1++)
            for (j1 = 0; j1 < jobs[i1].ops.size(); j1++) {
                lpf << "\ny"<<(i1+1)<<"_"<<(j1+1)<<"_"<<(q+1);
                for (j2 = j1+1; j2 < jobs[i1].ops.size(); j2++) {
                    lpf << " z"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(i1+1)<<"_"<< (j2+1)<<"_"<<(q+1);
                    if(carriage++%16==15) lpf << "\n";
                }
                for(i2 = i1+1; i2 < jobs.size(); i2++) {
                    for (j2 = 0; j2 < jobs[i2].ops.size(); j2++) {
                        lpf << " z"<<(i1+1)<<"_"<< (j1+1)<<"_"<<(i2+1)<<"_"<< (j2+1)<<"_"<<(q+1);
                        if(carriage++%16==15) lpf << "\n";
                    }
                }
            }

    // Setup the problem in SCIP
    VR = V;
    for(i = 0; i < jobs.size(); i++)
        for (j = 0; j < jobs[i].ops.size(); j++) {
            jobs[i].ops[j].ij = Ops.size();
            Ops.push_back(&jobs[i].ops[j]);
        }
    // Variables
    SCIP_VAR* c[jobs.size()];
    SCIP_VAR* z[Ops.size()*Ops.size()*l];
    x.resize(Ops.size());
    y.resize(Ops.size()*l);
    std::vector<SCIP_CONS*> Precedence, DisjunctiveP, DisjunctiveN, Slice, LastC, LastMax;
    SCIP_CALL( SCIPcreateProbBasic(scip, "JSP") );
    SCIP_CALL( SCIPcreateVarBasic(scip, &Cmax, "cmax", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_INTEGER) );
    SCIP_CALL( SCIPaddVar(scip, Cmax) );
    for(i = 0; i < jobs.size(); i++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "c%d", i+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &c[i], name, 0.0, SCIPinfinity(scip), jobs[i].weight, SCIP_VARTYPE_INTEGER) );
        SCIP_CALL( SCIPaddVar(scip, c[i]) );
    }
    for(j = 0; j < Ops.size(); j++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "x%d", j+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &x[j], name, 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_INTEGER) );
        SCIP_CALL( SCIPaddVar(scip, x[j]) );
    }
    for(j = 0; j < Ops.size(); j++) for(q = 0; q < l; q++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "y%d_%d", j+1, q+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &y[j*l+q], name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
        SCIP_CALL( SCIPaddVar(scip, y[j*l+q]) );
    }
    for(j1 = 0; j1 < Ops.size(); j1++) for(j2 = j1+1; j2 < Ops.size(); j2++) for(q = 0; q < l; q++) {
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "z%d_%d_%d", j1+1, j2+1, q+1);
        SCIP_CALL( SCIPcreateVarBasic(scip, &z[j1*Ops.size()*l+j2*l+q], name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
        SCIP_CALL( SCIPaddVar(scip, z[j1*Ops.size()*l+j2*l+q]) );
    }
    // Constraints
    for(i = 0; i < jobs.size(); i++) for (j = 0; j < jobs[i].ops.size(); j++) for(auto &d : jobs[i].ops[j].deps) {
        SCIP_CONS* Pre;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(3)%d-%d", jobs[i].ops[j].ij+1, jobs[i].ops[d].ij+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &Pre, name, 0, NULL, NULL, jobs[i].ops[d].duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, Pre, x[jobs[i].ops[j].ij], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, Pre, x[jobs[i].ops[d].ij], -1.0) );
        SCIP_CALL( SCIPaddCons(scip, Pre) );
        Precedence.push_back(Pre);
        SCIP_CALL( SCIPreleaseCons(scip, &Pre) );
    }
    for(j1 = 0; j1 < Ops.size(); j1++) for(j2 = j1+1; j2 < Ops.size(); j2++) for(q = 0; q < l; q++) {
        SCIP_CONS *DisP, *DisN;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(4)%d-%d-%d", j1+1, j2+1, q+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &DisP, name, 0, NULL, NULL, -VR*2+Ops[j2]->duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, x[j1], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, x[j2], -1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, y[j1*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, y[j2*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisP, z[j1*Ops.size()*l+j2*l+q], VR) );
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(5)%d-%d-%d", j1+1, j2+1, q+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &DisN, name, 0, NULL, NULL, -VR*3+Ops[j1]->duration/dGCD, SCIPinfinity(scip)) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, x[j2], 1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, x[j1], -1.0) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, y[j1*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, y[j2*l+q], -VR) );
        SCIP_CALL( SCIPaddCoefLinear(scip, DisN, z[j1*Ops.size()*l+j2*l+q], -VR) );
        SCIP_CALL( SCIPaddCons(scip, DisP) );
        SCIP_CALL( SCIPaddCons(scip, DisN) );
        Precedence.push_back(DisP);
        Precedence.push_back(DisN);
        SCIP_CALL( SCIPreleaseCons(scip, &DisP) );
        SCIP_CALL( SCIPreleaseCons(scip, &DisN) );
    }
    for(j = 0; j < Ops.size(); j++) {
        SCIP_CONS *Slic;
        (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "(6)%d", j+1);
        SCIP_CALL( SCIPcreateConsBasicLinear(scip, &Slic, name, 0, NULL, NULL, Ops[j]->slices, SCIPinfinity(scip)) );
        for(q = 0; q < l; q++)
            SCIP_CALL( SCIPaddCoefLinear(scip, Slic, y[j*l+q], 1.0) );
        SCIP_CALL( SCIPaddCons(scip, Slic) );
        Slice.push_back(Slic);
        SCIP_CALL( SCIPreleaseCons(scip, &Slic) );
    }
    for(i = 0; i < jobs.size(); i++) for (j = 0; j < jobs[i].ops.size(); j++) {
        SCIP_CONS *Last;
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
    SCIP_CALL( SCIPreleaseVar(scip, &Cmax) );
    for(i = 0; i < jobs.size(); i++)
        SCIP_CALL( SCIPreleaseVar(scip, &c[i]) );
    for(j1 = 0; j1 < Ops.size(); j1++) for(j2 = j1+1; j2 < Ops.size(); j2++) for(q = 0; q < l; q++)
        SCIP_CALL( SCIPreleaseVar(scip, &z[j1*Ops.size()*l+j2*l+q]) );
    return SCIP_OKAY;
}

SCIP_RETCODE RunJSP(const std::string &outfile, std::vector<Job> &jobs, const uint16_t &l) {
    SCIP* scip;
    SCIP_SOL* Sol;
    uint32_t dGCD;
    int NSols;
    std::vector<SCIP_VAR*> x, y;
    SCIP_CALL( SCIPcreate(&scip) );
    SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

    SCIP_CALL( FormulateMIP(scip, x, y, outfile+".lp", jobs, l, dGCD) );
    SCIP_CALL( SCIPprintOrigProblem(scip, NULL, "cip", FALSE) );

    SCIPinfoMessage(scip, NULL, "\nPresolving...\n");
    SCIP_CALL( SCIPpresolve(scip) );

    SCIPinfoMessage(scip, NULL, "\nSolving...\n");
    SCIP_CALL( SCIPsolve(scip) );

    if( (NSols = SCIPgetNSols(scip)) > 0 ) {
        SCIPinfoMessage(scip, NULL, "\nSolution:\n");
        Sol = SCIPgetBestSol(scip);
        SCIP_CALL( SCIPprintSol(scip, Sol, NULL, FALSE) );
        for(auto &job : jobs) for (auto &op : job.ops) {
            op.start_time = SCIPgetSolVal(scip, Sol, x[op.ij]) * dGCD;
            std::cerr<<SCIPvarGetName(x[op.ij])<<" "<<SCIPgetSolVal(scip, Sol, x[op.ij])<<" "<<op.start_time<<"\n";
            for(uint8_t q = 0; q < l; q++) {
                if(SCIPgetSolVal(scip, Sol, y[op.ij*l+q]) > 0 && op.in_slice.size() < op.slices) op.in_slice.push_back(q);
                std::cerr<<SCIPvarGetName(y[op.ij*l+q])<<" "<<SCIPgetSolVal(scip, Sol, y[op.ij*l+q])<<"\n";
            }
        }
    }

    for(auto &i : x) SCIP_CALL( SCIPreleaseVar(scip, &i) );
    for(auto &j : y) SCIP_CALL( SCIPreleaseVar(scip, &j) );
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
        return jobs[a].weight > jobs[b].weight;
    });
    for (auto &j : j_order) {
        uint32_t j_end{global_start};
        // Sort the operations in (Reversed) Topological Ordering
        for (uint16_t i = 0; i < jobs[j].ops.size(); i++)
            OpTopoSort(jobs[j], i);
        assert(jobs[j].ops.size() == jobs[j].opTopo.size());
        // Schedule each operation in the job
        for (auto &o : jobs[j].opTopo) {
            uint32_t j_start{j_end};
            assert(!jobs[j].ops[o].done);
            for (auto &d : jobs[j].ops[o].deps) {
                j_start = std::max<uint32_t>(j_start, jobs[j].ops[d].start_time + jobs[j].ops[d].duration);
            }
            jobs[j].ops[o].start_time = j_end;
            std::cerr << o << " " << jobs[j].ops[o].start_time << " " << j_start << " " << j_end << " " << jobs[j].ops[o].slices << "\n";
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

    if(RunJSP(argv[2], jobs, l) != SCIP_OKAY) TraditionalScheduling(jobs, l);
    const auto score = CalculateScore(argv[2], jobs);

    std::cout << std::fixed << std::setprecision(8) << score << '\n';
}
