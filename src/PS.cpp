#include "PS.h"

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

void WriteSchedule(
    const std::string &outfile, const std::vector<Job> &jobs) {
    std::ofstream outf(outfile);

    for (size_t j = 0; j < jobs.size(); j++) for (size_t k = 0; k < jobs[j].ops.size(); k++) {
        outf << jobs[j].ops[k].start_time;
        for (auto &s : jobs[j].ops[k].in_slice) outf << " " << (s + 1);
        outf << "\n";
    }
}

long double CalculateScore(std::vector<Job> &jobs) {
    long double makespan = 0, weighted_flow = 0;
    for (size_t j = 0; j < jobs.size(); j++) {
        long double job_end = 0;
        for (size_t k = 0; k < jobs[j].ops.size(); k++) {
            uint32_t fin = jobs[j].ops[k].start_time + jobs[j].ops[k].duration;
            makespan = std::max<long double>(makespan, fin);
            job_end = std::max<long double>(job_end, fin);
        }
        weighted_flow += static_cast<long double>(jobs[j].weight) * job_end;
    }
    return weighted_flow + makespan;
}

uint32_t TraditionalScheduling(std::vector<Job> &jobs, const uint16_t &l) {
    std::vector<uint16_t> j_order(jobs.size());
    std::vector<uint32_t> slice_end(l, 0);
    std::vector<uint8_t> slice_order(l);
    // Sort the jobs by weight in descending order
    for (uint8_t i = 0; i < jobs.size(); i++) j_order[i] = i;
    std::sort(j_order.begin(), j_order.end(), [&](const uint16_t &a, const uint16_t &b) {
        if (l == 1) return jobs[a].weight * jobs[b].duration >= jobs[b].weight * jobs[a].duration;
        return jobs[a].weight >= jobs[b].weight;
    });
    for (uint8_t q = 0; q < l; q++) slice_order[q] = q;
    for (auto &j : j_order) {
        // Schedule each operation in the job
        for (auto &o : jobs[j].opTopo) {
            uint32_t j_end;
            std::sort(slice_order.begin(), slice_order.end(), [&](const uint8_t &a, const uint8_t &b) {
                return slice_end[a] < slice_end[b];
            });
            j_end = slice_end[slice_order[jobs[j].ops[o].slices-1]];
            uint32_t j_start{j_end};
            assert(!jobs[j].ops[o].done);
            for (auto &d : jobs[j].ops[o].deps) {
                j_start = std::max<uint32_t>(j_start, jobs[j].ops[d].start_time + jobs[j].ops[d].duration);
            }
            jobs[j].ops[o].start_time = j_start;
            // std::cerr << o << " " << jobs[j].ops[o].start_time << " " << j_start << " " << j_end << " " << jobs[j].ops[o].slices << "\n";
            if (j_start == j_end)
                for (uint8_t s = 0; s < jobs[j].ops[o].slices; s++) {
                    jobs[j].ops[o].in_slice.push_back(slice_order[s]);
                    slice_end[slice_order[s]] = jobs[j].ops[o].start_time + jobs[j].ops[o].duration;
                }
            else {
                uint8_t q{(uint8_t)(jobs[j].ops[o].slices-1)};
                while (q < l && slice_end[slice_order[q]] <= j_start) ++q;
                for (uint8_t s = q-jobs[j].ops[o].slices; s < q; s++) {
                    jobs[j].ops[o].in_slice.push_back(slice_order[s]);
                    slice_end[slice_order[s]] = jobs[j].ops[o].start_time + jobs[j].ops[o].duration;
                }
            }
            jobs[j].ops[o].done = true;
        }
        // std::cerr << "\n";
    }
    return *std::max_element(slice_end.begin(), slice_end.end());
}

// There is a time limit of 12 hours for the public tests combined
// The time limit for the private tests combined is 24 hours
int timeLimit(const uint32_t &l, const uint32_t m, bool isMIP) {
    int tLimit;
    if(!isMIP) // CP
        switch(l) {
            case 2: tLimit =   60; break;
            case 3: tLimit =  180; break;
            case 4: tLimit =  300; break;
            case 5: tLimit =  600; break;
            case 6: tLimit =  900; break;
            case 7: tLimit = 1080; break;
            default:tLimit = 1200;
        }
    else // MIP
        switch(m) {
            case   1 ...  20: tLimit =   600; break; // 0 1 2 3 4
            case  21 ...  39: tLimit =   900; break; // 7 6 5
            case  40 ...  90: tLimit =  1200; break; // 8 9
            default: tLimit = 1; // 10
        }
    return tLimit;
}
