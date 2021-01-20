#include "src/JSP.h"
#include "src/MIP.cpp"
#include "src/CP.cpp"

int main(int argc, char **argv) {
    long double score, score2, score3;
    uint32_t TradSpan;
    SCIP_RETCODE JSP_MIP = SCIP_ERROR;
    operations_research::sat::CpSolverStatus JSP_CP = operations_research::sat::CpSolverStatus::UNKNOWN;
    bool JSP_CP_OK = false;
    assert(argc == 3);
    auto [l, jobs] = ReadJobs(argv[1]);

    TradSpan = TraditionalScheduling(jobs, l);
    score2 = score = CalculateScore(jobs);
    WriteSchedule(argv[2], jobs);

    if(l >= 2) {
        if(l <= 8) JSP_CP = operations_research::sat::RunJSP_CP(jobs, l, score, TradSpan*1.1);
        JSP_CP_OK = JSP_CP == operations_research::sat::CpSolverStatus::OPTIMAL ||
                    JSP_CP == operations_research::sat::CpSolverStatus::FEASIBLE;
        if(JSP_CP_OK && (score2 = CalculateScore(jobs)) < score)
            WriteSchedule(argv[2], jobs);
        if(l >= 5) if(JSP_CP != operations_research::sat::CpSolverStatus::OPTIMAL &&
           (JSP_MIP = RunJSP(argv[2], jobs, l, score)) == SCIP_OKAY &&
           (score3 = CalculateScore(jobs)) < std::min<long double>(score, score2))
            WriteSchedule(argv[2], jobs);
    }

    std::cout << "Trad:   " <<  std::fixed << std::setprecision(8) << score << '\n';
    if(JSP_CP_OK)
        std::cout << "CP-SAT: " <<  std::fixed << std::setprecision(8) << score2 << '\n';
    if(JSP_MIP == SCIP_OKAY)
        std::cout << "SCIP:   " <<  std::fixed << std::setprecision(8) << CalculateScore(jobs) << '\n';
}
