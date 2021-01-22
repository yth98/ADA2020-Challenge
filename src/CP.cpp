#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_checker.h"
#include "PS.h"

namespace operations_research {
namespace sat {
CpSolverStatus RunPS_CP(std::vector<Job> &jobs, const uint16_t &l, const long double &bound, const uint32_t &span) {
    int64 Bound;
    uint32_t dGCD, wGCD{1000000}, V{0}, tLimit;
    uint16_t m{0}, Group_start, Group_size, i, j, q;
    char name[32];
    bool use_interval = false;
    std::vector<Operation*> Ops;
    std::vector<uint32_t> w, slice_end(l, 0);
    std::vector<uint16_t> j_order(jobs.size()), o_order;

    dGCD = jobs[0].ops[0].duration;
    for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++) {
            dGCD = std::__gcd(dGCD, jobs[i].ops[j].duration);
            jobs[i].ops[j].ij = Ops.size();
            Ops.push_back(&jobs[i].ops[j]);
        }
    for(i = 0; i < jobs.size(); i++)
        for(j = 0; j < jobs[i].ops.size(); j++)
            V += jobs[i].ops[j].duration / dGCD;
    for(i = 0; i < jobs.size(); i++) {
        w.push_back(std::round(jobs[i].weight*1000000.0));
        wGCD = std::__gcd(wGCD, w.back());
    }
    Bound = ((int64)std::ceil(bound*1000000.0)) / dGCD / wGCD;
    LOG(INFO) << "Bound = " << Bound << " Span <= " << span << "\n";

    // Sort the jobs by weight in descending order
    o_order.resize(Ops.size());
    for(i = 0; i < jobs.size(); i++) j_order[i] = i;
    std::sort(j_order.begin(), j_order.end(), [&](const uint16_t &a, const uint16_t &b) {
        return jobs[a].weight > jobs[b].weight;
    });
    m = 0;
    for(i = 0; i < j_order.size(); i++) for (j = 0; j < jobs[j_order[i]].opTopo.size(); j++)
        o_order[jobs[j_order[i]].ops[jobs[j_order[i]].opTopo[j]].ij] = m++;

