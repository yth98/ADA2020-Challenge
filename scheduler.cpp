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

namespace {

struct Operation {
    uint32_t duration{}, start_time{};
    uint16_t slices{};
    bool done{false}, inTopo{false};
    std::vector<uint16_t> deps;
    std::basic_string<uint8_t> in_slice;
};

struct Job {
    double weight{};
    std::vector<Operation> ops;
    std::vector<uint16_t> opTopo;
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

void OpTopoSort(Job &job, uint16_t &o) {
    if(job.ops[o].inTopo) return;
    assert(!job.ops[o].done);
    job.ops[o].done = true;
    for(auto d : job.ops[o].deps)
        OpTopoSort(job, d);
    job.ops[o].done = false;
    job.ops[o].inTopo = true;
    job.opTopo.push_back(o);
}

void Scheduling(std::vector<Job> &jobs, const uint16_t &l) {
    uint32_t global_start{0};
    for (auto &j : jobs) {
        uint32_t slice_used{0}, j_end{global_start};
        // Sort the operations in (Reversed) Topological Ordering
        for (uint16_t i = 0; i < j.ops.size(); i++)
            OpTopoSort(j, i);
        assert(j.ops.size() == j.opTopo.size());
        // Schedule each operation in the job
        for (auto &o : j.opTopo) {
            uint32_t j_start{j_end};
            assert(!j.ops[o].done);
            for (auto &d : j.ops[o].deps) {
                j_start = std::max<uint32_t>(j_start, j.ops[d].start_time + j.ops[d].duration);
            }
            if ((slice_used + j.ops[o].slices <= l)) {
                j.ops[o].start_time = j_start;
                slice_used += j.ops[o].slices;
            } else {
                j.ops[o].start_time = j_end;
                slice_used = j.ops[o].slices;
            }
            std::cerr << o << " " << j.ops[o].start_time << " " << j_start << " " << j_end << " " << slice_used << "\n";
            for (uint32_t s = slice_used - j.ops[o].slices; s < slice_used; s++)
                j.ops[o].in_slice.push_back(s);
            j_end = std::max<uint32_t>(j_end, j.ops[o].start_time + j.ops[o].duration);
            j.ops[o].done = true;
        }
        std::cerr << "\n";
        global_start = j_end;
    }
}

}  // namespace

int main(int argc, char **argv) {
    assert(argc == 3);
    auto [l, jobs] = ReadJobs(argv[1]);

    Scheduling(jobs, l);
    const auto score = CalculateScore(argv[2], jobs);

    std::cout << std::fixed << std::setprecision(8) << score << '\n';
}
