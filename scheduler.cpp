#include "src/PS.h"
#include "src/CP.cpp"

int main(int argc, char **argv) {
    long double score, score2;
    uint32_t TradSpan, m = 0;
    operations_research::sat::CpSolverStatus PS_CP = operations_research::sat::CpSolverStatus::UNKNOWN;
    bool PS_CP_OK = false;
    assert(argc >= 3);
    auto [l, jobs] = ReadJobs(argv[1]);

    TradSpan = TraditionalScheduling(jobs, l);
    score2 = score = CalculateScore(jobs);
    WriteSchedule(argv[2], jobs);

    for(auto j : jobs) m += j.ops.size();
    if(l >= 2) {
        if (l >= 6 && jobs.size() >= 8 && score < 10000.0) return 0;
        PS_CP = operations_research::sat::RunPS_CP(jobs, l, score, TradSpan*1.5);
        PS_CP_OK = PS_CP == operations_research::sat::CpSolverStatus::OPTIMAL ||
                    PS_CP == operations_research::sat::CpSolverStatus::FEASIBLE;
        if(PS_CP_OK && (score2 = CalculateScore(jobs)) < score)
            WriteSchedule(argv[2], jobs);
    }

    std::cout << "Trad:   " <<  std::fixed << std::setprecision(8) << score << '\n';
    if(PS_CP_OK)
        std::cout << "CP-SAT: " <<  std::fixed << std::setprecision(8) << score2 << '\n';
}