    Group_size = (l >= 9) ? 12 : (l >= 6) ? 20 : jobs.size();
    CpSolverStatus GlobalStatus;
    for(Group_start = 0; Group_start < jobs.size(); Group_start+=Group_size) {
        std::cerr << "CP job group " << (Group_start+1) << " ~ " << (Group_start+Group_size) << "\n";
        CpModelBuilder cp_model;
        // Variables
        std::vector<IntVar> c, xs, xe;
        std::vector<IntervalVar> xinterval[l];
        std::vector<BoolVar> y, z;
        const Domain time(0, std::min<uint32_t>(V, span));
        const IntVar Cmax{cp_model.NewIntVar(time).WithName("makespan")};
        uint16_t ij{0};
        for(i = 0; i < jobs.size(); i++) {
            for(j = 0; j < jobs[i].ops.size(); j++) {
                std::snprintf(name, sizeof(name), "start_%d", ++ij);
                xs.push_back(cp_model.NewIntVar(time).WithName(name));
                std::snprintf(name, sizeof(name), "end_%d", ij);
                xe.push_back(cp_model.NewIntVar(time).WithName(name));
                for(q = 0; q < l; q++) {
                    std::snprintf(name, sizeof(name), "y_%d_%d", ij, q+1);
                    y.push_back(cp_model.NewBoolVar().WithName(name));
                }
            }
            std::snprintf(name, sizeof(name), "c_%d", i+1);
            c.push_back(cp_model.NewIntVar(time).WithName(name));
        }
        std::cerr << "C " << c.size() << " X " << xs.size() <<  " " << xe.size();
        std::cerr << " Y " << y.size() << "\n";
        // Constraints
        for(i = Group_start; i < std::min<uint16_t>(Group_start+Group_size,jobs.size()); i++) {
            std::vector<IntVar> op_ends;
            std::cerr << "Job " << (j_order[i]+1) << "\n";
            // Disjunctive and Width
            for(j = 0; j < jobs[j_order[i]].ops.size(); j++) {
                const IntVar Duration = cp_model.NewConstant(jobs[j_order[i]].ops[j].duration/dGCD);
                ij = jobs[j_order[i]].ops[j].ij;
                std::cerr << "Operation " << (j+1) << " (" << (ij+1) << ")\n";
                std::vector<BoolVar> slice_bools(y.begin()+(ij*l), y.begin()+((ij+1)*l));
                cp_model.AddEquality(LinearExpr::BooleanSum(slice_bools), cp_model.NewConstant(jobs[j_order[i]].ops[j].slices));
                op_ends.push_back(xe[ij]);
            }
            // Precedence
            for(j = 0; j < jobs[j_order[i]].ops.size(); j++) for(auto &d : jobs[j_order[i]].ops[j].deps)
                cp_model.AddGreaterOrEqual(xs[jobs[j_order[i]].ops[j].ij], xe[jobs[j_order[i]].ops[d].ij]);
            cp_model.AddMaxEquality(c[j_order[i]], op_ends);
        }
        for(i = 0; i < Ops.size(); i++)
            cp_model.AddEquality(xe[i], xs[i].AddConstant(Ops[i]->duration/dGCD));
        for(q = 0; q < l; q++) {
            const IntVar slice_per_end = cp_model.NewConstant(slice_end[q]);
            for(i = 0; i < Ops.size(); i++)
                cp_model.AddGreaterOrEqual(xs[i], slice_per_end).OnlyEnforceIf(y[i*l+q]);
        }
        for(i = 0; i < Ops.size()-1; i++) for(j = i+1; j < Ops.size(); j++) {
            z.push_back(cp_model.NewBoolVar());
            if(Ops[i]->slices + Ops[j]->slices > l) {
                cp_model.AddGreaterOrEqual(xs[j], xe[i]).OnlyEnforceIf(z.back());
                cp_model.AddGreaterOrEqual(xs[i], xe[j]).OnlyEnforceIf(z.back().Not());
            } else for(q = 0; q < l; q++) {
                cp_model.AddGreaterOrEqual(xs[j], xe[i]).OnlyEnforceIf({y[i*l+q], y[j*l+q], z.back()});
                cp_model.AddGreaterOrEqual(xs[i], xe[j]).OnlyEnforceIf({y[i*l+q], y[j*l+q], z.back().Not()});
            }
        }

        cp_model.AddMaxEquality(Cmax, c);
        LinearExpr obj;
        obj.AddTerm(Cmax, 1000000/wGCD);
        for(i = 0; i < c.size(); i++) obj.AddTerm(c[i], w[i]/wGCD);
        cp_model.AddLessOrEqual(Cmax, span);
        cp_model.AddLessOrEqual(obj, Bound);
        cp_model.Minimize(obj);

        std::cerr << cp_model.Proto().variables_size() << " variables ";
        std::cerr << cp_model.Proto().constraints_size() << " constraints\n";
        std::cerr << ValidateCpModel(cp_model.Proto()) << "\n";
        Model model;
        model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& r) {
            LOG(INFO) << "Solution makespan " << SolutionIntegerValue(r, Cmax)
                    << " objective " << SolutionIntegerValue(r, obj);
        }));

        SatParameters parameters;
        parameters.set_num_search_workers(32);
        parameters.set_enumerate_all_solutions(false);
        parameters.set_max_time_in_seconds(timeLimit(l, Ops.size(), false)*(1.0)*Group_size/jobs.size());
        model.Add(NewSatParameters(parameters));
        const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);
        LOG(INFO) << CpSolverResponseStats(response);
        if((GlobalStatus = response.status()) == CpSolverStatus::OPTIMAL || GlobalStatus == CpSolverStatus::FEASIBLE)
            for(i = Group_start; i < std::min<uint16_t>(Group_start+Group_size,jobs.size()); i++)
                for(j = 0; j < jobs[j_order[i]].ops.size(); j++) {
                    Operation* op = &jobs[j_order[i]].ops[j];
                    op->start_time = SolutionIntegerValue(response, xs[op->ij]) * dGCD;
                    op->in_slice.clear();
                    for(q = 0; q < l; q++) if(SolutionIntegerValue(response, y[op->ij*l+q])) {
                        op->in_slice.push_back(q);
                        slice_end[q] = std::max<uint32_t>(slice_end[q], SolutionIntegerValue(response, xe[op->ij]));
                    }
                }
        else return GlobalStatus;
    }
    return GlobalStatus;
}
} // namespace sat
} // namespace operations_research
