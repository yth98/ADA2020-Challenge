#include "src/PS.h"
// #include "src/MIP.cpp"
#include "src/CP.cpp"

int main(int argc, char **argv) {
    long double score, score2, score3;
    uint32_t TradSpan, m = 0;
    // SCIP_RETCODE PS_MIP = SCIP_ERROR;
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
        PS_CP = operations_research::sat::RunPS_CP(jobs, l, score, TradSpan*1.1);
        PS_CP_OK = PS_CP == operations_research::sat::CpSolverStatus::OPTIMAL ||
                    PS_CP == operations_research::sat::CpSolverStatus::FEASIBLE;
        if(PS_CP_OK && (score2 = CalculateScore(jobs)) < score)
            WriteSchedule(argv[2], jobs);
        // if(false) if(PS_CP != operations_research::sat::CpSolverStatus::OPTIMAL &&
        //    (PS_MIP = RunPS(argv[2], jobs, l, score)) == SCIP_OKAY &&
        //    (score3 = CalculateScore(jobs)) < std::min<long double>(score, score2))
        //     WriteSchedule(argv[2], jobs);
    }

    std::cout << "Trad:   " <<  std::fixed << std::setprecision(8) << score << '\n';
    if(PS_CP_OK)
        std::cout << "CP-SAT: " <<  std::fixed << std::setprecision(8) << score2 << '\n';
    // if(PS_MIP == SCIP_OKAY)
    //     std::cout << "SCIP:   " <<  std::fixed << std::setprecision(8) << CalculateScore(jobs) << '\n';
}
