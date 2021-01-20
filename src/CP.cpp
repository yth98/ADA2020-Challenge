#include "ortools/sat/cp_model.h"
#include "PS.h"

namespace operations_research {
namespace sat {
CpSolverStatus RunPS_CP(std::vector<Job> &jobs, const uint16_t &l, const long double &bound, const uint32_t &span) {
    int64 Bound;
    uint32_t dGCD, wGCD{1000000}, V{0}, tLimit;
    CpModelBuilder cp_model;
    uint16_t i, j, ij{0}, q;
    char name[32];
    std::vector<Operation*> Ops;
    std::vector<IntVar> c, xs, xe;
    std::vector<BoolVar> y, z;
    std::vector<IntervalVar> xinterval[l];
    std::vector<uint32_t> w;

    dGCD = jobs[0].ops[0].duration;
    for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++)
            dGCD = std::__gcd(dGCD, jobs[i].ops[j].duration);
    for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++)
            V += jobs[i].ops[j].duration / dGCD;
    for(i = 0; i < jobs.size(); i++) {
        w.push_back(std::round(jobs[i].weight*1000000.0));
        wGCD = std::__gcd(wGCD, w.back());
    }
    Bound = ((int64)std::ceil(bound*1000000.0)) / dGCD / wGCD;
    LOG(INFO) << "Bound = " << Bound << " Span <= " << span << "\n";

    const Domain time(0, std::min<uint32_t>(V, span));
    const IntVar Cmax = cp_model.NewIntVar(time).WithName("makespan");
    for(i = 0; i < jobs.size(); i++) {
        for(j = 0; j < jobs[i].ops.size(); j++) {
            const IntVar Duration = cp_model.NewConstant(jobs[i].ops[j].duration/dGCD);
            jobs[i].ops[j].ij = Ops.size();
            std::snprintf(name, sizeof(name), "start_%d", ++ij);
            xs.push_back(cp_model.NewIntVar(time).WithName(name));
            std::snprintf(name, sizeof(name), "end_%d", ij);
            xe.push_back(cp_model.NewIntVar(time).WithName(name));
            for(q = 0; q < l; q++) {
                std::snprintf(name, sizeof(name), "y_%d_%d", ij, q);
                y.push_back(cp_model.NewBoolVar().WithName(name));
                std::snprintf(name, sizeof(name), "op_%d_%d", ij, q);
                xinterval[q].push_back(cp_model.NewOptionalIntervalVar(xs.back(), Duration, xe.back(), y.back()).WithName(name));
            }
            cp_model.AddEquality(LinearExpr::BooleanSum(std::vector<BoolVar>(y.end()-l, y.end())), cp_model.NewConstant(jobs[i].ops[j].slices));
            Ops.push_back(&jobs[i].ops[j]);
        }
        for(j = 0; j < jobs[i].ops.size(); j++) for(auto &d : jobs[i].ops[j].deps) {
            LinearExpr precedence(xs[jobs[i].ops[j].ij]);
            precedence.AddTerm(xe[jobs[i].ops[d].ij], -1);
            cp_model.AddGreaterOrEqual(precedence, 0);
        }
        std::snprintf(name, sizeof(name), "c_%d", i+1);
        c.push_back(cp_model.NewIntVar(time).WithName(name));
        cp_model.AddMaxEquality(c.back(), std::vector<IntVar>(xe.end()-jobs[i].ops.size(), xe.end()));
    }
    for(q = 0; q < l; q++) cp_model.AddNoOverlap(xinterval[q]);
    for(i = 0; i < Ops.size()-1; i++) for(j = i+1; j < Ops.size(); j++) if(Ops[i]->slices + Ops[j]->slices > l) {
        z.push_back(cp_model.NewBoolVar());
        cp_model.AddGreaterOrEqual(xs[j], xe[i]).OnlyEnforceIf(z.back());
        cp_model.AddGreaterOrEqual(xs[i], xe[j]).OnlyEnforceIf(Not(z.back()));
    }
    for(i = j = 0; i < jobs.size(); i++, j+=jobs[i].ops.size()) {
        if(Ops[j]->slices < l) {
            cp_model.AddEquality(y[j], cp_model.TrueVar()); // Eliminate the symmetry
            break;
        }
    }

    cp_model.AddMaxEquality(Cmax, c);
    LinearExpr obj;
    obj.AddTerm(Cmax, 1000000/wGCD);
    for(i = 0; i < jobs.size(); i++) obj.AddTerm(c[i], w[i]/wGCD);
    cp_model.AddLessOrEqual(Cmax, span);
    cp_model.AddLessOrEqual(obj, Bound);
    cp_model.Minimize(obj);

    Model model;
    model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& r) {
        LOG(INFO) << "Solution makespan " << SolutionIntegerValue(r, Cmax)
                  << " objective " << SolutionIntegerValue(r, obj);
        for(auto s : xs) std::cerr << SolutionIntegerValue(r, s) << " "; std::cerr << "\n";
        for(auto e : xe) std::cerr << SolutionIntegerValue(r, e) << " "; std::cerr << "\n";
    }));

    SatParameters parameters;
    parameters.set_enumerate_all_solutions(false);
    parameters.set_max_time_in_seconds(timeLimit(l, Ops.size(), false));
    model.Add(NewSatParameters(parameters));
    const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);
    LOG(INFO) << CpSolverResponseStats(response);
    if(response.status() == CpSolverStatus::OPTIMAL || response.status() == CpSolverStatus::FEASIBLE) for(auto op : Ops) {
        op->start_time = SolutionIntegerValue(response, xs[op->ij]) * dGCD;
        op->in_slice.clear();
        for(q = 0; q < l; q++) if(SolutionIntegerValue(response, y[op->ij*l+q])) op->in_slice.push_back(q);
    }
    return response.status();
}
} // namespace sat
} // namespace operations_research
