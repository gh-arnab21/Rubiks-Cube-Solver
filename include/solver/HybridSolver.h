/**
 * @file HybridSolver.h
 * @brief Hybrid solver combining Two-Phase divide and conquer with Korf's IDA* optimal solve
 */

#ifndef HYBRID_SOLVER_H
#define HYBRID_SOLVER_H

#pragma once

#include <memory>
#include "solver/Solver.h"
#include "solver/TwoPhase.h"

namespace cube_solver {

/**
 * @class HybridSolver
 * @brief Uses Kociemba Phase 1 to reach G1, then Korf's IDA* to optimally solve the rest
 */
class HybridSolver : public Solver {
public:
    HybridSolver(bool useSymmetryReduction = true);
    ~HybridSolver() override = default;
    
    bool loadDatabases(const std::string& dataDirectory);
    bool generateDatabases(const std::string& dataDirectory);
    
    SolutionResult solve(const RubiksCube& cube) override;
    bool isReady() const override { return twoPhaseSolver->isReady() && korfSolver->isReady(); }
    std::string getName() const override { return "Hybrid (Two-Phase + Korf)"; }
    
private:
    std::unique_ptr<TwoPhase> twoPhaseSolver;
    std::unique_ptr<KorfSolver> korfSolver;
};

} // namespace cube_solver

#endif // HYBRID_SOLVER_H
